#ifndef __CIRCULARLINKEDLIST_H__
#define __CIRCULARLINKEDLIST_H__
#include "linkedlist.h"

template <typename Traits>
class CircularLinkedList : 
    public LinkedList<Traits> {

public:

    using Parent = LinkedList<Traits>;

    using Node   = typename Parent::Node;

protected:

    using Parent::m_pRoot;
    using Parent::sentinel;
    
    void internalPushBack(Node* newNode) override {
        Parent::internalPushBack(newNode);
        if(m_pRoot) 
            sentinel.m_pNext = m_pRoot;
    }

    void internalInsert(Node* newNode) override {
        Parent::internalInsert(newNode);
        sentinel.m_pNext = m_pRoot;
    }

    void internalClear() override {
        Parent::internalClear();
        sentinel.m_pNext = nullptr;
    };

    void connect(Node* prev, Node* next) override {
        Parent::connect(prev, next);
        if(prev == nullptr)
            sentinel.m_pNext = next;
    }
};
#endif // __CIRCULARLINKEDLIST_H__
