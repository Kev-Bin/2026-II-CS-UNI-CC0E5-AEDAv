#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <utility>
#include <mutex>
#include "Iterator.h"
#include "../foreach.h"
#include "../types.h"

template <typename T, typename CompareT> 
struct CompareTraits {
    using value_type = T;
    using Compare    = CompareT;
};

template <typename T> 
struct AscendingTraits : public CompareTraits<T, std::less<T>> {};

template <typename T> 
struct DescendingTraits : public CompareTraits<T, std::greater<T>> {};

template <typename Traits, typename Derived>
class Container;

template <typename Traits, typename Derived>
std::ostream& operator<<(std::ostream&, const Container<Traits, Derived>&);

template <typename Traits, typename Derived>
std::istream& operator>>(std::istream&, Container<Traits, Derived>&);

template <typename Traits, typename Derived = void>
class Container : 
    public ForwardIterable<Traits> {

public:

    using value_type            = typename Traits::value_type;
    using Node                  = typename Traits::Node;
    using Compare               = typename Traits::Compare;
    using Delim                 = typename Node::Delim;
    
    using ForwardIterator       = typename ForwardIterable<Traits>::ForwardIterator;
    using ConstForwardIterator  = typename ForwardIterable<Traits>::ConstForwardIterator;

protected:

    mutable std::mutex  m_mutex;
    Compare m_comp;

    virtual void internalPush(const value_type&, Ref)   = 0;
    virtual void internalRemove(const value_type&, Ref) = 0;
    virtual void internalClear()                        = 0;

    virtual Container& internalCopy(const Derived& another) {
        if (this == &another) 
            return *this;
        
        internalClear();

        m_comp = another.getComp();
        
        return *this;
    }

public:

    Container()                 = default;
    virtual ~Container()        = default;
    virtual bool empty() const  = 0;

    Compare getComp() const { return m_comp; }
    
    virtual void push(const value_type& value, Ref ref) {
        scoped_lock lock(m_mutex);
        internalPush(value, ref); 
    }

    virtual void remove(const value_type& value, Ref ref) {
        scoped_lock lock(m_mutex);
        internalRemove(value, ref); 
    }

    void clear() { 
        scoped_lock lock(m_mutex);
        internalClear(); 
    }
    
    std::ostream&   write(std::ostream& os)   { return os << *this; }
    std::istream&   read(std::istream& is)    { return is >> *this; }
    Container&      copy(const Derived& other){ return *this = other; }

    friend std::ostream& operator<< <Traits, Derived>(std::ostream&, const Container<Traits, Derived>&);
    friend std::istream& operator>> <Traits, Derived>(std::istream&, Container<Traits, Derived>&);
    
    Container& operator=(const Derived& another) {
        scoped_lock lock(m_mutex); 
        return internalCopy(another);
    }
};
#endif // __CONTAINER_H__