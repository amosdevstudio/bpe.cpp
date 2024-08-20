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

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <iterator>

using namespace std;

string ReadFile(const string& path){
    ifstream file(path, ios::binary);

    if (!file.is_open()) {
        cerr << "Could not open file: " << path << endl;
        exit(-1);
    }

    string data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    cout << "Read file " << path << endl;
    return data;
}

BPE::BPE(const size_t numThreads):m_VocabSize(0), m_RegexPattern(nullptr), m_NumThreads(numThreads){}

BPE::~BPE(){
    if (m_RegexPattern) {
        pcre2_code_free(m_RegexPattern);
    }
}

void BPE::BuildVocab(){
    cout << "Building vocab..." << endl;
    m_Vocab.resize(m_VocabSize);
    for(unsigned int i = 0; i < 256; ++i){
        m_Vocab[i] = string(1, i);
    }

    for(size_t i = 256; i < m_VocabSize; ++i){
        Pair pair = m_Pairs[i-256];
        m_Vocab[i] = (m_Vocab[pair.idx1] + m_Vocab[pair.idx2]);
    }
    cout << "Vocab built." << endl;
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

    if(!file.is_open()){
        cout << "Could not open file: " << path << endl;
        exit(-1);
    }

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
        m_Pairs.reserve(m_VocabSize - 256);
        while(true){
            string idx1String, idx2String;
            if(!getline(file, idx1String, ' ') || !getline(file, idx2String, '\n')){
                break;
            }

            m_Pairs.push_back({
                (unsigned int)stoi(idx1String),
                (unsigned int)stoi(idx2String)
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
        for(unsigned int i = startIdx; i < m_VocabSize - 256; ++i){
            m_PairIndexMap[m_Pairs[i]] = i;
        }
    }

    /* Loop through all pairs (merges) */
    for(unsigned int j = 0; j < tokens.size()-1; ++j){
        Pair currentPair = {tokens[j], tokens[j+1]};
        /* If you find an instance in tokens */
        auto iter = m_PairIndexMap.find(currentPair);
        if(iter != m_PairIndexMap.end()){
            /* Return the pair index */
            return iter->second;
        }
    }

    /* Here -1 is fine as it will just wrap around to the biggest number possible */
    return -1;
}

void BPE::Merge(vector<unsigned int>& tokens, const unsigned int& newToken) const{
    /* In-place implementation of the merge function */
    Pair pair = m_Pairs[newToken-256];
    /* The index for reading and the index for writing (read is faster) */
    unsigned int read=0, write=0;

    while(read < tokens.size()){
        if(read < tokens.size() - 1 && Pair{tokens[read], tokens[read+1]} == pair){
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

vector<PCRE2_SIZE> BPE::SplitTextSinglethreaded(const string& text, PCRE2_SIZE start, const PCRE2_SIZE end) const {
    pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(m_RegexPattern, nullptr);
    vector<PCRE2_SIZE> result;
    PCRE2_SIZE* ovector;
    int rc;

    while (start < end && (rc = pcre2_match(m_RegexPattern, (PCRE2_SPTR)text.c_str(), text.length(), start, 0, match_data, nullptr)) > 0) {
        ovector = pcre2_get_ovector_pointer(match_data);
        result.push_back(ovector[1]);

        start = ovector[1];
    }
    pcre2_match_data_free(match_data);
    return result;
}

vector<PCRE2_SIZE> BPE::SplitText(const string& text) const {
    if (text.length() <= m_NumThreads * 2) {
        return SplitTextSinglethreaded(text, 0, text.length());
    }

    vector<PCRE2_SIZE> result;
    vector<thread> threads;
    mutex resultMutex;
    condition_variable cv;
    size_t activeThread = 0;

    auto processChunk = [&](size_t threadId, PCRE2_SIZE start, const PCRE2_SIZE end) {
        auto localMatches = SplitTextSinglethreaded(text, start, end);

        unique_lock<mutex> lock(resultMutex);
        cv.wait(lock, [&] { return activeThread == threadId; });

        auto matchesBegin = localMatches.begin();
        if (threadId > 0 && result.back() > start) {
            matchesBegin = next(matchesBegin);
        }

        result.insert(result.end(), matchesBegin, localMatches.end());

        ++activeThread;
        lock.unlock();
        cv.notify_all();
    };

    size_t chunkSize = text.length() / m_NumThreads;
    for (size_t i = 0; i < m_NumThreads; ++i) {
        PCRE2_SIZE start = i * chunkSize;
        const PCRE2_SIZE end = (i == m_NumThreads - 1) ? text.length() : (i + 1) * chunkSize;
        threads.emplace_back(processChunk, i, start, end);
    }

    for (auto& t : threads) {
        t.join();
    }

    return result;
}

vector<unsigned int> BPE::EncodeChunk(const string& chunk, bool cache){
    if(cache){
        auto cacheIter = m_Cache.find(chunk);
        if(cacheIter != m_Cache.end()){
            /* Chunk present in the cache */
            return cacheIter->second;
        }
    }

    /* Vector of unicode bytes (total bytes = 255) */
    vector<unsigned int> tokens((unsigned char*)&*chunk.begin(), (unsigned char*)&*chunk.end());

    unsigned int startSearchIdx = 0;
    unsigned int pairIdx;
    while((pairIdx = FindFirstPair(tokens, startSearchIdx)) != -1){
        startSearchIdx = pairIdx+1;
        Merge(tokens, pairIdx+256);
    }

    if(cache){
        m_Cache.insert({chunk, tokens});
    }

    return tokens;
}

vector<unsigned int> BPE::Encode(const string& text){
    vector<unsigned int> tokens;

    PCRE2_SIZE prevMatch = 0;
    for(PCRE2_SIZE match : SplitText(text)){
        vector<unsigned int> encoded = EncodeChunk(text.substr(prevMatch, match - prevMatch));
        /* Append new tokens to tokens */
        tokens.insert(tokens.end(), encoded.begin(), encoded.end());
        prevMatch = match;
    }

    return tokens;
}

string BPE::Decode(const vector<unsigned int>& tokens) const{
    string result = "";
    for(size_t i = 0; i < tokens.size(); ++i){
        result.append(m_Vocab[tokens[i]]);
    }
    return result;
}

vector<vector<unsigned int>> BPE::FileToTokenBuffer(const string& dataPath) const{
    cout << "Loading file to memory..." << endl;

    vector<vector<unsigned int>> tokenBuffer;
    string text = ReadFile(dataPath);
    vector<PCRE2_SIZE> matches = SplitText(text);
    cout << "Split text" << endl;
    tokenBuffer.reserve(matches.size());

    const unsigned char* textBegin = (unsigned char*)&*text.begin();
    PCRE2_SIZE prevMatch = 0;
    for(const auto& match : matches){
        tokenBuffer.emplace_back(textBegin + prevMatch, textBegin + match);
        prevMatch = match;
    }

    cout << "Done." << endl;

    return tokenBuffer;
}

Pair BPE::GetMostFrequentPair(const vector<vector<unsigned int>>& tokenBuffer) const {
    unordered_map<Pair, size_t> pairFrequency;
    mutex threadFrequencyMutex;
    vector<thread> threads;

    Pair mostFrequentPair{0, 0};
    size_t maxFrequency = 1;

    auto processChunk = [&](const size_t start, const size_t end) {
        unordered_map<Pair, size_t> localFrequency;
        for (size_t i = start; i < end; ++i) {
            Pair prevPair{0, 0};
            for (size_t j = 0; j < tokenBuffer[i].size() - 1; ++j) {
                Pair pair{tokenBuffer[i][j], tokenBuffer[i][j+1]};
                if(pair != prevPair){
                    ++localFrequency[pair];
                    prevPair = pair;
                } else {
                    prevPair = {0, 0};
                }
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

    size_t chunkSize = tokenBuffer.size() / m_NumThreads;
    for (size_t i = 0; i < m_NumThreads; ++i) {
        const size_t start = i * chunkSize;
        size_t end = (i == m_NumThreads - 1) ? tokenBuffer.size() : (i + 1) * chunkSize;
        threads.emplace_back(processChunk, start, end);
    }

    for (auto& t : threads) {
        t.join();
    }

    return mostFrequentPair;
}

void BPE::MergeBuffer(vector<vector<unsigned int>>& tokenBuffer, unsigned int newToken) const{
    vector<thread> threads;
    mutex resultMutex;

    auto processChunk = [&](const size_t start, const size_t end){
        for(size_t i = start; i < end; ++i){
            Merge(tokenBuffer[i], newToken);
        }
    };

    size_t chunkSize = tokenBuffer.size() / m_NumThreads;
    for (size_t i = 0; i < m_NumThreads; ++i) {
        const size_t start = i * chunkSize;
        const size_t end = (i == m_NumThreads - 1) ? tokenBuffer.size() : (i + 1) * chunkSize;
        threads.emplace_back(processChunk, start, end);
    }

    for (auto& t : threads) {
        t.join();
    }
}

void BPE::Fit(const size_t vocabSize, const string& dataPath){
    m_Cache.clear();
    m_VocabSize = vocabSize;
    vector<vector<unsigned int>> tokenBuffer = FileToTokenBuffer(dataPath);

    m_Pairs.reserve(m_VocabSize - 256);

    for(unsigned int i = 256; i < m_VocabSize; ++i){
        Pair pair = GetMostFrequentPair(tokenBuffer);

        if(pair.idx1 == 0 && pair.idx2 == 0){
            /* No more tokens to merge */
            cout << "No more merges... Returning..." << endl;
            m_VocabSize = m_Pairs.size() + 256;
            break;
        }

        m_Pairs.push_back(pair);

        MergeBuffer(tokenBuffer, i);

        if(i % 100 == 0){
            cout << "Reached token " << to_string(i) << endl;
        }
    }

    BuildVocab();
}

void BPE::Save(const string& path) const{
    cout << "Saving..." << endl;
    ofstream file;
    file.open(path);

    file << m_RegexString << "\n";
    file << to_string(m_VocabSize) << "\n";

    for(Pair pair : m_Pairs){
        file << to_string(pair.idx1) << " "
            << to_string(pair.idx2) << "\n";
    }

    file.close();
    cout << "Saved." << endl;
}
