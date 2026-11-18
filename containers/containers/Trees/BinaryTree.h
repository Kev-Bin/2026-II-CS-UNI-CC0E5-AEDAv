#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__

#include "../../Container.h"
#include "../vector.h"
#include "../../../types.h"

template <typename T, typename R = Ref, typename Derived = void>
class BinaryTreeNode : public GeneralNode<T, R> {

public:

    using Parent        = GeneralNode<T, R>;
    using MySelf        = std::conditional_t<std::is_void_v<Derived>, BinaryTreeNode, Derived>;
    using value_type    = Parent::value_type;
    
    using Parent::Parent;

protected:

    MySelf* m_pChild[2] = {nullptr, nullptr};

public:

    BinaryTreeNode(const BinaryTreeNode& another) : Parent(another) {
        if (another.getLeft())
            m_pChild[0] = new MySelf(*another.getLeft());

        if (another.getRight())
            m_pChild[1] = new MySelf(*another.getRight());
    }

    virtual ~BinaryTreeNode() = default;

    bool            hasLeft() const  { return m_pChild[0] != nullptr; }
    bool            hasRight() const { return m_pChild[1] != nullptr; }

    MySelf*&        getLeft()   { return m_pChild[0]; }
    MySelf*&        getRight()  { return m_pChild[1]; }

    const MySelf*   getLeft() const     { return m_pChild[0]; }
    const MySelf*   getRight() const    { return m_pChild[1]; }

    void            setLeft(MySelf* ptr)    { m_pChild[0] = ptr; }
    void            setRight(MySelf* ptr)   { m_pChild[1] = ptr; }

};

template <typename BTIterator>
class Range {

    public:

        using Node = typename BTIterator::value_type;   // el iterador no define 'Node'

    private:

        Node* m_pRoot;
    
    public:

        Range(Node* root) : m_pRoot(root) {}

        BTIterator begin()  const   { return BTIterator(m_pRoot); }
        BTIterator end()    const   { return BTIterator(nullptr); }

};

template <typename Node, bool Reverse>
struct Traversal {
    static Node* getFirst(Node* ptr) {
        if constexpr (Reverse) return ptr->getRight();
        else return ptr->getLeft();
    }

    static Node* getLast(Node* ptr) {
        if constexpr (Reverse) return ptr->getLeft();
        else return ptr->getRight();
    }

    static bool hasFirst(Node* ptr) {
        if constexpr (Reverse) return ptr->hasRight();
        else return ptr->hasLeft();
    }

    static bool hasLast(Node* ptr) {
        if constexpr (Reverse) return ptr->hasLeft();
        else return ptr->hasRight();
    }
};


template <typename T, bool IsConst = false, bool Reverse = false>
class BTInOrderIterator : public Iterator<T, IsConst> {

    public:

        using MySelf        = BTInOrderIterator<T, IsConst, Reverse>;
        using Parent        = Iterator<T, IsConst>;
        using value_type    = typename Parent::value_type;
        using Trav          = Traversal<value_type, Reverse>;
        using Parent::set;

    private:

        Vector<value_type*> stack;
    
    public:

        BTInOrderIterator(value_type* ptr) : Parent(ptr) { if (ptr) descendFirst(ptr); }

        MySelf& operator++() override {
            if (!this->m_ptr) return *this;

            if (Trav::hasLast(this->m_ptr)) {
                descendFirst(Trav::getLast(this->m_ptr));
                return *this;
            }

            if (stack.empty()) {
                set(nullptr);
                return *this;
            }

            set(stack.pop_back().getValue());
            return *this;
        }

    private:

        void descendFirst(value_type* ptr) {
            while (Trav::hasFirst(ptr)) {
                stack.pushBack(ptr, 0);
                ptr = Trav::getFirst(ptr);
            }
            set(ptr);
        }
};

template <typename T, bool IsConst = false, bool Reverse = false>
class BTPreorderIterator : public Iterator<T,IsConst> {

    public:

        using MySelf        = BTPreorderIterator<T, IsConst, Reverse>;
        using Parent        = Iterator<T, IsConst>;
        using value_type    = typename Parent::value_type;
        using Trav          = Traversal<value_type, Reverse>;
        using Parent::set;
        
        BTPreorderIterator(value_type* ptr) : Parent(ptr) { if (ptr) pushChildren(ptr); }

        MySelf& operator++() override {
            if (stack.empty()) {
                set(nullptr);
                return *this;
            }

            value_type* next = stack.pop_back().getValue();

            pushChildren(next);
            set(next);
            return *this;
        }

    private:

        Vector<value_type*> stack;

        void pushChildren(value_type* ptr) {
            if (Trav::hasLast(ptr)) stack.pushBack(Trav::getLast(ptr), 0);
            if (Trav::hasFirst(ptr)) stack.pushBack(Trav::getFirst(ptr), 0);
        }
};

template <typename T, bool IsConst = false, bool Reverse = false>
class BTPostorderIterator : public Iterator<T,IsConst> {

    public:

        using MySelf        = BTPostorderIterator<T, IsConst, Reverse>;
        using Parent        = Iterator<T, IsConst>;
        using value_type    = typename Parent::value_type;
        using Trav          = Traversal<value_type, Reverse>;
        using Parent::set;
    
    private:

        Vector<value_type*> stack;
    
    public:

        BTPostorderIterator(value_type* ptr) : Parent(ptr) {
            if (!ptr) return;
            
            value_type* curr = ptr;

            while (Trav::hasFirst(curr) || Trav::hasLast(curr)) {
                stack.pushBack(curr, 0);
                if (Trav::hasFirst(curr)) curr = Trav::getFirst(curr);
                else curr = Trav::getLast(curr);
            }
            set(curr);
        }
        
        MySelf& operator++() override {
            if (stack.empty()) {
                set(nullptr);
                return *this;
            }
            
            value_type* parent = stack.pop_back().getValue();

            if (Trav::hasLast(parent) && Trav::getLast(parent) != this->m_ptr) {
                stack.pushBack(parent, 0);
                
                value_type* curr = Trav::getLast(parent);
                
                while (Trav::hasFirst(curr) || Trav::hasLast(curr)) {
                    stack.pushBack(curr, 0);
                    if (Trav::hasFirst(curr)) curr = Trav::getFirst(curr);
                    else curr = Trav::getLast(curr);
                }

                set(curr);
                return *this;
            }
            set(parent);
            return *this;
        }

};

template <typename CompareTraits, typename NodeT = BinaryTreeNode<typename CompareTraits::value_type>>
struct BinaryTreeTraits {

    using value_type                = typename CompareTraits::value_type;
    using Compare                   = typename CompareTraits::Compare;
    using Node                      = NodeT;

    using ForwardIterator           = BTPreorderIterator<Node>;
    using ConstForwardIterator      = BTPreorderIterator<Node, true>;

    using BackwardIterator          = BTPostorderIterator<Node, false, true>;
    using ConstBackwardIterator     = BTPostorderIterator<Node, true, true>;

    using InOrderIterator           = BTInOrderIterator<Node>;
    using ReverseInOrderIterator    = BTInOrderIterator<Node, false, true>;

    using PreOrderIterator          = BTPreorderIterator<Node>;
    using ReversePreOrderIterator   = BTPreorderIterator<Node, false, true>;

    using PostOrderIterator         = BTPostorderIterator<Node>;
    using ReversePostOrderIterator  = BTPostorderIterator<Node, false, true>;
        
};

template <typename T, typename NodeT = BinaryTreeNode<T>>
struct BinaryTreeAscTraits  : BinaryTreeTraits<AscendingTraits<T>, NodeT> {};

template <typename T, typename NodeT = BinaryTreeNode<T>>
struct BinaryTreeDescTraits : BinaryTreeTraits<DescendingTraits<T>, NodeT> {};

template <typename Traits>
class BinaryTree : 
    public Container<Traits, BinaryTree<Traits>>,
    public BackwardIterable<Traits> {//toca quitar backward

public:

    using Parent                    = Container<Traits, BinaryTree<Traits>>;
    
    using value_type                = typename Parent::value_type;
    using Node                      = typename Parent::Node;
    using Compare                   = typename Traits::Compare;
    using ForwardIterator           = typename Parent::ForwardIterator;
    using ConstForwardIterator      = typename Parent::ConstForwardIterator;
    
    using BackwardIterator          = typename Traits::BackwardIterator;
    using ConstBackwardIterator     = typename Traits::ConstBackwardIterator;
    
    using InOrderIterator           = typename Traits::InOrderIterator;
    using ReverseInOrderIterator    = typename Traits::ReverseInOrderIterator;
    
    using PreOrderIterator          = typename Traits::PreOrderIterator;
    using ReversePreOrderIterator   = typename Traits::ReversePreOrderIterator;
    
    using PostOrderIterator         = typename Traits::PostOrderIterator;
    using ReversePostOrderIterator  = typename Traits::ReversePostOrderIterator;

protected:

    using Parent::m_comp;
    using Parent::m_mutex;

    Node* m_pRoot = nullptr;

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

    void internalClear() override {
        this->ApplyFunction([](Node& node) { delete &node; });
        m_pRoot = nullptr;
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

    Parent& internalCopy(const BinaryTree& another) override { 
        Parent::internalCopy(another);
        
        if (this != &another) {
            internalClear();
            m_pRoot = another.m_pRoot ? new Node(*another.m_pRoot) : nullptr;
        }
        
        return *this;
    }

    std::ostream& printTree(std::ostream& os, Node* node, const char* connector = "\t", size_t lvl = 0) const {
        if (!node) 
            return os;

        printTree(os, node->getRight(),"┌──", lvl + 1);

        for (size_t i = 0; i < lvl; i++) os << "\t";  
        os << connector <<*node << '\n';

        printTree(os, node->getLeft(), "└──", lvl + 1);

        return os;
    }

public:
    BinaryTree() = default;

    BinaryTree(const BinaryTree& another) : Parent() { this->copy(another); }

    BinaryTree(initializer_list<pair<value_type, Ref>> values) { 
        for (const auto& value : values) 
            push(value.first, value.second); 
    }

    BinaryTree(BinaryTree&& another) : Parent() {
        std::scoped_lock lock(another.m_mutex);

        m_comp  = std::move(another.m_comp);
        m_pRoot = std::exchange(another.m_pRoot, nullptr);
    }

    ~BinaryTree() override { this->clear(); }

    bool empty() const override { return m_pRoot == nullptr; }

    //iteradores x defecto
    ForwardIterator      begin() override           { return ForwardIterator(m_pRoot); }
    ForwardIterator      end() override             { return ForwardIterator(nullptr); }
    ConstForwardIterator cbegin() const override    { return ConstForwardIterator(m_pRoot); }
    ConstForwardIterator cend() const override      { return ConstForwardIterator(nullptr); }
    
    BackwardIterator      rbegin() override          { return BackwardIterator(m_pRoot); }
    BackwardIterator      rend() override            { return BackwardIterator(nullptr); }
    ConstBackwardIterator crbegin() const override   { return ConstBackwardIterator(m_pRoot); }
    ConstBackwardIterator crend() const override     { return ConstBackwardIterator(nullptr); }
    
    //Rangos
    auto inOrder()    { return Range<InOrderIterator>(m_pRoot); }
    auto rInOrder()   { return Range<ReverseInOrderIterator>(m_pRoot); }
    auto preOrder()   { return Range<PreOrderIterator>(m_pRoot); }
    auto rPreOrder()  { return Range<ReversePreOrderIterator>(m_pRoot); }
    auto postOrder()  { return Range<PostOrderIterator>(m_pRoot); }
    auto rPostOrder() { return Range<ReversePostOrderIterator>(m_pRoot); }
        
    friend std::ostream& operator<<(std::ostream& os, const BinaryTree<Traits>& tree) {
        std::scoped_lock lock(tree.m_mutex);
        return tree.printTree(os, tree.m_pRoot);
    }

};
#endif // __BINARYTREE_H__