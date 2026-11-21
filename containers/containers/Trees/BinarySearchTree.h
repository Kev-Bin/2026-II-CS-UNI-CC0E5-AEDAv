#ifndef __BINARYSEARCHTREE_H__
#define __BINARYSEARCHTREE_H__

#include <initializer_list>
#include <utility>
#include "BinaryTree.h"

// Arbol binario de busqueda: inserta y elimina comparando con m_comp
// (menores a la izquierda, el resto a la derecha). No se balancea por si
// mismo; las subclases (AVLTree) redefinen rebalance.
template <typename Traits>
class BinarySearchTree : public BinaryTree<Traits> {

public:

    using Parent     = BinaryTree<Traits>;
    using value_type = typename Parent::value_type;
    using Node       = typename Parent::Node;
    using Compare    = typename Parent::Compare;

protected:

    using Parent::m_pRoot;
    using Parent::m_comp;
    using Parent::m_mutex;

    virtual void rebalance(Node*& root) {}

    void internalPush(const value_type& value, Ref ref) override { internalPush(value, ref, m_pRoot); }

    virtual void internalPush(const value_type& value, Ref ref, Node*& root) {
        if (!root)
            root = new Node(value, ref);
        else if (m_comp(value, root->getValue()))
            internalPush(value, ref, root->getLeft());
        else
            internalPush(value, ref, root->getRight());

        rebalance(root);
    }

    void internalRemove(const value_type& value, Ref ref) override { internalRemove(value, ref, m_pRoot); }

    virtual void internalRemove(const value_type& value, Ref ref, Node*& root) {
        if (!root) return;

        if (root->getValue() == value && root->getRef() == ref)
            removeNode(root);
        else if (m_comp(value, root->getValue()))
            internalRemove(value, ref, root->getLeft());
        else
            internalRemove(value, ref, root->getRight());

        if (root)
            rebalance(root);
    }

    void removeNode(Node*& root) {
        Node* toDelete = root;

        if (!root->hasLeft())
            root = root->getRight();
        else if (!root->hasRight())
            root = root->getLeft();
        else {
            Node* pred = extractMax(toDelete->getLeft());
            pred->setLeft(toDelete->getLeft());
            pred->setRight(toDelete->getRight());
            root = pred;
        }
        delete toDelete;
    }

    Node* extractMax(Node*& root) {
        if (root->hasRight()) {
            Node* m = extractMax(root->getRight());
            rebalance(root);
            return m;
        }
        Node* m = root;
        root = root->getLeft();
        return m;
    }

public:

    BinarySearchTree() = default;

    BinarySearchTree(const BinarySearchTree& another) : Parent(another) {}

    BinarySearchTree(BinarySearchTree&& another) : Parent(std::move(another)) {}

    BinarySearchTree(std::initializer_list<std::pair<value_type, Ref>> values) {
        for (const auto& [value, ref] : values)
            this->push(value, ref);
    }
};

#endif // __BINARYSEARCHTREE_H__
