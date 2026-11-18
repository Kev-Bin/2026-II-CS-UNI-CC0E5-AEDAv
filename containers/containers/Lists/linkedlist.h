#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <initializer_list>
#include <iostream>
#include <mutex>
#include "../../GeneralNode.h"
#include "../../Iterator.h"
#include "../../../foreach.h"
#include "../../List.h"

template <typename T>
class LinkedListNode : public GeneralNode<T> {

public:

    using Node      = LinkedListNode<T>;

    Node* m_pNext = nullptr;
    
    LinkedListNode() : GeneralNode<T>(T{}, Ref{}), m_pNext(nullptr) {}
    LinkedListNode(const T& value, Ref ref, Node* pNext) : GeneralNode<T>(value, ref), m_pNext(pNext) {}
};

template <typename Node, bool IsConst = false>
class LinkedListForwardIterator : public Iterator<Node,IsConst> {
public:
    using MySelf = LinkedListForwardIterator<Node,IsConst>;
    using Parent = Iterator<Node,IsConst>;
    using Parent::Parent;
    using Parent::set;

    MySelf& operator++() override {
        set((*this)->m_pNext);
        return *this;
    }
};

template <typename CompareTraits>
struct LinkedListTraits {
    using value_type            = typename CompareTraits::value_type;
    using Compare               = typename CompareTraits::Compare;
    using Node                  = LinkedListNode<value_type>;
    using ForwardIterator       = LinkedListForwardIterator<Node>;
    using ConstForwardIterator  = LinkedListForwardIterator<Node,true>;
};

template <typename T> struct LinkedListAscTraits : public LinkedListTraits<AscendingTraits<T>> {};
template <typename T> struct LinkedListDescTraits : public LinkedListTraits<DescendingTraits<T>> {};


template <typename Traits>
class LinkedList : 
    public List<Traits> {

public:

    using Parent                = List<Traits>;
    
    using value_type            = typename Parent::value_type;
    using Node                  = typename Parent::Node;
    using Delim                 = typename Parent::Delim;
    
    using Parent::Parent;

protected:
    using Parent::m_comp;
    using Parent::m_size;
    using Parent::m_pRoot;
    using Parent::m_pTail;
    using Parent::sentinel;

    void internalPushBack(const value_type& value, Ref ref) override { internalPushBack(new Node(value, ref, &sentinel)); }

    virtual void internalPushBack(Node* nodePtr) {
        nodePtr->m_pNext = &sentinel;
        if (m_size == 0) {
            m_pRoot = m_pTail = nodePtr;
        } else {
            m_pTail->m_pNext = nodePtr;
            m_pTail = nodePtr;
        }
        ++m_size;
    }

    void internalInsert(const value_type& value, Ref ref) { internalInsert(new Node(value, ref, nullptr)); }

    virtual void internalInsert(Node* nodePtr) {
        if (m_size == 0) 
            return internalPushBack(nodePtr);

        Node** currentPtr = &m_pRoot;
        while (*currentPtr != &sentinel) {
            if(m_comp(nodePtr->getValue(), (*currentPtr)->getValue())) {
                nodePtr->m_pNext = *currentPtr;
                *currentPtr = nodePtr;
                ++m_size;
                return;
            }
            currentPtr = &(*currentPtr)->m_pNext;
        } // bucle mejorable
        
        internalPushBack(nodePtr);
    }

    void connect(Node* prev, Node* next) override {
        if (prev == nullptr) {
            m_pRoot = next;
            return;
        }

        prev->m_pNext = next;
    }

public:
    
    LinkedList() = default;
    
    LinkedList(initializer_list<pair<value_type, Ref>> values) : LinkedList() {
        for (const auto& value : values) 
            this->pushBack(value.first, value.second);
    }

    ~LinkedList() override { this->clear(); }
};
#endif // __LINKEDLIST_H__
