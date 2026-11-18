#ifndef __DOUBLYLINKEDLIST_H__
#define __DOUBLYLINKEDLIST_H__

#include "../../GeneralNode.h"
#include "../../Iterator.h"
#include "linkedlist.h"

template <typename T>
class DoublyLinkedListNode : 
    public GeneralNode<T> {

public:

    using MySelf = DoublyLinkedListNode<T>;
    using Parent = GeneralNode<T>;

    MySelf* m_pPrev = nullptr;
    MySelf* m_pNext = nullptr;

    DoublyLinkedListNode() = default;
    DoublyLinkedListNode(const T& value, Ref ref, MySelf* pNext = nullptr) : Parent(value, ref), m_pNext(pNext) {}
};

template <typename T, bool IsConst = false>
class DoublyLinkedListBackwardIterator : 
    public Iterator<DoublyLinkedListNode<T>, IsConst> {

public:

    using MySelf    = DoublyLinkedListBackwardIterator<T,IsConst>;
    using Parent    = Iterator<DoublyLinkedListNode<T>, IsConst>;
    using Node      = Parent::value_type;

    using Parent::Parent;
    using Parent::set;

    MySelf& operator++() {
        set((*this)->m_pPrev);
        return *this;
    }
};

template <typename Traits>
struct DoublyLinkedListTraits {
    using value_type            = typename Traits::value_type;
    using Compare               = typename Traits::Compare;
    using Node                  = DoublyLinkedListNode<value_type>;
    using ForwardIterator       = LinkedListForwardIterator<Node>;
    using ConstForwardIterator  = LinkedListForwardIterator<Node,true>;
    using BackwardIterator      = DoublyLinkedListBackwardIterator<value_type>;
    using ConstBackwardIterator = DoublyLinkedListBackwardIterator<value_type,true>;
};

template <typename T>
struct DoublyLinkedListAscTraits : 
    DoublyLinkedListTraits<AscendingTraits<T>> {};

template <typename T>
struct DoublyLinkedListDescTraits : 
    DoublyLinkedListTraits<DescendingTraits<T>> {};

template <typename Traits>
class DoublyLinkedList : 
    public LinkedList<Traits>, 
    public BackwardIterable<Traits> {

public:

    using Parent                = LinkedList<Traits>;

    using Node                  = typename Parent::Node;
    
    using BackwardIterator      = typename Traits::BackwardIterator;
    using ConstBackwardIterator = typename Traits::ConstBackwardIterator;

protected:

    using Parent::m_pRoot;
    using Parent::m_pTail;
    using Parent::sentinel;

    void internalPushBack(Node* newNode) override {
        Parent::internalPushBack(newNode);
        newNode->m_pPrev = sentinel.m_pPrev;
        sentinel.m_pPrev = newNode;
    }

    void internalInsert(Node* newNode) override {
        Parent::internalInsert(newNode);
        if (m_pTail != newNode) {
            newNode->m_pPrev = newNode->m_pNext->m_pPrev;
            newNode->m_pNext->m_pPrev = newNode;
        }
    }

    void internalClear() override {
        Parent::internalClear();
        sentinel.m_pPrev = nullptr;
    }

    void connect(Node* prev, Node* next) override {
        Parent::connect(prev, next);
        next->m_pPrev = prev; 
    }
public:

    BackwardIterator rbegin() override              { return BackwardIterator(m_pTail); }
    BackwardIterator rend() override                { return BackwardIterator(nullptr); }
    ConstBackwardIterator crbegin() const override  { return ConstBackwardIterator(m_pTail); }
    ConstBackwardIterator crend() const override    { return ConstBackwardIterator(nullptr); }
};

#endif // __DOUBLYLINKEDLIST_H__
