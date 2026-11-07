#ifndef __AVLTREE_H__
#define __AVLTREE_H__

#include "binarytree.h"
#include <algorithm>
#include <initializer_list>
#include <mutex>
#include <utility>

template <typename T>
class AVLTreeNode : public BinaryTreeNode<T, AVLTreeNode<T>> {

public:

    using Parent = BinaryTreeNode<T, AVLTreeNode<T>>;
    using Parent::Parent;

protected:

    size_t m_height = 1;

public:

    AVLTreeNode(const AVLTreeNode& another) : Parent(another), m_height(another.m_height) {}

    size_t  height() const  { return m_height; }

    void    setHeight(size_t height) { m_height = height; }

};

template <typename T>
struct AVLTreeAscTraits : BinaryTreeAscTraits<T> {
    using Node = AVLTreeNode<T>;
};

template <typename T>
struct AVLTreeDescTraits : BinaryTreeDescTraits<T> {
    using Node = AVLTreeNode<T>;
};

template <typename Traits>
class AVLTree : 
    public BinaryTree<Traits> {

public:

    using Parent     = BinaryTree<Traits>;
    using value_type = typename Parent::value_type;
    using Node       = AVLTreeNode<value_type>;

    AVLTree() = default;

    AVLTree(std::initializer_list<std::pair<value_type, Ref>> values) { for (const auto& [value, ref] : values) push(value, ref); }

    AVLTree(const AVLTree& another) : Parent() { 
        scoped_lock lock(m_mutex);
        copy(another); 
    }

    AVLTree& operator=(const AVLTree& another) {
        if (this != &another) {
            std::scoped_lock lock(this->m_mutex, another.m_mutex);
            copy(another);
        }

        return *this;
    }

protected:

    using Parent::copy;

    using Parent::m_comp;

    void internalPush(Node* node) override {
        recursivePush(this->m_pRoot, node);
    }

    Node* cloneTree(Node* node) override { return node ? new Node(*node) : nullptr; }

    static void updateHeight(Node* node) {
        if (!node) 
            return;

        size_t lHeight = node->getLeft()->height();
        size_t rHeight = node->getRight()->height();

        node->setHeight(1 + std::max(lHeight, lHeight)); 
    }

    static int bFactor(Node* node) { 
        if (!node) 
            return 0;

        size_t lHeight = node->getLeft()->height();
        size_t rHeight = node->getRight()->height();

        return lHeight - rHeight; 
    }

    static void rebalance(Node*& root) {
        if (!root) 
            return;

        updateHeight(root);

        int balance = bFactor(root);
        
        if (balance > 1) {
            Node*& left = root->getLeft();

            if (bFactor(left) < 0) 
                rotate(left, 0);

            rotate(root, 1);
        } 
        else if (balance < -1) {
            Node*& right = root->getRight();

            if (bFactor(right) > 0) 
                rotate(right, 1);

            rotate(root, 0);
        }
    }

    static void rotate(Node*& root, int dir) {
        Node* pivot = (dir == 0) ? root->getRight() : root->getLeft();

        if (dir == 0) {
            root->getRight() = pivot->getLeft();
            pivot->getLeft() = root;
        } else {
            root->getLeft() = pivot->getRight();
            pivot->getRight() = root;
        }

        updateHeight(root);
        updateHeight(pivot);

        root = pivot;
    }

    void remove(Node*& root) {
        if (!root->hasLeft()) {
            Node* temp = root;
            root = root->getRight();
            delete temp;
        }
        else if (!root->hasRight()) {
            Node* temp = root;
            root = root->getLeft();
            delete temp;
        }
        else {
            // Tiene dos hijos.
            // Buscamos el máximo del subárbol izquierdo.
            Node* temp = this->max(root->getLeft());

            temp->setLeft(root->getLeft());
            temp->setRight(root->getRight());

            Node* oldRoot = root;
            root = temp;

            delete oldRoot;
        }

        if (root)
            rebalance(root);
    }
};
#endif // __AVLTREE_H__
