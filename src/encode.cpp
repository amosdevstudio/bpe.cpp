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
#include <iostream>

using namespace std;

int main(){
    BPE bpe;
    bpe.Load("tokenizer.bpe");
    while(true){
        cout << "Text:" << endl;
        string in;
        getline(cin, in);


        TokenList encoded = bpe.Encode(in);
        vector<string> decodedList;

        TokenNode* token = encoded.head();
        while(token != nullptr){
            cout << to_string(token->val) << " ";
            decodedList.push_back(bpe.Decode(TokenList(token->val)));
            token = token->next;
        }
        cout << endl << "[";

        for(const auto& decoded : decodedList){
            cout << "\"" << decoded << "\"" << ", ";
        }
        cout << "]" << endl;

        string decoded = bpe.Decode(encoded);
        cout << decoded << endl;

        encoded.DeleteContents();
    }
}

