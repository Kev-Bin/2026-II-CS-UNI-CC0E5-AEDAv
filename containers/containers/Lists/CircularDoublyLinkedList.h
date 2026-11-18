#ifndef __CIRCULARDOUBLYLINKEDLIST_H__
#define __CIRCULARDOUBLYLINKEDLIST_H__

#include "DoublyLinkedList.h"

template <typename Traits>
struct CircularDoublyLinkedListTraits : DoublyLinkedListTraits<Traits> {};

template <typename T>
struct CircularDoublyLinkedListAscTraits
    : CircularDoublyLinkedListTraits<AscendingTraits<T>> {};

template <typename T>
struct CircularDoublyLinkedListDescTraits
    : CircularDoublyLinkedListTraits<DescendingTraits<T>> {};

template <typename Traits>
class CircularDoublyLinkedList : 
    public DoublyLinkedList<Traits> {

public:

    using Parent                = DoublyLinkedList<Traits>;

    using Node                  = typename Parent::Node;
    using BackwardIterator      = typename Parent::BackwardIterator;
    using ConstBackwardIterator = typename Parent::ConstBackwardIterator;

protected:
    
    using Parent::m_pRoot;
    using Parent::sentinel;
    
    void internalPushBack(Node* newNode) override {
        Parent::internalPushBack(newNode);
        sentinel.m_pNext = m_pRoot;
        m_pRoot->m_pPrev = &sentinel;
    }

    void internalInsert(Node* newNode) override {
        Parent::internalInsert(newNode);
        sentinel.m_pNext = m_pRoot;
        m_pRoot->m_pPrev = &sentinel;
    }

    void internalClear() override {
        Parent::internalClear();
        sentinel.m_pNext = nullptr;
        sentinel.m_pPrev = nullptr;
    }

    void connect(Node* prev, Node* next) override {
        Parent::connect(prev, next);
        if (prev == nullptr) {
            sentinel.m_pNext    = next;
            next->m_pPrev       = &sentinel;
        }
    }

public:

    BackwardIterator      rbegin()  override        { return BackwardIterator(sentinel.m_pPrev ? sentinel.m_pPrev : &sentinel); }
    BackwardIterator      rend()    override        { return BackwardIterator(&sentinel); }
    ConstBackwardIterator crbegin() const override  { return ConstBackwardIterator(sentinel.m_pPrev ? sentinel.m_pPrev : &sentinel); }
    ConstBackwardIterator crend()   const override  { return ConstBackwardIterator(&sentinel); }
    
    template <typename Func, typename... Args>
    decltype(auto) rcall(Func func, Args&&... args) {
        if constexpr (is_void_v<invoke_result_t<Func, Node&, Args...>>) {
            ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
        } else {
            return ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
        }
    }
};

#endif // __CIRCULARDOUBLYLINKEDLIST_H__
