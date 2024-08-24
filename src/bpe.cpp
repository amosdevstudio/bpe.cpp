#ifndef BPE_HPP
#define BPE_HPP

#include "bpe.hpp"

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
        heap.AddPosition(token);
        token = token->next;
    }
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

TokenList BPE::Encode(const std::string& text){

}

string BPE::Decode(const TokenList& tokens){

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


std::string Decode(const std::vector<uint32_t>& tokens) const;

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

#endif
