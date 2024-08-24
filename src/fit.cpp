#include "bpe.hpp"
#include <chrono>
#include <iostream>

using namespace std;

int main(){
    string splitLetters,
    filePath,
    vocabSizeString,
    numThreadsString;

    int vocabSize;

    cout << "Type in (or paste in) the letters used to split the words:" << endl;
    getline(cin, splitLetters);
    cout << endl
    << "Type in (or paste in) the path to the text file for fitting:" << endl;
    getline(cin, filePath);
    cout << endl
    << "Type in (or paste in) the vocab size:" << endl;
    getline(cin, vocabSizeString);

    vocabSize = stoi(vocabSizeString);

    cout << endl
        << "Regex: " << splitLetters << endl
        << "File path: " << filePath << endl
        << "Vocab size: " << to_string(vocabSize) << endl;

    cout << "Continue(y/N)? ";
    string continueTraining;
    getline(cin, continueTraining);

    if(continueTraining != "y" && continueTraining != "Y"){
        cout << "Not continuing." << endl;
        return -1;
    }

    cout << "Continuing" << endl;

    BPE bpe;

    auto start = chrono::high_resolution_clock::now();

    bpe.LoadSplitLetters(splitLetters);
    bpe.Fit(vocabSize,
            filePath
            );
    auto duration = chrono::duration_cast<chrono::milliseconds>(
        chrono::high_resolution_clock::now() -
        start
    );
    cout << duration.count() << "ms" << endl;

    bpe.Save("tokenizer.bpe");
}
