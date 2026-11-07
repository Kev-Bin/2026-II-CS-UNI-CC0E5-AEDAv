#ifndef __NODEIO_H__
#define __NODEIO_H__

#include "../types.h"
#include "GeneralNode.h"

template <typename T>
std::ostream& operator << (std::ostream &os, const GeneralNode<T> &node) {
    
    os  << "(" 
        << node.getValue() 
        << "," 
        << node.getRef() 
        << ")";
    
    return os;
}

template <typename T>
std::istream& operator >> (std::istream &is, GeneralNode<T> &node){
    Ref ref;

    GeneralNode<T>::Delim       d;
    GeneralNode<T>::value_type  value;


    if (!(is >> std::ws >> d) || d != '(') {
        is.setstate(std::ios::failbit);
        return is;
    }

    if (!(is >> std::ws >> value >> std::ws >> d) || d != ',') {
        is.setstate(std::ios::failbit);
        return is;
    }

    if (!(is >> std::ws >> ref >> std::ws >> d) || d != ')') {
        is.setstate(std::ios::failbit);
        return is;
    }

    node.m_value    = value;
    node.m_ref      = ref;

    return is;
}

#endif // __NODE_H__