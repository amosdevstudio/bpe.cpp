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

#ifndef DATASTRUCTURES_HPP
#define DATASTRUCTURES_HPP

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct TokenPair{
    uint32_t token1, token2;

    inline bool operator==(const TokenPair& b) const { return token1==b.token1 && token2==b.token2; }
    inline bool operator!=(const TokenPair& b) const { return token1!=b.token1 || token2!=b.token2; }
};

template<>
struct std::hash<TokenPair> {
    inline uint64_t operator()(const TokenPair& p) const {
        return *(uint64_t*)(&p);
    }
};

struct TokenNode{
    uint32_t val;
    TokenNode* prev;
    TokenNode* next;
};

class TokenList{
private:
    size_t m_Size;
    TokenNode* m_Head;
    TokenNode* m_Tail;

public:
    std::vector<TokenNode*> checkpoints;

    inline size_t size() const { return m_Size; }
    inline TokenNode* head() const { return m_Head; }
    inline TokenNode* tail() const { return m_Tail; }

    inline TokenList():m_Tail(nullptr), m_Head(nullptr), m_Size(0){}
    inline TokenList(uint32_t token){
        m_Tail = nullptr;
        m_Head = nullptr;
        m_Size = 0;

        Append(token);
    }

    inline void DeleteContents(){
        while(m_Size > 0){
            PopBack();
        }
    }

    inline void Append(uint32_t val){
        if(m_Size == 0){
            m_Head = new TokenNode{.val=val, .prev=nullptr, .next=nullptr};
            m_Tail = m_Head;
            m_Size = 1;
            return;
        }

        m_Tail->next = new TokenNode{.val=val, .prev=m_Tail, .next=nullptr};
        m_Tail = m_Tail->next;
        ++m_Size;
    }

    inline void AppendList(const TokenList& tokens){
        if(m_Size == 0){
            m_Head = tokens.head();
            m_Tail = tokens.tail();
        } else {
            m_Tail->next = tokens.head();
            m_Tail = tokens.tail();
        }
        m_Size += tokens.size();
    }

    inline void PopFront(){
        if(m_Size == 1){
            delete m_Head;
            m_Head = nullptr;
            m_Tail = nullptr;
            m_Size = 0;
            return;
        }

        assert(m_Size > 0);

        m_Head = m_Head->next;
        delete m_Head->prev;
        m_Head->prev = nullptr;
        --m_Size;
    }

    inline void PopBack(){
        if(m_Size == 1){
            delete m_Tail;
            m_Head = nullptr;
            m_Tail = nullptr;
            m_Size = 0;
            return;
        }

        assert(m_Size > 0);

        m_Tail = m_Tail->prev;
        delete m_Tail->next;
        m_Tail->next = nullptr;
        --m_Size;
    }

    inline void Remove(TokenNode* token){
        if(token == m_Head){
            PopFront();
        } else if (token == m_Tail){
            PopBack();
        } else {
            assert(m_Size > 0);
            token->prev->next = token->next;
            token->next->prev = token->prev;
            delete token;
            --m_Size;
        }
    }
};

class HeapNode{
private:
    TokenPair m_Pair;
    size_t m_Idx;
public:
    std::unordered_set<TokenNode*> positions;

    inline HeapNode(const TokenPair& pair, size_t idx):m_Pair(pair), m_Idx(idx){};

    inline size_t idx(){ return m_Idx; }
    inline size_t key(){ return positions.size(); }
    inline TokenPair pair(){ return m_Pair; }

    inline void SetIdx(size_t idx){ m_Idx = idx; }

    inline size_t ParentIdx(){
        return (m_Idx-1) >> 1;
    }

    inline size_t LeftChildIdx(){
        return (m_Idx << 1) + 1;
    }

    inline size_t RightChildIdx(){
        return (m_Idx << 1) + 2;
    }

    inline void AddPosition(TokenNode* position){
        positions.insert(position);
    }

    inline void RemovePosition(TokenNode* position){
        positions.erase(position);
    }

    inline static void SwapIndices(HeapNode* node1, HeapNode* node2){
        auto temp = node1->m_Idx;
        node1->m_Idx = node2->m_Idx;
        node2->m_Idx = temp;
    }
};

class Heap{
private:
    std::vector<HeapNode*> m_Nodes;
    std::unordered_map<TokenPair, HeapNode*> m_PairMap;

    inline void Swap(HeapNode* node1, HeapNode* node2){
        m_Nodes[node1->idx()] = node2;
        m_Nodes[node2->idx()] = node1;
        HeapNode::SwapIndices(node1, node2);
    }

    inline void HeapifyUp(HeapNode* node){
        while (node->idx() > 0) {
            HeapNode* parent = m_Nodes[node->ParentIdx()];
            if (parent->key() >= node->key()) {
                break;
            }
            Swap(parent, node);
        }
    }

    inline void HeapifyDown(HeapNode* node){
        size_t leftChildIdx = node->LeftChildIdx();
        size_t rightChildIdx = node->RightChildIdx();

        while(leftChildIdx < size()){
            HeapNode* biggestChild = m_Nodes[leftChildIdx];
            if(rightChildIdx < size() && m_Nodes[rightChildIdx]->key() > biggestChild->key() ){
                biggestChild = m_Nodes[rightChildIdx];
            }

            if(biggestChild->key() > node->key()){
                Swap(biggestChild, node);
                leftChildIdx = node->LeftChildIdx();
                rightChildIdx = node->RightChildIdx();
            } else {
                break;
            }
        }
    }

public:

    inline void DeleteContents(){
        for(auto& node : m_Nodes){
            delete node;
        }
    }

    inline const size_t size() const { return m_Nodes.size(); }
    inline HeapNode* GetNode(const size_t idx) const { return m_Nodes[idx]; }
    inline size_t LastNonLeafIdx() const { return m_Nodes[size()-1]->ParentIdx(); }

    inline void MakeHeap(){
        size_t lastNonLeaf = LastNonLeafIdx();
        for(size_t i = 0; i <= lastNonLeaf; ++i){
            HeapifyDown(m_Nodes[lastNonLeaf-i]);
        };
    }

    inline void AddNodeNoHeapify(HeapNode* node){
        m_Nodes.push_back(node);
        node->SetIdx(size()-1);
        m_PairMap[node->pair()] = node;
    }

    inline void AddNode(HeapNode* node){
        AddNodeNoHeapify(node);
        HeapifyUp(node);
    }

    inline HeapNode* PopTop(){
        HeapNode* top = m_Nodes[0];
        Swap(top, m_Nodes[size()-1]);
        m_Nodes.pop_back();
        HeapifyDown(m_Nodes[0]);
        return top;
    }

    inline void RemoveNode(HeapNode* node){
        m_PairMap.erase(node->pair());
        delete node;
    }

    inline HeapNode* AddPositionNoHeapify(TokenNode* token){
        if(token == nullptr || token->next == nullptr || token->val == 0 || token->next->val == 0){
            return nullptr;
        }

        TokenPair pair{token->val, token->next->val};

        auto iter = m_PairMap.find(pair);
        HeapNode* node;
        if(iter == m_PairMap.end()){
            /* Create new item */
            node = new HeapNode(pair, size());
            node->AddPosition(token);
            AddNodeNoHeapify(node);
        } else{
            /* Add new position */
            node = iter->second;
            node->AddPosition(token);
        }

        return node;
    }

    inline void AddPosition(TokenNode* token){
        HeapNode* node = AddPositionNoHeapify(token);
        if(node != nullptr){
            HeapifyUp(node);
        }
    }

    inline HeapNode* RemovePositionNoHeapify(TokenNode* token){
        if(token == nullptr || token->next == nullptr || token->val == 0 || token->next->val == 0){
            return nullptr;
        }

        TokenPair pair{token->val, token->next->val};

        auto iter = m_PairMap.find(pair);
        if(iter == m_PairMap.end()){
            //Didn't find the pair (no need to remove the position)
            return nullptr;
        }

        HeapNode* node = iter->second;

        node->RemovePosition(token);
        return node;
    }

    inline void RemovePosition(TokenNode* token){
        HeapNode* node = RemovePositionNoHeapify(token);
        if(node != nullptr){
            HeapifyDown(node);
        }
    }

    inline void Truncate(const size_t newSize){
        if(size() <= newSize){
            return;
        }

        for(size_t i = newSize; i < size(); ++i){
            RemoveNode(m_Nodes[i]);
        }

        m_Nodes.resize(newSize);
    }
};

#endif
