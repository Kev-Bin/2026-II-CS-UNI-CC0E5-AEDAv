#ifndef __NODEIO_H__
#define __NODEIO_H__

#include "../../types.h"
#include "../GeneralNode.h"

template <typename T, typename Reference>
std::ostream& operator << (std::ostream &os, const GeneralNode<T, Reference> &node) {
    
    os  << "(" 
        << node.getValue() 
        << "," 
        << node.getRef() 
        << ")";
    
    return os;
}

template <typename T, typename Reference>
std::istream& operator >> (std::istream &is, GeneralNode<T, Reference> &node){
    Reference ref;

    typename GeneralNode<T, Reference>::Delim       d;
    typename GeneralNode<T, Reference>::value_type  value;


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

    node.setValue(value);
    node.setRef(ref);

    return is;
}

#endif // __NODE_H__