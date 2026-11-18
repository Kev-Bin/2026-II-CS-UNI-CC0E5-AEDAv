#ifndef __LIST_H__
#define __LIST_H__

#include "Sequence.h"

template <typename Traits>
class List : 
    public Sequence<Traits, List<Traits>> {

public:

    using Parent                = Sequence<Traits, List<Traits>>;

    using value_type            = typename Parent::value_type;
    using Node                  = typename Parent::Node;
    using Delim                 = typename Parent::Delim;
    using ForwardIterator       = typename Parent::ForwardIterator;
    using ConstForwardIterator  = typename Parent::ConstForwardIterator;

    using Parent::Parent;

protected:
    
    using Parent::copy;

    using Parent::m_comp;
    using Parent::m_mutex;
    using Parent::m_size;

    Node* m_pRoot = nullptr; 
    Node* m_pTail = nullptr;
    Node sentinel;

    virtual void internalInsert(Node* newNode)      = 0;
    virtual void connect(Node* prev, Node* next)    = 0;

    void internalInsert(const value_type& value, Ref ref) { internalInsert(new Node(value, ref, nullptr)); }

    void internalPush(const value_type& value, Ref ref) override { this->internalPushBack(value, ref); }
    
    void internalRemove(const value_type& value, Ref ref) override {
        ForwardIterator it = begin();
        Node* previous  = nullptr;
        
        Node& toDelete  = this->FirstThat([&](Node& node) {
            if(node.getValue() == value && node.getRef() == ref)
                return true;
            else
                previous = &*it; //aplicar operator* y referenciar
            ++it;

            return false;
        });

        Node* next = &*(++it);
        
        connect(previous, next);
        delete &toDelete;
        --m_size;
    }

    void internalClear() override {
        this->ApplyFunction([](Node& node) { delete &node; });
        
        m_pRoot = nullptr;
        m_pTail = nullptr;
        m_size  = 0;
    }

public:

    void insert(const value_type& value, Ref ref) {
        scoped_lock lock(m_mutex);
        internalInsert(value, ref); 
    }

    ForwardIterator      begin()  override          { return ForwardIterator(m_pRoot ? m_pRoot : &sentinel); }
    ForwardIterator      end()    override          { return ForwardIterator(&sentinel); }
    ConstForwardIterator cbegin() const override    { return ConstForwardIterator(m_pRoot ? m_pRoot : &sentinel); }
    ConstForwardIterator cend()   const override    { return ConstForwardIterator(&sentinel); }

};

#endif // __LIST_H__