#ifndef __MERKLETREE_H__
#define __MERKLETREE_H__

#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "../vector.h"
#include "../Heap.h"
#include "BinaryTree.h"
#include "../../../types.h"

// Merkle tree como LISTA de hojas: cada hoja se identifica por su indice
// (posicion), que se guarda en su Ref. Las posiciones nunca se corren:
//   - remove deja una lapida (hash EmptyHash) en lugar de quitar la hoja.
//   - push reutiliza la primera lapida libre (la menor de un MinHeap con
//     los indices de las lapidas); si no hay, agrega al final.
//   - update cambia el hash de una hoja en su lugar.
//
// Forma: arbol binario perfecto de altura h (capacidad 2^h), como un vector
// con capacidad. Cuando se llena, grow() duplica la capacidad: la raiz vieja
// pasa a ser el hijo izquierdo y el derecho queda vacio (nullptr).
//
// Hay dos tipos de "vacio":
//   - nullptr : posiciones mas alla del final (nunca usadas). No participan
//               del hash: un padre sin hijo derecho vale lo que su izquierdo.
//   - lapida  : hoja real con hash EmptyHash. Participa en combine.
// Por eso la raiz no depende de la capacidad (grow no la cambia) y coincide
// con la del RFC 6962 para la misma secuencia de posiciones.
// Los nullptr solo aparecen como sufijo derecho: todo nodo interno tiene
// hijo izquierdo y todas las hojas estan a profundidad h.

// --------------------------------------------------
// Hash de los datos de las hojas
// --------------------------------------------------
// No se usa std::hash: su algoritmo depende de la biblioteca estandar (en
// GCC/Clang los enteros dan la identidad, en MSVC FNV-1a), asi que la misma
// lista daria raices distintas segun el compilador. MerkleHasher<T> es
// propio y da el mismo resultado en cualquier plataforma de 64 bits.
// Para otros tipos se pasa un Hasher propio a MerkleTree; si no se pasa,
// MerkleHasher<T> no esta definido y no compila (nada de hashes silenciosos).

// Finalizador de splitmix64: biyectivo y con buena difusion de bits.
// Ojo: es invertible, NO es criptografico.
constexpr MerkleHash merkleMix(MerkleHash x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

// FNV-1a de 64 bits sobre bytes.
constexpr MerkleHash merkleFnv1a(std::string_view bytes) {
    MerkleHash h = 0xcbf29ce484222325ULL;
    for (unsigned char c : bytes) {
        h ^= c;
        h *= 0x100000001b3ULL;
    }
    return h;
}

template <typename T>
struct MerkleHasher;    // sin definicion general a proposito

// Enteros (incluye char y bool): mix del valor. Los con signo se extienden
// en signo al convertirse, igual que hacia std::hash en GCC, asi que las
// raices no cambian respecto de la version anterior.
template <typename T>
    requires std::is_integral_v<T>
struct MerkleHasher<T> {
    constexpr MerkleHash operator()(T value) const { return merkleMix(static_cast<MerkleHash>(value)); }
};

template <>
struct MerkleHasher<std::string_view> {
    constexpr MerkleHash operator()(std::string_view value) const { return merkleMix(merkleFnv1a(value)); }
};

template <>
struct MerkleHasher<std::string> {
    MerkleHash operator()(const std::string& value) const { return MerkleHasher<std::string_view>{}(value); }
};

template <typename T>
struct MerkleTreeTraits : BinaryTreeTraits<AscendingTraits<T>, BinaryTreeNode<MerkleHash>> {};

enum class MerkleChange { Added, Removed, Modified };

// Una hoja que cambio entre dos versiones del arbol.
struct MerkleLeafChange {
    Ref          index;
    MerkleChange kind;
    MerkleHash   before;
    MerkleHash   after;

    bool operator==(const MerkleLeafChange&) const = default;
};

// Hasher: funcion objeto (const value_type&) -> MerkleHash.
template <typename T, typename Hasher = MerkleHasher<T>, typename Traits = MerkleTreeTraits<T>>
class MerkleTree : public BinaryTree<Traits> {

public:

    using Parent     = BinaryTree<Traits>;
    using value_type = typename Parent::value_type;
    using Node       = typename Parent::Node;
    using Hash       = MerkleHash;
    using Changes    = Vector<MerkleLeafChange>;

    // Hash de una hoja eliminada. Es par y hashData siempre es impar,
    // asi que ningun dato puede confundirse con una lapida.
    static constexpr Hash EmptyHash = 0;

protected:

    using Parent::m_pRoot;
    using Parent::m_mutex;

    static constexpr Ref InternalRef = -1;

    size_t m_height = 0;    // capacidad = 2^m_height (si hay raiz)
    size_t m_slots  = 0;    // posiciones usadas (hojas vivas + lapidas)

    MinHeap<size_t> m_free; // indices de las lapidas (valor y Ref = indice)

    // '| 1': el Hasher puede devolver 0 (merkleMix(0) == 0); sin esto ese
    // dato tendria el hash de una lapida.
    static Hash hashData(const value_type& data) { return Hasher{}(data) | 1; }

    static Hash combine(Hash left, Hash right) {
        return left ^ (right + 0x9e3779b97f4a7c15ULL + (left << 6) + (left >> 2));
    }

    static Hash hashOf(const Node* node) { return node ? node->getValue() : EmptyHash; }

    // Hijo derecho nullptr (fuera del final): transparente.
    static Hash join(const Node* left, const Node* right) {
        if (!right) return hashOf(left);
        return combine(hashOf(left), right->getValue());
    }

    static void rehash(Node* node) { node->setValue(join(node->getLeft(), node->getRight())); }

    static const Node* child(const Node* node, bool right) {
        if (!node) return nullptr;
        return right ? node->getRight() : node->getLeft();
    }

    // Hojas del subarbol izquierdo de un subarbol de altura 'height' (> 0).
    static size_t half(size_t height) { return size_t(1) << (height - 1); }

    static bool isLeaf(const Node* node) { return !node->hasLeft() && !node->hasRight(); }
    static bool isEmpty(const Node* leaf) { return leaf->getValue() == EmptyHash; }

    size_t capacity() const { return m_pRoot ? size_t(1) << m_height : 0; }

    // Duplica la capacidad. La raiz no cambia: join(raiz, nullptr) == raiz.
    void grow() {
        Node* root = new Node(hashOf(m_pRoot), InternalRef);
        root->setLeft(m_pRoot);
        m_pRoot = root;
        ++m_height;
    }

    // Crea la hoja 'i' (relativa al subarbol) y los internos que falten en el
    // camino; al volver recalcula los hashes: O(log n).
    static void appendLeaf(Node*& node, size_t height, size_t i, Hash h, Ref ref) {
        if (height == 0) {
            node = new Node(h, ref);
            return;
        }
        if (!node) node = new Node(EmptyHash, InternalRef);

        if (i < half(height)) appendLeaf(node->getLeft(),  height - 1, i,                h, ref);
        else                  appendLeaf(node->getRight(), height - 1, i - half(height), h, ref);

        rehash(node);
    }

    // Baja hasta la hoja 'i'. Solo la cambia si su hash actual es 'expected'
    // (verificacion explicita por hash) y, al volver, recalcula los hashes
    // del camino hasta la raiz: O(log n).
    static bool updateLeaf(Node* node, size_t height, size_t i, Hash expected, Hash newHash) {
        if (!node) return false;

        if (height == 0) {
            if (node->getValue() != expected) return false;
            node->setValue(newHash);
            return true;
        }

        bool ok = (i < half(height))
                ? updateLeaf(node->getLeft(),  height - 1, i,                expected, newHash)
                : updateLeaf(node->getRight(), height - 1, i - half(height), expected, newHash);

        if (ok) rehash(node);
        return ok;
    }

    const Node* leafAt(size_t i) const {
        const Node* node = m_pRoot;
        for (size_t h = m_height; node && h > 0; --h) {
            bool right = i >= half(h);
            if (right) i -= half(h);
            node = child(node, right);
        }
        return node;
    }

    bool validIndex(Ref ref) const { return ref >= 0 && size_t(ref) < m_slots; }

    static MerkleLeafChange makeChange(size_t i, Hash before, Hash after) {
        MerkleChange kind = before == EmptyHash ? MerkleChange::Added
                          : after  == EmptyHash ? MerkleChange::Removed
                          :                       MerkleChange::Modified;
        return {Ref(i), kind, before, after};
    }

    // Subarboles de la misma altura y posicion: se descartan los de igual
    // hash. Un nullptr cuenta como hojas EmptyHash.
    static void diff(const Node* before, const Node* after, size_t height, size_t base, Changes& out) {
        if (!before && !after) return;
        if (before && after && before->getValue() == after->getValue()) return;

        if (height == 0) {
            Hash b = hashOf(before), a = hashOf(after);
            if (b != a) out.pushBack(makeChange(base, b, a), 0);
            return;
        }

        diff(child(before, false), child(after, false), height - 1, base,                out);
        diff(child(before, true),  child(after, true),  height - 1, base + half(height), out);
    }

    // Alturas distintas: el arbol mas bajo equivale al subarbol
    // izquierda-izquierda-... del mas alto; lo que cuelga a la derecha de
    // ese camino se compara contra vacio.
    static void diffAligned(const Node* before, size_t hb, const Node* after, size_t ha, Changes& out) {
        if (hb > ha) {
            diffAligned(child(before, false), hb - 1, after, ha, out);
            diff(child(before, true), nullptr, hb - 1, half(hb), out);
        } else if (ha > hb) {
            diffAligned(before, hb, child(after, false), ha - 1, out);
            diff(nullptr, child(after, true), ha - 1, half(ha), out);
        } else {
            diff(before, after, ha, 0, out);
        }
    }

    static size_t countLeaves(const Node* node) {
        if (!node) return 0;
        if (isLeaf(node)) return 1;
        return countLeaves(node->getLeft()) + countLeaves(node->getRight());
    }

    // Tras copiar nodos: la altura es la del camino izquierdo (la hoja 0
    // siempre existe), las posiciones usadas son las hojas y el heap de
    // libres se rearma con las lapidas (las hojas en orden = por indice).
    void syncShape() {
        m_height = 0;
        for (const Node* node = m_pRoot; node && node->hasLeft(); node = node->getLeft())
            ++m_height;

        m_free.clear();
        m_slots = 0;
        for (auto&& node : this->inOrder()) {
            if (!isLeaf(&node)) continue;
            if (isEmpty(&node)) m_free.push(m_slots, Ref(m_slots));
            ++m_slots;
        }
    }

    // El indice ya no es la Ref del usuario: la posicion la asigna el arbol.
    void internalPush(const value_type& data, Ref) override {
        Hash h = hashData(data);

        if (!m_free.empty()) {                          // reutiliza la menor lapida
            size_t i = m_free.pop().getValue();         // O(log n)
            updateLeaf(m_pRoot, m_height, i, EmptyHash, h);
            return;
        }

        if (m_pRoot && m_slots == capacity()) grow();  // lleno: duplica
        appendLeaf(m_pRoot, m_height, m_slots, h, Ref(m_slots));
        ++m_slots;
    }

    // Deja una lapida. Solo se aplica si la hoja 'ref' contiene 'data'
    // (una lapida nunca coincide: hashData es impar), asi que cada indice
    // entra al heap a lo mas una vez.
    void internalRemove(const value_type& data, Ref ref) override {
        if (validIndex(ref) &&
            updateLeaf(m_pRoot, m_height, size_t(ref), hashData(data), EmptyHash))
            m_free.push(size_t(ref), ref);
    }

    void internalClear() override {
        Parent::internalClear();
        m_free.clear();
        m_height = 0;
        m_slots  = 0;
    }

    // operator= pasa por aqui: Parent no conoce la forma, se recalcula.
    typename Parent::Parent& internalCopy(const Parent& another) override {
        auto& self = Parent::internalCopy(another);
        syncShape();
        return self;
    }

public:

    MerkleTree() = default;

    // El constructor de Parent llama a su propio internalCopy (no al de esta
    // clase), por eso la forma se recalcula aqui.
    MerkleTree(const MerkleTree& another) : Parent(another) { syncShape(); }

    MerkleTree(MerkleTree&& another) : Parent(std::move(another)) {
        m_height = std::exchange(another.m_height, 0);
        m_slots  = std::exchange(another.m_slots, 0);
        m_free   = another.m_free;      // Heap no tiene move
        another.m_free.clear();
    }

    MerkleTree(std::initializer_list<std::pair<value_type, Ref>> values) {
        for (const auto& [data, ref] : values)
            this->push(data, ref);
    }

    MerkleTree& operator=(const MerkleTree& another) {
        this->copy(another);    // Container::operator= -> internalCopy de esta clase
        return *this;
    }

    Hash root() const {
        std::scoped_lock lock(m_mutex);
        return m_pRoot ? m_pRoot->getValue() : 0;
    }

    // Modifica la hoja 'ref' solo si hoy contiene 'oldData'.
    bool update(const value_type& oldData, Ref ref, const value_type& newData) {
        std::scoped_lock lock(m_mutex);
        return validIndex(ref) &&
               updateLeaf(m_pRoot, m_height, size_t(ref), hashData(oldData), hashData(newData));
    }

    // true si la hoja 'ref' contiene exactamente 'data'.
    bool verify(const value_type& data, Ref ref) const {
        std::scoped_lock lock(m_mutex);
        return validIndex(ref) && leafAt(size_t(ref))->getValue() == hashData(data);
    }

    bool isRemoved(Ref ref) const {
        std::scoped_lock lock(m_mutex);
        return validIndex(ref) && isEmpty(leafAt(size_t(ref)));
    }

    // Hojas que cambiaron respecto de 'snapshot' (una copia anterior del
    // arbol). Siempre poda por hash, aunque la capacidad haya cambiado.
    Changes changesSince(const MerkleTree& snapshot) const {
        Changes out;
        if (&snapshot == this) return out;

        std::scoped_lock lock(m_mutex, snapshot.m_mutex);
        diffAligned(snapshot.m_pRoot, snapshot.m_height, m_pRoot, m_height, out);
        return out;
    }

    // Hojas vivas (sin contar lapidas).
    size_t leafCount() {
        std::scoped_lock lock(m_mutex);
        size_t count = 0;
        for (auto&& node : this->inOrder())
            count += isLeaf(&node) && !isEmpty(&node);
        return count;
    }

    // Indice de la lapida que reutilizaria el proximo push, o slotCount()
    // si no hay ninguna (el push agregaria al final).
    size_t nextFreeSlot() const {
        std::scoped_lock lock(m_mutex);
        if (m_free.empty()) return m_slots;
        return m_free.cbegin()->getValue();     // raiz del MinHeap
    }

    // Posiciones usadas (hojas vivas + lapidas).
    size_t slotCount() const {
        std::scoped_lock lock(m_mutex);
        return m_slots;
    }

    // Posiciones disponibles antes del proximo grow (2^altura).
    size_t capacityCount() const {
        std::scoped_lock lock(m_mutex);
        return capacity();
    }
};

#endif // __MERKLETREE_H__
