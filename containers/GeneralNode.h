#ifndef __GENERALNODE_H__
#define __GENERALNODE_H__
#include <iostream>
#include "../types.h" // Ref

template <typename T, typename Reference>
class GeneralNode;

template <typename T, typename Reference>
std::ostream& operator<<(std::ostream&, const GeneralNode<T, Reference>&);

template <typename T, typename Reference>
std::istream& operator>>(std::istream&, GeneralNode<T, Reference>&);

template <typename T, typename Reference = Ref>
class GeneralNode{

public:

    using value_type    = T;
    using Ref           = Reference;
    using Delim         = std::istream::char_type;
    
private:

    value_type  m_value;
    Reference   m_ref;

public:

    GeneralNode()   = default; // requerido por resize(): new Node[new_cap]
    GeneralNode(const T& value, Reference ref) : m_value(value), m_ref(ref) {}
    GeneralNode(const GeneralNode& another) : m_value(another.m_value), m_ref(another.m_ref) {}
    ~GeneralNode()  = default;

    value_type&         getValue()          { return m_value; }
    const value_type&   getValue() const    { return m_value; }
    
    Ref&                getRef()            { return m_ref; }
    const Ref&          getRef()   const    { return m_ref; }

    void                setValue(const value_type& value)   { m_value = value; }
    void                setRef(Ref ref)                     { m_ref = ref; }

    friend std::ostream& operator<< <T, Reference> (std::ostream &, const GeneralNode&);
    friend std::istream& operator>> <T, Reference> (std::istream &, GeneralNode&);
};

#endif // __GENERALNODE_H__