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
    TokenList Encode(const std::string& text);
    std::string Decode(const std::vector<uint32_t>& tokens) const;
    void Fit(const size_t vocabSize, const std::string& path);
    void Save(const std::string& path) const;
};
