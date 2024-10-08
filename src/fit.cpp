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
        << "Split letters: " << splitLetters << endl
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
