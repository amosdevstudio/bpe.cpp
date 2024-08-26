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

#include "datastructures.hpp"
#include <string>

class BPE{
private:
    std::vector<TokenPair> m_Merges;
    std::vector<std::string> m_Vocab;
    size_t m_VocabSize;
    std::unordered_set<uint32_t> m_SplitLetters;
    std::string m_SplitLettersString;

    void StringToTokens(const std::string& data, TokenList& tokens) const;
    void BuildVocab();
public:
    void LoadSplitLetters(const std::string& splitLetters);
    void Load(const std::string& path);
    TokenList Encode(const std::string& text) const;
    std::vector<uint32_t> EncodeToVector(const std::string& text) const;
    std::string Decode(const TokenList& tokens) const;
    std::string DecodeFromVector(const std::vector<uint32_t>& tokens) const;
    void Fit(const size_t vocabSize, const std::string& path);
    void Save(const std::string& path) const;
};

#endif
