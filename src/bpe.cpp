/*
    bpe.cpp - A simple and fast Byte Pair Encoder written in c++ with python bindings.
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


#define NUM_PAIRS m_VocabSize-256

#include "bpe.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include <thread>
#include <mutex>

using namespace std;

BPE::BPE():m_VocabSize(0), m_RegexPattern(nullptr){}
BPE::~BPE(){
    if (m_RegexPattern) {
        pcre2_code_free(m_RegexPattern);
    }
}

void BPE::BuildVocab(){
    m_Vocab.reserve(m_VocabSize);
    for(unsigned int i = 0; i < 256; ++i){
        m_Vocab.push_back(string(1, i));
    }

    for(size_t i = 0; i < NUM_PAIRS; ++i){
        Pair pair = m_Pairs[i];
        m_Vocab.push_back(m_Vocab[pair.idx1] + m_Vocab[pair.idx2]);
    }
}

void BPE::LoadRegex(const string& regexText) {
    cout << "Compiling regex..." << endl;

    int errorcode;
    PCRE2_SIZE erroroffset;
    uint32_t options = PCRE2_MULTILINE | PCRE2_CASELESS;

    m_RegexPattern = pcre2_compile(
        (PCRE2_SPTR)regexText.c_str(),
        PCRE2_ZERO_TERMINATED,
        options,
        &errorcode,
        &erroroffset,
        nullptr
    );

    if (m_RegexPattern == nullptr) {
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
        cerr << "PCRE2 compilation failed at offset " << erroroffset << ": " << buffer << endl;
        throw runtime_error("Regex compilation failed");
    }

    m_RegexString = regexText;

    cout << "Done." << endl;
}

void BPE::Load(const string& path){
    ifstream file;

    file.open(path);
    if(!file.is_open()){std:cout << "Could not open file: " << path << endl; }

    {
        /* Load regex pattern */
        string regexText;
        getline(file, regexText);
        cout << regexText << endl;

        LoadRegex(regexText);
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
        m_Pairs.reserve(NUM_PAIRS);
        while(true){
            string idx1String, idx2String;
            if(!getline(file, idx1String, ' ') || !getline(file, idx2String, '\n')){
                break;
            }

            m_Pairs.push_back({
                .idx1 = (unsigned int)stoi(idx1String),
                .idx2 = (unsigned int)stoi(idx2String)
            });

            /* Debugging */
            /*
            cout << to_string(m_Pairs[m_Pairs.size()-1].idx1) << ", " <<
                to_string(m_Pairs[m_Pairs.size()-1].idx2) << endl;
            */
        }
    }
    BuildVocab();

    file.close();
}

unsigned int BPE::FindFirstPair(const vector<unsigned int>& tokens, const unsigned int& startIdx){
    if(m_PairIndexMap.empty()){
        for(unsigned int i = startIdx; i < NUM_PAIRS; ++i){
            m_PairIndexMap[m_Pairs[i]] = i;
        }
    }

    /* Loop through all pairs (merges) */
    for(unsigned int j = 0; j < tokens.size()-1; ++j){
        Pair currentPair = {tokens[j], tokens[j+1]};
        /* If you find an instance in tokens */
        if(m_PairIndexMap.find(currentPair) != m_PairIndexMap.end()){
            /* Return the pair index */
            return m_PairIndexMap[currentPair];
        }
    }

    /* Here -1 is fine as it will just wrap around to the biggest number possible */
    return -1;
}

void BPE::Merge(vector<unsigned int>& tokens, const unsigned int& newToken) const{
    /* In-place implementation of the merge function */
    Pair pair = m_Pairs[newToken-256];
    unsigned int numTokens = tokens.size();
    /* The index for reading and the index for writing (read is faster) */
    unsigned int read=0, write=0;

    while(read < numTokens){
        if(read < numTokens - 1 && Pair{tokens[read], tokens[read+1]} == pair){
           /* Pair match */
            tokens[write] = newToken;
            read += 2; // Jump one token
        } else {
            /* No match */
            tokens[write] = tokens[read];
            ++read;
        }
        ++write;
    }
    /* Resize the tokens to the right size (no reallocation) */
    tokens.resize(write);
}

vector<unsigned int> BPE::EncodeChunk(const string& chunk, bool cache){
    if(cache && m_Cache.find(chunk) != m_Cache.end()){
        /* Chunk present in the cache */
        return m_Cache[chunk];
    }

    /* Vector of unicode bytes (total bytes = 255) */
    vector<unsigned int> tokens(chunk.begin(), chunk.end());

    unsigned int startSearchIdx = 0;
    unsigned int pairIdx;
    while((pairIdx = FindFirstPair(tokens, startSearchIdx)) != -1){
        startSearchIdx = pairIdx+1;
        Merge(tokens, pairIdx+256);
    }

    if(cache){
        m_Cache[chunk] = tokens;
    }

    return tokens;
}

vector<unsigned int> BPE::Encode(const string& text) {
    vector<unsigned int> tokens;

    pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(m_RegexPattern, nullptr);

    PCRE2_SIZE* ovector;
    int rc;
    PCRE2_SIZE start = 0;

    while ((rc = pcre2_match(m_RegexPattern, (PCRE2_SPTR)text.c_str(), text.length(), start, 0, match_data, nullptr)) > 0) {
        ovector = pcre2_get_ovector_pointer(match_data);
        string match = text.substr(ovector[0], ovector[1] - ovector[0]);

        for (unsigned int token : EncodeChunk(match)) {
            tokens.push_back(token);
        }

        start = ovector[1];
    }

    pcre2_match_data_free(match_data);

    return tokens;
}

string BPE::Decode(const vector<unsigned int>& tokens){
    string result = "";
    for(size_t i = 0; i < tokens.size(); ++i){
        result.append(m_Vocab[tokens[i]]);
    }
    return result;
}

vector<vector<unsigned int>> BPE::FileToTokenBuffer(const string& dataPath) {
    cout << "Loading file to memory..." << endl;

    vector<vector<unsigned int>> tokenBuffer;
    ifstream file(dataPath, ios::binary);

    if (!file.is_open()) {
        cerr << "Could not open file: " << dataPath << endl;
        return tokenBuffer;
    }

    string data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(m_RegexPattern, nullptr);

    PCRE2_SIZE* ovector;
    int rc;
    PCRE2_SIZE start = 0;

    while ((rc = pcre2_match(m_RegexPattern, (PCRE2_SPTR)data.c_str(), data.length(), start, 0, match_data, nullptr)) > 0) {
        ovector = pcre2_get_ovector_pointer(match_data);
        string match = data.substr(ovector[0], ovector[1] - ovector[0]);

        vector<unsigned int> tokens;
        tokens.reserve(match.size());
        for (unsigned char c : match) {
            tokens.push_back(static_cast<unsigned int>(c));
        }
        tokenBuffer.push_back(std::move(tokens));

        start = ovector[1];
    }

    pcre2_match_data_free(match_data);
    cout << "Done." << endl;

    return tokenBuffer;
}

Pair BPE::GetMostFrequentPair(const vector<vector<unsigned int>>& tokenBuffer, const size_t numThreads) const {
    unordered_map<Pair, size_t> pairFrequency;
    mutex threadFrequencyMutex;
    vector<thread> threads;

    Pair mostFrequentPair{0, 0};
    size_t maxFrequency = 1;

    auto processChunk = [&](size_t threadId, size_t start, size_t end) {
        unordered_map<Pair, size_t> localFrequency;
        for (size_t i = start; i < end; ++i) {
            for (size_t j = 0; j < tokenBuffer[i].size() - 1; ++j) {
                Pair pair{tokenBuffer[i][j], tokenBuffer[i][j+1]};
                ++localFrequency[pair];
            }
        }

        lock_guard<mutex> lock(threadFrequencyMutex);;
        for (const auto& [pair, freq] : localFrequency) {
            auto iter = pairFrequency.find(pair);
            if(iter == pairFrequency.end()){
                iter = pairFrequency.insert({pair, 0}).first;
            }

            iter->second += freq;
            if(iter->second > maxFrequency){
                mostFrequentPair = pair;
                maxFrequency = iter->second;
            }
        }
    };

    size_t chunkSize = tokenBuffer.size() / numThreads;
    for (size_t i = 0; i < numThreads; ++i) {
        size_t start = i * chunkSize;
        size_t end = (i == numThreads - 1) ? tokenBuffer.size() : (i + 1) * chunkSize;
        threads.emplace_back(processChunk, i, start, end);
    }

    for (auto& t : threads) {
        t.join();
    }

    return mostFrequentPair;
}

void BPE::Fit(const size_t vocabSize, const string& dataPath, const size_t numThreads){
    m_Cache.clear();
    m_VocabSize = vocabSize;
    vector<vector<unsigned int>> tokenBuffer = FileToTokenBuffer(dataPath);

    m_Pairs.reserve(NUM_PAIRS);

    for(unsigned int i = 256; i < m_VocabSize; ++i){
        Pair pair = GetMostFrequentPair(tokenBuffer, numThreads);

        if(pair.idx1 == 0 && pair.idx2 == 0){
            /* No more tokens to merge */
            cout << "No more merges... Returning..." << endl;
            m_VocabSize = m_Pairs.size() + 256;
            break;
        }

        m_Pairs.push_back(pair);

        for(size_t idx = 0; idx < tokenBuffer.size(); ++idx){
            Merge(tokenBuffer[idx], i);
        }

        if(i % 100 == 0){
            cout << "Reached token " << to_string(i) << endl;
        }
    }

    BuildVocab();
}

void BPE::Save(const string& path){
    ofstream file;
    file.open(path);

    file << m_RegexString << "\n";
    file << to_string(m_VocabSize) << "\n";

    for(Pair pair : m_Pairs){
        file << to_string(pair.idx1) << " "
            << to_string(pair.idx2) << "\n";
    }

    file.close();
}
