#ifndef __SEQUENCE_H__
#define __SEQUENCE_H__

#include "Container.h"

template <typename Traits, typename Derived>
class Sequence : 
    public Container<Traits, Derived> {

public:

    using Parent                = Container<Traits, Derived>;

    using value_type            = typename Parent::value_type;
    using Node                  = typename Parent::Node;
    using Delim                 = typename Parent::Delim;
    using ForwardIterator       = typename Parent::ForwardIterator;
    using ConstForwardIterator  = typename Parent::ConstForwardIterator;

    using Parent::Parent;

protected:

    using Parent::m_mutex;
    using Parent::m_comp;
    
    size_t m_size = 0;

    virtual void internalPushBack(const value_type&, Ref)   = 0;
    virtual void internalPushBack(Node*)                    = 0;
    
    Sequence& internalCopy(const Derived& another) override {
        if(this != &another) { 
            Parent::copy(another);
            
            another.cApplyFunction([this](const Node& node) {
                internalPushBack(node.getValue(), node.getRef());
            });
        }

        return *this;
    }

public:

    const size_t size() const                       { return m_size; }
    
    bool empty() const  override                    { return m_size == 0; }
    
    void pushBack(const value_type& value, Ref ref) { 
        std::scoped_lock guard(m_mutex);
        internalPushBack(value, ref);
    }
};

#endif // __SEQUENCE_H__