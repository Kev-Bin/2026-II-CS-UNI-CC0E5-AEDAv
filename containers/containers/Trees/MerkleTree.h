#ifndef __MERKLETREE_H__
#define __MERKLETREE_H__

#include <functional>
#include <initializer_list>
#include <utility>
#include "../vector.h"
#include "BinaryTree.h"
#include "../../../types.h"

template <typename T>
struct MerkleTreeTraits : BinaryTreeTraits<AscendingTraits<T>, BinaryTreeNode<MerkleHash>> {};

template <typename T, typename Traits = MerkleTreeTraits<T>>
class MerkleTree : public BinaryTree<Traits> {

public:

    using Parent     = BinaryTree<Traits>;
    using value_type = typename Parent::value_type;
    using Node       = typename Parent::Node;
    using Hash       = MerkleHash;

protected:

    using Parent::m_pRoot;
    using Parent::m_mutex;

    static constexpr Ref InternalRef = -1;

    static Hash mix(Hash x) {
        x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27; x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return x;
    }

    static Hash hashData(const value_type& data) { return mix(std::hash<value_type>{}(data)); }

    static Hash combine(Hash left, Hash right) {
        return left ^ (right + 0x9e3779b97f4a7c15ULL + (left << 6) + (left >> 2));
    }

    static bool isLeaf(Node* node) { return !node->hasLeft() && !node->hasRight(); }

    using NodeList = Vector<Node*>;

    void detachLeaves(NodeList& leaves) {
        NodeList internals;

        for (auto&& node : this->inOrder())
            (isLeaf(&node) ? leaves : internals).pushBack(&node, 0);

        for (size_t i = 0; i < internals.size(); ++i)
            delete internals[i].getValue();

        m_pRoot = nullptr;
    }

    void build(NodeList& leaves) {
        NodeList  other;
        NodeList* level = &leaves;
        NodeList* next  = &other;

        while (level->size() > 1) {
            next->clear();

            for (size_t i = 0; i < level->size(); i += 2) {
                if (i + 1 == level->size()) {       // impar: sube sin cambios
                    next->pushBack((*level)[i].getValue(), 0);
                    break;
                }

                Node* left   = (*level)[i].getValue();
                Node* right  = (*level)[i + 1].getValue();
                Node* parent = new Node(combine(left->getValue(), right->getValue()), InternalRef);

                parent->setLeft(left);
                parent->setRight(right);
                next->pushBack(parent, 0);
            }

            std::swap(level, next);
        }

        m_pRoot = level->empty() ? nullptr : (*level)[0].getValue();
    }

    void internalPush(const value_type& data, Ref ref) override {
        NodeList leaves;
        detachLeaves(leaves);
        leaves.pushBack(new Node(hashData(data), ref), 0);
        build(leaves);
    }

    void internalRemove(const value_type& data, Ref ref) override {
        NodeList leaves;
        detachLeaves(leaves);
        Hash h = hashData(data);

        for (size_t i = 0; i < leaves.size(); ++i) {
            Node* leaf = leaves[i].getValue();

            if (leaf->getValue() == h && leaf->getRef() == ref) {
                leaves.remove(leaf, 0);
                delete leaf;
                break;
            }
        }

        build(leaves);
    }

public:

    MerkleTree() = default;
    MerkleTree(const MerkleTree& another) : Parent(another) {}
    MerkleTree(MerkleTree&& another) : Parent(std::move(another)) {}

    MerkleTree(std::initializer_list<std::pair<value_type, Ref>> values) {
        for (const auto& [data, ref] : values)
            this->push(data, ref);
    }

    Hash root() const {
        std::scoped_lock lock(m_mutex);
        return m_pRoot ? m_pRoot->getValue() : 0;
    }

    size_t leafCount() {
        std::scoped_lock lock(m_mutex);
        size_t count = 0;
        for (auto&& node : this->inOrder())
            count += isLeaf(&node);
        return count;
    }
};

#endif // __MERKLETREE_H__
