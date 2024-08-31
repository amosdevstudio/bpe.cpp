/*
    bpe.cpp - A simple, fast and multithreaded Byte Pair Encoder written in c++ with python bindings.
    Copyright (C) 2024  Lorenzo Amos Sanzullo

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "bpe.hpp"
#include "datastructures.hpp"

#include <fstream>
#include <iostream>

using namespace std;

void ReadFile(const string& path, string& output){
    ifstream file(path, ios::binary);

    if (!file.is_open()) {
        cerr << "Could not open file: " << path << endl;
        exit(-1);
    }

    output = string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
}

void CountTokens(const TokenList& tokens, Heap& heap){
    TokenNode* token = tokens.head();
    while(token->next != nullptr){
        heap.AddPositionNoHeapify(token);
        token = token->next;
    }

    cout << "Making Heap..." << endl;
    heap.MakeHeap();
}

void BPE::BuildVocab(){
    cout << "Building vocab..." << endl;
    m_Vocab.resize(m_VocabSize);
    for(uint32_t i = 0; i < 256; ++i){
        m_Vocab[i] = string(1, i);
    }

    for(size_t i = 256; i < m_VocabSize; ++i){
        TokenPair pair = m_Merges[i-256];
        m_Vocab[i] = (m_Vocab[pair.token1] + m_Vocab[pair.token2]);
    }
    cout << "Vocab built." << endl;
}

void BPE::LoadSplitLetters(const string& splitLetters){
    m_SplitLettersString = splitLetters;
    for(unsigned char c : m_SplitLettersString){
        m_SplitLetters.insert(c);
    }
}

void BPE::Load(const string& path){
    ifstream file;

    file.open(path);

    if(!file.is_open()){
        cout << "Could not open file: " << path << endl;
        exit(-1);
    }

    {
        /* Load regex pattern */
        string splitLetterText;
        getline(file, splitLetterText);
        cout << splitLetterText << endl;

        LoadSplitLetters(splitLetterText);
    }

    {
        /* Load vocab size */
        string vocabSizeString;
        getline(file, vocabSizeString);

        m_VocabSize = stoi(vocabSizeString);

        cout << to_string(m_VocabSize) << endl;
    }

    {
        /* Load pairs */
        m_Merges.reserve(m_VocabSize - 256);
        while(true){
            string idx1String, idx2String;
            if(!getline(file, idx1String, ' ') || !getline(file, idx2String, '\n')){
                break;
            }

            m_Merges.push_back({
                (uint32_t)stoi(idx1String),
                (uint32_t)stoi(idx2String)
            });
        }
    }
    file.close();

    BuildVocab();
}

TokenList BPE::Encode(const std::string& text) const{
    TokenList tokens;
    StringToTokens(text, tokens);

    /* Slow implementation */
    for(size_t i = 256; i < m_VocabSize; ++i){
        const TokenPair& merge = m_Merges[i-256];
        TokenNode* token = tokens.head();
        while(token != nullptr && token->next != nullptr){
            if(token->val == 0){
                tokens.Remove(token);
                continue;
            }

            if(merge.token1 == token->val && merge.token2 == token->next->val){
                token->val = i;
                tokens.Remove(token->next);
            }

            token = token->next;
        }
    }
    return tokens;
}

std::vector<uint32_t> BPE::EncodeToVector(const std::string& text) const{
    TokenList tokenList = Encode(text);
    vector<uint32_t> tokens;

    TokenNode* token = tokenList.head();
    while(token != nullptr){
        tokens.push_back(token->val);
        token = token->next;
    }

    tokenList.DeleteContents();
    return tokens;
}

string BPE::Decode(const TokenList& tokens) const{
    string result;
    TokenNode* token = tokens.head();
    while(token != nullptr){
        result.append(m_Vocab[token->val]);
        token = token->next;
    }

    return result;
}

string BPE::DecodeFromVector(const vector<uint32_t>& tokens) const{
    string result;
    for(const uint32_t& token : tokens){
        result.append(m_Vocab[token]);
    }
    return result;
}

void BPE::StringToTokens(const string& data, TokenList& tokens) const{
    for(unsigned char c : data){
        if(m_SplitLetters.find(c) != m_SplitLetters.end()){
            tokens.Append(0);
        }
        tokens.Append(c);
        /* Why the need for all this complicated Regex? */
        /* This is much faster, higly parallelizable (thanks to the linked list), but still upgradeable */
        /* (Unicode support coming soon :P) */
    }
}

void BPE::Fit(const size_t vocabSize, const std::string& path){
    m_VocabSize = vocabSize;

    TokenList tokens;
    {
        string data;
        ReadFile(path, data);
        cout << "File read! :P" << path << endl;
        StringToTokens(data, tokens);
    }
    cout << "Tokens loaded! :P" << endl;

    Heap heap;
    CountTokens(tokens, heap);

    heap.Truncate(m_VocabSize-256);

    cout << "Tokens counted! :P" << endl;

    for(uint32_t i = 256; i < m_VocabSize; ++i){
        HeapNode* top = heap.PopTop();

        for(TokenNode* token : top->positions){
            heap.RemovePosition(token->prev);
            heap.RemovePosition(token->next);

            token->val = i;
            tokens.Remove(token->next);

            heap.AddPosition(token->prev);
            heap.AddPosition(token);
        }

        m_Merges.push_back(top->pair());
        heap.RemoveNode(top);

        if(i % 100 == 0){
            cout << "Token " << i << " reached." << endl;
        }

        if(heap.size() == 0){
            cout << "All words have a token. Breaking early." << endl;
            m_VocabSize = i+1;
            break;
        }

        heap.Truncate(m_VocabSize-256);
    }

    heap.DeleteContents();
    tokens.DeleteContents();

    BuildVocab();
}

void BPE::Save(const string& path) const{
    cout << "Saving..." << endl;
    ofstream file;
    file.open(path);

    file << m_SplitLettersString << "\n";
    file << to_string(m_VocabSize) << "\n";

    for(TokenPair pair : m_Merges){
        file << to_string(pair.token1) << " "
            << to_string(pair.token2) << "\n";
    }

    file.close();
    cout << "Saved." << endl;
}
