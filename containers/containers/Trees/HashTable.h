#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include <stdexcept>
#include "AVLTree.h"

// Tabla clave -> valor respaldada por un AVLTree.
// Cada nodo guarda la clave en getValue() y el valor asociado en getRef().
//
// Nota: Container::push(value, ref) recibe el valor como 'Ref' (long long),
// por lo que V debe poder construirse desde Ref.
template <typename K, typename V>
using HashTableNode = AVLTreeNode<K, V>;

template <typename K, typename V>
struct HashTableAscTraits  : AVLTreeAscTraits<K, HashTableNode<K, V>> {};

template <typename K, typename V>
struct HashTableDescTraits : AVLTreeDescTraits<K, HashTableNode<K, V>> {};

template <typename K, typename V, typename Traits = HashTableAscTraits<K, V>>
class HashTable : public AVLTree<Traits> {

public:

    using Parent      = AVLTree<Traits>;
    using value_type  = typename Parent::value_type;   // K
    using Node        = typename Parent::Node;
    using key_type    = K;
    using mapped_type = V;

protected:

    using Parent::m_pRoot;
    using Parent::m_comp;
    using Parent::m_mutex;
    using Parent::internalPush;   // evita ocultar internalPush(value, ref)

    // Insercion con semantica de mapa: si la clave ya existe se actualiza
    // su valor en lugar de insertar un duplicado.
    void internalPush(const value_type& key, Ref value, Node*& root) override {
        if (!root)
            root = new Node(key, value);
        else if (root->getValue() == key) {
            root->setRef(value);
            return;
        }
        else if (m_comp(key, root->getValue()))
            internalPush(key, value, root->getLeft());
        else
            internalPush(key, value, root->getRight());

        this->rebalance(root);
    }

    Node* internalGet(const key_type& key) const {
        Node* curr = m_pRoot;

        while (curr && curr->getValue() != key)
            curr = m_comp(key, curr->getValue()) ? curr->getLeft() : curr->getRight();

        return curr;
    }

    void internalRemoveKey(const key_type& key, Node*& root) {
        if (!root) return;

        if (root->getValue() == key)
            this->removeNode(root);
        else if (m_comp(key, root->getValue()))
            internalRemoveKey(key, root->getLeft());
        else
            internalRemoveKey(key, root->getRight());

        if (root)
            this->rebalance(root);
    }

public:

    using Parent::Parent;
    using Parent::remove;   // conserva remove(key, value) de Container

    bool contains(const key_type& key) const {
        std::scoped_lock lock(m_mutex);
        return internalGet(key) != nullptr;
    }

    // Elimina la clave sin importar el valor asociado.
    void remove(const key_type& key) {
        std::scoped_lock lock(m_mutex);
        internalRemoveKey(key, m_pRoot);
    }

    // Acceso con verificacion: lanza std::out_of_range si la clave no existe.
    mapped_type& at(const key_type& key) {
        std::scoped_lock lock(m_mutex);
        Node* node = internalGet(key);
        if (!node) throw std::out_of_range("HashTable::at: clave no encontrada");
        return node->getRef();
    }

    const mapped_type& at(const key_type& key) const {
        std::scoped_lock lock(m_mutex);
        const Node* node = internalGet(key);
        if (!node) throw std::out_of_range("HashTable::at: clave no encontrada");
        return node->getRef();
    }

    // Como std::map: si la clave no existe se inserta con valor por defecto.
    mapped_type& operator[](const key_type& key) {
        std::scoped_lock lock(m_mutex);
        Node* node = internalGet(key);
        if (!node) {
            internalPush(key, mapped_type{}, m_pRoot);
            node = internalGet(key);
        }
        return node->getRef();
    }
};

#endif // __HASH_TABLE_H__
