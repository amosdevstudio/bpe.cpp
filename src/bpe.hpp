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


#ifndef BPE_HPP
#define BPE_HPP

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct Pair{
    unsigned int idx1;
    unsigned int idx2;

    bool operator==(const Pair& b) const{return idx1 == b.idx1 && idx2 == b.idx2; };
};

namespace std {
    template<>
    struct hash<Pair> {
        inline size_t operator()(const Pair& p) const {
            /* Cantor's pairing function */
            size_t x = (size_t)p.idx1, y = (size_t)p.idx2;
            return ((x + y) * (x + y + 1) >> 1) + y;
        }
    };
}

class BPE{
private:
    std::vector<Pair> m_Pairs;
    std::unordered_map<Pair, unsigned int> m_PairIndexMap;
    std::vector<std::string> m_Vocab;
    size_t m_VocabSize;
    pcre2_code* m_RegexPattern;
    std::string m_RegexString;
    std::unordered_map<std::string, std::vector<unsigned int>> m_Cache;

    unsigned int FindFirstPair(const std::vector<unsigned int>& tokens, const unsigned int& startIdx);
    void Merge(std::vector<unsigned int>& tokens, const unsigned int& newToken) const;
    std::vector<unsigned int> EncodeChunk(const std::string& chunk, bool cache=true);
    void BuildVocab();
    std::vector<std::vector<unsigned int>> FileToTokenBuffer(const std::string& dataPath);
    Pair GetMostFrequentPair(const std::vector<std::vector<unsigned int>>& tokenBuffer, const size_t numThreads) const;

public:
    BPE();
    ~BPE();

    void LoadRegex(const std::string& regexText);
    void Load(const std::string& path);
    std::vector<unsigned int> Encode(const std::string& text);
    std::string Decode(const std::vector<unsigned int>& tokens);
    void Fit(const size_t vocabSize, const std::string& dataPath, const size_t numThreads);
    void Save(const std::string& path);
};

#endif
