#ifndef __AVLTREE_H__
#define __AVLTREE_H__

#include "BinaryTree.h"
#include <algorithm>
#include <initializer_list>
#include <utility>

template <typename T, typename R = Ref, typename Derived = void>
class AVLTreeNode : public BinaryTreeNode<T, R, AVLTreeNode<T, R, Derived>> {

    public:

        using Parent = BinaryTreeNode<T, R, AVLTreeNode<T, R, Derived>>;
        using Parent::Parent;

    protected:

        size_t m_height = 1;

    public:
    
        AVLTreeNode(const AVLTreeNode& another) : Parent(another), m_height(another.m_height) {}

        size_t  height() const              { return m_height; }
        void    setHeight(size_t height)    { m_height = height; }

};

template <typename T, typename NodeT = AVLTreeNode<T>>
struct AVLTreeAscTraits  : BinaryTreeAscTraits<T, NodeT> {};

template <typename T, typename NodeT = AVLTreeNode<T>>
struct AVLTreeDescTraits : BinaryTreeDescTraits<T, NodeT> {};

template <typename Traits>
class AVLTree : public BinaryTree<Traits> {

    public:

        using Parent     = BinaryTree<Traits>;
        using value_type = typename Parent::value_type;
        using Node       = typename Parent::Node;


    protected:

        using Parent::m_mutex;   // protected para que las subclases (HashTable) lo usen

    public:
    
        AVLTree() = default;

        AVLTree(std::initializer_list<std::pair<value_type, Ref>> values) { for (const auto& [value, ref] : values) this->push(value, ref); }

        // copy() ya toma m_mutex (via Container::operator=); bloquearlo aqui
        // tambien causaba deadlock al ser std::mutex no recursivo.
        AVLTree(const AVLTree& another) : Parent(another) {}

        AVLTree& operator=(const AVLTree& another) {
            if (this != &another)
                copy(another);

            return *this;
        }

    protected:

        using Parent::copy;

        using Parent::m_comp;

        static int height(Node* n) { return n ? n->height() : 0; }

        static void updateHeight(Node* node) { node->setHeight(1 + std::max(height(node->getLeft()), height(node->getRight()))); }

        static int bFactor(Node* node) { return height(node->getLeft()) - height(node->getRight()); }

        void rebalance(Node*& root) override {
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
