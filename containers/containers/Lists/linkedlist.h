/**
 * @file linkedlist.h
 * @brief Lista simplemente enlazada genérica con nodo centinela.
 *
 * Define el nodo, el iterador hacia adelante, los traits y la clase
 * LinkedList. El orden de inserción ordenada (insert()) lo determina
 * el comparador de los traits: LinkedListAscTraits o LinkedListDescTraits.
 *
 * @code
 * LinkedList<LinkedListAscTraits<int>> lista = {{5, 1}, {3, 2}};
 * lista.insert(4, 3);     // inserción ordenada
 * lista.pushBack(1, 4);   // inserción al final, sin respetar el orden
 * @endcode
 */
#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <initializer_list>
#include <iostream>
#include <mutex>
#include "../../GeneralNode.h"
#include "../../Iterator.h"
#include "../../../foreach.h"
#include "../../List.h"

/**
 * @brief Nodo de una lista simplemente enlazada.
 *
 * Guarda un valor y una referencia (heredados de GeneralNode) y un puntero
 * al siguiente nodo. El último nodo real apunta al centinela de la lista.
 *
 * @tparam T Tipo del valor almacenado.
 */
template <typename T>
class LinkedListNode : public GeneralNode<T> {

public:

    using Node      = LinkedListNode<T>;   ///< Alias del propio tipo de nodo.

    Node* m_pNext = nullptr;               ///< Siguiente nodo (o el centinela si es el último).

    /// @brief Construye un nodo vacío (valor y referencia por defecto). Se usa para el centinela.
    LinkedListNode() : GeneralNode<T>(T{}, Ref{}), m_pNext(nullptr) {}

    /**
     * @brief Construye un nodo con valor, referencia y siguiente.
     * @param value Valor a almacenar.
     * @param ref   Referencia asociada al valor (Ref = long long).
     * @param pNext Nodo siguiente.
     */
    LinkedListNode(const T& value, Ref ref, Node* pNext) : GeneralNode<T>(value, ref), m_pNext(pNext) {}
};

/**
 * @brief Iterador hacia adelante para LinkedList.
 *
 * Avanza siguiendo m_pNext. El recorrido termina al llegar al centinela,
 * que es lo que devuelve end().
 *
 * @tparam Node    Tipo de nodo que se recorre.
 * @tparam IsConst true para un iterador de solo lectura.
 */
template <typename Node, bool IsConst = false>
class LinkedListForwardIterator : public Iterator<Node,IsConst> {
public:
    using MySelf = LinkedListForwardIterator<Node,IsConst>;  ///< Alias del propio tipo.
    using Parent = Iterator<Node,IsConst>;                    ///< Clase base.
    using Parent::Parent;
    using Parent::set;

    /**
     * @brief Pre-incremento: avanza al siguiente nodo.
     * @return Referencia a este iterador ya avanzado.
     * @complexity O(1)
     */
    MySelf& operator++() override {
        set((*this)->m_pNext);
        return *this;
    }
};

/**
 * @brief Agrupa los tipos que usa LinkedList.
 *
 * @tparam CompareTraits Traits que aportan value_type y Compare
 *         (AscendingTraits<T> o DescendingTraits<T>).
 */
template <typename CompareTraits>
struct LinkedListTraits {
    using value_type            = typename CompareTraits::value_type;     ///< Tipo del valor almacenado.
    using Compare               = typename CompareTraits::Compare;        ///< Comparador que define el orden de insert().
    using Node                  = LinkedListNode<value_type>;             ///< Tipo de nodo.
    using ForwardIterator       = LinkedListForwardIterator<Node>;        ///< Iterador mutable.
    using ConstForwardIterator  = LinkedListForwardIterator<Node,true>;   ///< Iterador de solo lectura.
};

/// @brief Traits para una LinkedList en orden ascendente.
/// @tparam T Tipo del valor almacenado.
template <typename T> struct LinkedListAscTraits : public LinkedListTraits<AscendingTraits<T>> {};

/// @brief Traits para una LinkedList en orden descendente.
/// @tparam T Tipo del valor almacenado.
template <typename T> struct LinkedListDescTraits : public LinkedListTraits<DescendingTraits<T>> {};


/**
 * @brief Lista simplemente enlazada con centinela al final.
 *
 * - Mantiene punteros a la cabeza (m_pRoot) y a la cola (m_pTail), por lo
 *   que pushBack() es O(1).
 * - El último nodo apunta a @c sentinel, que es lo que devuelve end().
 * - Las operaciones públicas heredadas (insert, pushBack, remove, clear)
 *   toman el mutex de la clase base; los métodos @c internal* asumen que
 *   el lock ya está tomado.
 *
 * @tparam Traits LinkedListAscTraits<T>, LinkedListDescTraits<T> u otro
 *         LinkedListTraits.
 *
 * @see List, LinkedListTraits
 */
template <typename Traits>
class LinkedList :
    public List<Traits> {

public:

    using Parent                = List<Traits>;                  ///< Clase base.

    using value_type            = typename Parent::value_type;   ///< Tipo del valor almacenado.
    using Node                  = typename Parent::Node;         ///< Tipo de nodo.
    using Delim                 = typename Parent::Delim;

    using Parent::Parent;

protected:
    using Parent::m_comp;
    using Parent::m_size;
    using Parent::m_pRoot;
    using Parent::m_pTail;
    using Parent::sentinel;

    /**
     * @brief Crea un nodo y lo agrega al final.
     * @param value Valor a agregar.
     * @param ref   Referencia asociada.
     * @complexity O(1)
     */
    void internalPushBack(const value_type& value, Ref ref) override { internalPushBack(new Node(value, ref, &sentinel)); }

    /**
     * @brief Enlaza un nodo ya creado al final de la lista.
     * @param nodePtr Nodo a enlazar. La lista toma su propiedad.
     * @post nodePtr pasa a ser la cola y apunta al centinela.
     * @complexity O(1)
     */
    virtual void internalPushBack(Node* nodePtr) {
        nodePtr->m_pNext = &sentinel;
        if (m_size == 0) {
            m_pRoot = m_pTail = nodePtr;
        } else {
            m_pTail->m_pNext = nodePtr;
            m_pTail = nodePtr;
        }
        ++m_size;
    }

    /**
     * @brief Crea un nodo y lo inserta en su posición ordenada.
     * @param value Valor a insertar.
     * @param ref   Referencia asociada.
     * @complexity O(n)
     */
    void internalInsert(const value_type& value, Ref ref) { internalInsert(new Node(value, ref, nullptr)); }

    /**
     * @brief Inserta un nodo ya creado según el comparador @c m_comp.
     *
     * Recorre la lista con un puntero a puntero (@c Node**), lo que evita
     * tratar aparte la inserción en la cabeza. Si ningún elemento cumple
     * la comparación, el nodo se agrega al final.
     *
     * @param nodePtr Nodo a insertar. La lista toma su propiedad.
     * @complexity O(n)
     */
    virtual void internalInsert(Node* nodePtr) {
        if (m_size == 0)
            return internalPushBack(nodePtr);

        Node** currentPtr = &m_pRoot;
        while (*currentPtr != &sentinel) {
            if(m_comp(nodePtr->getValue(), (*currentPtr)->getValue())) {
                nodePtr->m_pNext = *currentPtr;
                *currentPtr = nodePtr;
                ++m_size;
                return;
            }
            currentPtr = &(*currentPtr)->m_pNext;
        } // bucle mejorable

        internalPushBack(nodePtr);
    }

    /**
     * @brief Une dos nodos tras eliminar el que estaba entre ellos.
     *
     * La llama List::internalRemove() antes de liberar el nodo borrado.
     *
     * @param prev Nodo anterior al eliminado, o @c nullptr si se eliminó la cabeza.
     * @param next Nodo siguiente al eliminado (puede ser el centinela).
     * @post Si @p next es el centinela, @c m_pTail pasa a ser @p prev.
     * @complexity O(1)
     */
    void connect(Node* prev, Node* next) override {
        // Si se quito el ultimo nodo, el anterior pasa a ser la cola (o
        // nullptr si la lista quedo vacia). Sin esto m_pTail apuntaria al
        // nodo borrado y el siguiente pushBack escribiria en memoria liberada.
        if (next == &sentinel)
            m_pTail = prev;

        if (prev == nullptr) {
            m_pRoot = next;
            return;
        }

        prev->m_pNext = next;
    }

public:

    /// @brief Construye una lista vacía.
    LinkedList() = default;

    /**
     * @brief Construye la lista a partir de pares (valor, referencia).
     *
     * Los elementos se agregan con pushBack() en el orden dado, sin ordenar.
     *
     * @param values Lista de pares {valor, ref}.
     * @complexity O(n)
     */
    LinkedList(initializer_list<pair<value_type, Ref>> values) : LinkedList() {
        for (const auto& value : values)
            this->pushBack(value.first, value.second);
    }

    /// @brief Libera todos los nodos de la lista.
    ~LinkedList() override { this->clear(); }
};
#endif // __LINKEDLIST_H__
