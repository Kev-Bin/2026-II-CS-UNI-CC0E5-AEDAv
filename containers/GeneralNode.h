#ifndef __GENERALNODE_H__
#define __GENERALNODE_H__
#include <iostream>
#include "../types.h" // Ref

template <typename T>
class GeneralNode;

template <typename T>
std::ostream& operator<<(std::ostream&, const GeneralNode<T>&);

template <typename T>
std::istream& operator>>(std::istream&, GeneralNode<T>&);

template <typename T>
class GeneralNode{

public:

    using value_type    = T;
    using Delim         = std::istream::char_type;
    
private:

    value_type  m_value;
    Ref         m_ref;

public:

    GeneralNode()   = default; // requerido por resize(): new Node[new_cap]
    GeneralNode(const T& value, Ref ref) : m_value(value), m_ref(ref) {}
    GeneralNode(const GeneralNode& another) : m_value(another.m_value), m_ref(another.m_ref) {}
    ~GeneralNode()  = default;

    value_type&          getValue() { return m_value; }
    const value_type&    getValue() const { return m_value; }
    Ref                  getRef()   const { return m_ref;   }

    friend std::ostream& operator<< <T> (std::ostream &, const GeneralNode<T>&);
    friend std::istream& operator>> <T> (std::istream &, GeneralNode<T>&);
};

#endif // __GENERALNODE_H__