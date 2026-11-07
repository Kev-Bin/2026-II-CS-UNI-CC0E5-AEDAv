#ifndef __HEAP_H__
#define __HEAP_H__

#include <mutex>
#include "vector.h"
#include "../types.h"

template <typename CompareTraits>
struct HeapTraits {
    using value_type            = typename CompareTraits::value_type;
    using Compare               = typename CompareTraits::Compare;
    using Node                  = GeneralNode<value_type>;
    using ForwardIterator       = VectorForwardIterator<Node>;
    using ConstForwardIterator  = VectorForwardIterator<Node,true>;
};

template <typename T>
struct HeapAscTraits : 
    public HeapTraits<AscendingTraits<T>> {};

template <typename T>
struct HeapDescTraits : 
    public HeapTraits<DescendingTraits<T>> {};


template <typename Traits>
class Heap : 
    public Container<Traits, Heap<Traits>> {

public:

    using Parent                = Container<Traits, Heap<Traits>>;
    
    using value_type            = Parent::value_type;
    using Compare               = Parent::Compare;    
    using Node                  = Parent::Node;
    using ForwardIterator       = Parent::ForwardIterator;
    using ConstForwardIterator  = Parent::ConstForwardIterator;
    using Parent::Parent;

protected:

    using Parent::m_mutex;
    using Parent::m_comp;
    
    size_t m_size = 0;

    Vector<value_type> m_data;

    void internalClear() override {
        m_data.clear();
        m_size = 0;
    }
        
    void internalPush(const value_type &value, Ref ref) { internalPush(new Node(value, ref)); }

    void internalPush(Node* nodePtr) {
        m_data.pushBack(nodePtr->getValue(),nodePtr->getRef());
        heapifyUp(m_size);
        ++m_size;
    }

    void heapifyUp(size_t index) {
        if (index >= m_size) 
            return;

        auto insertedValue = m_data[index].getValue();
        
        while (index > 0) {
            size_t pIndex = (index - 1) / 2;
            auto pValue = m_data[pIndex].getValue();

            if (!m_comp(insertedValue, pValue)) 
                break;
            
            std::swap(m_data[index], m_data[pIndex]);
            index = pIndex;
        }
    }

    void heapifyDown(size_t index) {
        if (index >= m_size) 
            return;
        
        auto insertedValue = m_data[index].getValue();

        while (hasLeftChild(index)) {
            Node left = getLeftChild(index);
            size_t bestIndex = index * 2 + 1;

            if (hasRightChild(index)) {
                Node right = getRightChild(index);

                if (m_comp(right.getValue(), left.getValue())) 
                    ++bestIndex;
            }

            if (m_comp(insertedValue, m_data[bestIndex].getValue())) 
                break;

            std::swap(m_data[index], m_data[bestIndex]);
            index = bestIndex;
        }
    }

    bool hasLeftChild(size_t index) const { return index * 2 + 1 < m_size; }
    bool hasRightChild(size_t index) const { return index * 2 + 2 < m_size; }
    
    Node getLeftChild(size_t index) const { return m_data[index * 2 + 1]; }
    Node getRightChild(size_t index) const { return m_data[index * 2 + 2]; }

    Heap& copy(const Heap& another) {
        if (this == &another) 
            return *this;
        
        m_size = another.m_size;
        m_comp = another.m_comp;
        m_data = another.m_data;
        
        return *this;
    }

public:

    ForwardIterator      begin()    override       { return m_data.begin(); }
    ForwardIterator      end()      override       { return m_data.end(); }
    ConstForwardIterator cbegin()   const override { return m_data.cbegin(); }
    ConstForwardIterator cend()     const override { return m_data.cend(); }

    bool empty () const override { return m_size == 0; }

    void push(const value_type &value, Ref ref) override {
        scoped_lock lock(m_mutex);
        internalPush(value, ref);
    }
    
    Node pop() {
        scoped_lock lock(m_mutex);

        if (m_size == 0) 
            throw std::out_of_range("Heap vacio");
        
        std::swap(m_data[0], m_data[m_size - 1]);
        Node ret = m_data.pop_back();
        --m_size;
        heapifyDown(0);
        
        return ret;
    }

    template <typename Func, typename... Args>
    void ApplyFunction(Func func, Args... args) {
        scoped_lock lock (m_mutex);
        this->call(func, std::forward<Args>(args)...);

        for (size_t i = m_size / 2; i > 0; --i) 
            heapifyDown(i - 1);
    }

    Heap& operator=(const Heap& another) { 
        scoped_lock lock(m_mutex);
        return copy(another); 
    }

};

template <typename T> class MinHeap : public Heap<HeapAscTraits<T>> {};
template <typename T> class MaxHeap : public Heap<HeapDescTraits<T>> {};

#endif // __HEAP_H__