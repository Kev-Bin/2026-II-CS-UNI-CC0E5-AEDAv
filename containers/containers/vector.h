#ifndef __VECTOR_H__
#define __VECTOR_H__
#include <initializer_list>
#include <mutex>
#include "Iterator.h"
#include "GeneralNode.h"
#include "../../foreach.h"
#include "../../types.h" // Ref
#include "Sequence.h"

using namespace std;

template <typename Node, bool IsConst = false>
class VectorForwardIterator : 
    public Iterator<Node,IsConst> {
    
public:
    using MySelf            = VectorForwardIterator<Node,IsConst>;
    using Parent            = Iterator<Node,IsConst>;
    using Parent::Parent;

    MySelf& operator++() { 
        ++Parent::m_ptr; 
        return *this; 
    }
};


template <typename Node, bool IsConst = false>
class VectorBackwardIterator : 
    public Iterator<Node,IsConst> {

public:
    using MySelf            = VectorBackwardIterator<Node,IsConst>;
    using Parent            = Iterator<Node,IsConst>;
    using Parent::Parent;

    MySelf& operator++() { 
        --Parent::m_ptr; 
        return *this; 
    }
};

template <typename CompareTraits>
struct VectorTraits {
    using value_type            = typename CompareTraits::value_type;
    using Compare               = typename CompareTraits::Compare;
    using Node                  = GeneralNode<value_type>;
    using ForwardIterator       = VectorForwardIterator<Node>;  // itera sobre Node, no sobre T
    using ConstForwardIterator  = VectorForwardIterator<Node,true>;
    using BackwardIterator      = VectorBackwardIterator<Node>; // itera sobre Node, no sobre T
    using ConstBackwardIterator = VectorBackwardIterator<Node,true>;
};

template <typename T>
struct VectorAscTraits : public VectorTraits<AscendingTraits<T>> {};

template <typename T>
struct VectorDescTraits : public VectorTraits<DescendingTraits<T>> {};

template <typename T, typename Traits = VectorAscTraits<T>>
class Vector : 
    public Sequence<Traits, Vector<T, Traits>>, 
    public BackwardIterable<Traits> {

public:
    using Parent                = Sequence<Traits, Vector<T, Traits>>;
    
    using value_type            = typename Parent::value_type;
    using Compare               = typename Parent::Compare;    
    using Node                  = typename Parent::Node;
    using Delim                 = typename Parent::Delim;
    using ForwardIterator       = typename Parent::ForwardIterator;
    using ConstForwardIterator  = typename Parent::ConstForwardIterator;
    
    using BackwardIterator      = typename Traits::BackwardIterator;
    using ConstBackwardIterator = typename Traits::ConstBackwardIterator;
    
    using Parent::Parent;

protected:
    using Parent::m_mutex;
    using Parent::m_comp;
    using Parent::m_size;
    
    Node*   m_data      = nullptr;
    size_t  m_capacity  = 0;

public:

    Vector() = default;

    Vector(initializer_list<pair<value_type, Ref>> values) {
        for (const auto& value : values)
            this->pushBack(value.first, value.second);
    }

    Vector(const Vector& other) {
        resize(other.m_size);
        for (size_t i = 0; i < other.m_size; ++i)
            m_data[i] = other.m_data[i];
        m_size = other.m_size;
    }

    Vector(Vector&& other) noexcept {
        m_data     = std::exchange(other.m_data, nullptr);
        m_size     = std::exchange(other.m_size, 0);
        m_capacity = std::exchange(other.m_capacity, 0);
    }

    ~Vector() override { delete[] m_data; }

protected:

    void resize(size_t new_cap) {
        if (new_cap <= m_capacity) 
            return;
        
        Node* new_data = new Node[new_cap];

        for (size_t i = 0; i < m_size; ++i) 
            new_data[i] = m_data[i];
            
        delete[] m_data;

        m_data = new_data;
        m_capacity = new_cap;
    }

    void internalClear() override {
        delete [] m_data;

        m_data      = nullptr;
        m_size      = 0;
        m_capacity  = 0;
    }

    void internalPushBack(const value_type& value, Ref ref) override { internalPushBack(new Node(value, ref)); }
    
    void internalPushBack(Node* nodePtr) override {
        if (m_size == m_capacity)
            resize((m_capacity == 0) ? 10 : m_capacity * 2);

        m_data[m_size] = *nodePtr;
        ++m_size;
    }

    Parent& copy(const Vector& another) override {
        if (this != &another) {
            Parent::copy(another);
            m_capacity = another.m_capacity;
        }

        return *this;
    }

public:

    const Node& Peek() const {
        lock_guard<mutex> lock(m_mutex);

        if (m_size == 0)
            throw out_of_range("Vector vacio");

        return m_data[m_size - 1];
    }

    Node pop_back() {
        lock_guard<mutex> lock(m_mutex);

        if (m_size == 0) 
            throw std::out_of_range("Vector vacio");

        Node ret = m_data[m_size-1];
        --m_size;
        return ret;
    }

    Node& at(size_t index) {
        if (index >= m_size) 
            throw std::out_of_range("Indice fuera de rango");

        return m_data[index];
    }

    Node& operator[](size_t index) {
        if (index >= m_size) 
            throw std::out_of_range("Indice fuera de rango");
            
        return m_data[index];
    }

    Node& operator[] (size_t index) const {
        if (index >= m_size) 
            throw std::out_of_range("Indice fuera de rango");

        return m_data[index];
    }

    Vector& operator=(const Vector& another) {
        scoped_lock lock(m_mutex); 
        copy(another); 
        return *this;
    }

    ForwardIterator       begin()   override        { return ForwardIterator(m_data); }
    ForwardIterator       end()     override        { return ForwardIterator(m_data + m_size); }
    ConstForwardIterator  cbegin()  const override  { return ConstForwardIterator(m_data); }
    ConstForwardIterator  cend()    const override  { return ConstForwardIterator(m_data + m_size); }
    BackwardIterator      rbegin()  override        { return BackwardIterator(m_data + m_size - 1); }
    BackwardIterator      rend()    override        { return BackwardIterator(m_data - 1); }
    ConstBackwardIterator crbegin() const override  { return ConstBackwardIterator(m_data + m_size - 1); }
    ConstBackwardIterator crend()   const override  { return ConstBackwardIterator(m_data - 1); }
};

template <typename T>
class AscVector : 
    public Vector<VectorAscTraits<T>> {};

template <typename T>
class DescVector : 
    public Vector<VectorDescTraits<T>> {};

#endif // __VECTOR_H__