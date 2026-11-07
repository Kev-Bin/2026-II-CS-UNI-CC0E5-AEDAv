#ifndef __CONTAINERIO_H__
#define __CONTAINERIO_H__

#include "Container.h"
#include <istream>
#include <ostream>

template <typename Traits, typename Derived>
std::ostream& operator<<(std::ostream& os, const Container<Traits, Derived>& container) {
    using Node = typename Container<Traits, Derived>::Node;

    std::scoped_lock lock(container.m_mutex);

    bool first = true;

    os << '[';
    container.cApplyFunction([&](const Node& node) {
        if (!first) os << ',';
        os << node;
        first = false;
    });

    return os << ']';
}

template <typename Traits, typename Derived>
std::istream& operator>>(std::istream& is, Container<Traits, Derived>& container) {
    using Node  = typename Container<Traits, Derived>::Node;
    using Delim = typename Container<Traits, Derived>::Delim;

    Node node;
    Delim d;

    is >> std::ws >> d;
    if (d != '[') return is;
    std::scoped_lock lock(container.m_mutex);
    container.internalClear();
    is >> std::ws;
    if (is.peek() == ']') return is >> d;

    while (is >> node) {
        container.internalPush(node.getValue(), node.getRef());
        is >> std::ws >> d;
        if (d == ']') break;
    }

    return is;
}
#endif // __CONTAINERIO_H__