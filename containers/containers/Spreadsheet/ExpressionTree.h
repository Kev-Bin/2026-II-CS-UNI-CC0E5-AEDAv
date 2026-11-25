#ifndef __EXPRESSIONTREE_H__
#define __EXPRESSIONTREE_H__

#include <cctype>
#include <cstddef>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "../Trees/BinaryTree.h"
#include "../../IO/NodeIO.h"

// ---------------------------------------------------------------------------
// Arbol de expresion para las formulas de la hoja de calculo.
//
//   "=A1 + 2 * (B3 - 1)"   ->   +
//                               |-- A1
//                               `-- *
//                                   |-- 2
//                                   `-- -
//                                       |-- B3
//                                       `-- 1
//
// - Hojas:          numeros y referencias a celdas.
// - Nodos internos: operadores binarios (+ - * /) con dos hijos, o el menos
//                   unario (Negate) con un solo hijo (el izquierdo).
// - El Ref de cada nodo es la posicion del token dentro del texto de la
//   formula (sirve para mensajes de error y se ve al imprimir el arbol).
//
// Se reutiliza BinaryTree: guarda la raiz, copia, limpia, imprime y da los
// recorridos. Lo propio de esta clase es como se construye (parse) y como se
// evalua (recorrido en postorden = notacion polaca inversa).
// ---------------------------------------------------------------------------

// Error de sintaxis o de evaluacion.
//   pos  : posicion en el texto de la formula (npos si no aplica).
//   code : texto corto que se muestra dentro de la celda (#ERROR, #DIV/0 ...).
struct FormulaError : std::runtime_error {
    std::size_t pos;
    std::string code;
    FormulaError(const std::string& msg, std::size_t p = std::string::npos,
                 const std::string& c = "#ERROR")
        : std::runtime_error(msg), pos(p), code(c) {}
};

// ---------------------------- Operaciones ---------------------------------
// Tabla de operaciones binarias: un arreglo de punteros a funcion. El nodo
// del arbol no guarda el simbolo sino el INDICE de la operacion; al evaluar
// se llama directamente a Operations[indice](a, b), sin switch.
//
//   indice:   0     1     2     3
//   simbolo: '+'   '-'   '*'   '/'
//
// Para agregar una operacion (ej. potencia '^'): se agrega su funcion, su
// simbolo y su indice en el enum, y se ubica en la regla de la gramatica que
// le corresponda segun su precedencia.

using BinaryOperation = double (*)(double, double);

enum OpIndex : std::size_t {
    OP_ADD = 0,
    OP_SUB = 1,
    OP_MUL = 2,
    OP_DIV = 3,
    OP_COUNT            // cantidad de operaciones (tamaño de las tablas)
};

double OpAdd(double a, double b) { return a + b; }
double OpSub(double a, double b) { return a - b; }
double OpMul(double a, double b) { return a * b; }
double OpDiv(double a, double b) {
    if (b == 0) throw FormulaError("Division entre cero", std::string::npos, "#DIV/0");
    return a / b;
}

constexpr BinaryOperation Operations[OP_COUNT] = { OpAdd, OpSub, OpMul, OpDiv };
constexpr char            OpSymbols [OP_COUNT] = { '+',   '-',   '*',   '/'   };

// Simbolo -> indice en la tabla (OP_COUNT si no es un operador).
std::size_t OpIndexOf(char symbol) {
    for (std::size_t i = 0; i < OP_COUNT; ++i)
        if (OpSymbols[i] == symbol) return i;
    return OP_COUNT;
}

// ------------------------------- Token ------------------------------------

struct Token {
    enum class Kind { Number, CellRef, BinaryOp, Negate };

    Kind        kind   = Kind::Number;
    double      number = 0;     // Kind::Number
    std::size_t op     = 0;     // Kind::BinaryOp: indice en Operations[] (OP_ADD, OP_SUB ...)
    std::size_t row    = 0;     // Kind::CellRef (base 0)
    std::size_t col    = 0;     // Kind::CellRef (base 0)

    static Token Num(double v)                      { Token t; t.kind = Kind::Number;   t.number = v;        return t; }
    static Token Cell(std::size_t r, std::size_t c) { Token t; t.kind = Kind::CellRef;  t.row = r; t.col = c; return t; }
    static Token Op(std::size_t index)              { Token t; t.kind = Kind::BinaryOp; t.op = index;        return t; }
    static Token Neg()                              { Token t; t.kind = Kind::Negate;                        return t; }

    char symbol() const { return OpSymbols[op]; }

    // "B8" a partir de (fila 7, col 1)
    static std::string CellName(std::size_t r, std::size_t c) {
        return std::string(1, char('A' + c)) + std::to_string(r + 1);
    }

    std::string toString() const {
        switch (kind) {
            case Kind::Number: {
                std::ostringstream os;
                os << number;   // 72 -> "72", 2.5 -> "2.5"
                return os.str();
            }
            case Kind::CellRef:  return CellName(row, col);
            case Kind::BinaryOp: return std::string(1, symbol()) + "[" + std::to_string(op) + "]";  // "+[0]"
            case Kind::Negate:   return "neg";
        }
        return "?";
    }
};

std::ostream& operator<<(std::ostream& os, const Token& t) { return os << t.toString(); }

// --------------------------- ExpressionTree -------------------------------

struct ExpressionTreeTraits : BinaryTreeTraits<AscendingTraits<Token>, BinaryTreeNode<Token>> {};

class ExpressionTree : public BinaryTree<ExpressionTreeTraits> {

public:

    using Parent     = BinaryTree<ExpressionTreeTraits>;
    using value_type = typename Parent::value_type;     // Token
    using Node       = typename Parent::Node;           // BinaryTreeNode<Token>

    // Devuelve el valor de la celda (fila, col). Puede lanzar FormulaError.
    using CellLookup = std::function<double(std::size_t, std::size_t)>;

protected:

    using Parent::m_pRoot;
    using Parent::m_mutex;

    // El arbol no se arma con push: su forma la decide la gramatica.
    void internalPush(const value_type&, Ref) override {
        throw std::logic_error("ExpressionTree: use parse() en lugar de push()");
    }
    void internalRemove(const value_type&, Ref) override {
        throw std::logic_error("ExpressionTree: no admite remove()");
    }

public:

    ExpressionTree() = default;
    explicit ExpressionTree(const std::string& formula) { parse(formula); }
    ExpressionTree(const ExpressionTree& another) : Parent(another) {}

    // Construye el arbol a partir del texto (con o sin '=' inicial).
    // Si hay error de sintaxis lanza FormulaError y el arbol queda vacio.
    void parse(const std::string& formula) {
        std::scoped_lock lock(m_mutex);
        this->internalClear();

        Parser p(formula);
        m_pRoot = p.parseFormula();
    }

    // Evalua recorriendo el arbol en POSTORDEN: cada nodo se visita despues
    // de sus hijos, asi que sus operandos ya estan en la pila (igual que
    // evaluar notacion polaca inversa).
    double evaluate(const CellLookup& lookup) {
        if (!m_pRoot)
            throw FormulaError("Formula vacia");

        Vector<double> stack;
        auto pop = [&]() { return stack.pop_back().getValue(); };

        for (auto&& node : this->postOrder()) {
            const Token& t = node.getValue();
            switch (t.kind) {
                case Token::Kind::Number:  stack.pushBack(t.number, 0);             break;
                case Token::Kind::CellRef: stack.pushBack(lookup(t.row, t.col), 0); break;
                case Token::Kind::Negate:  stack.pushBack(-pop(), 0);               break;
                case Token::Kind::BinaryOp: {
                    double b = pop();       // derecho (se apilo al final)
                    double a = pop();       // izquierdo
                    try {
                        // Despacho por indice: el nodo guarda que funcion aplicar.
                        stack.pushBack(Operations[t.op](a, b), 0);
                    } catch (FormulaError& e) {
                        if (e.pos == std::string::npos)
                            e.pos = std::size_t(node.getRef());   // ubica el error en el operador
                        throw;
                    }
                    break;
                }
            }
        }
        return pop();
    }

    // Celdas que aparecen en la formula (con repeticiones), en preorden.
    // Es lo que se usara para armar las listas de dependencias.
    std::vector<std::pair<std::size_t, std::size_t>> references() {
        std::vector<std::pair<std::size_t, std::size_t>> refs;
        for (auto&& node : this->preOrder())
            if (node.getValue().kind == Token::Kind::CellRef)
                refs.emplace_back(node.getValue().row, node.getValue().col);
        return refs;
    }

    // Reconstruye la formula en infijo con todos los parentesis, recorriendo
    // en INORDEN (izquierdo, raiz, derecho). Util para ver como se agrupo.
    std::string toInfix() const { return infix(m_pRoot); }

private:

    static std::string infix(const Node* n) {
        if (!n) return "";
        const Token& t = n->getValue();
        switch (t.kind) {
            case Token::Kind::Number:
            case Token::Kind::CellRef:  return t.toString();
            case Token::Kind::Negate:   return "-" + infix(n->getLeft());
            case Token::Kind::BinaryOp: return "(" + infix(n->getLeft()) + " " + t.symbol() + " " + infix(n->getRight()) + ")";
        }
        return "?";
    }

    // ------------------------------------------------------------------
    // Parser de descenso recursivo. Gramatica (de menor a mayor precedencia):
    //
    //   formula := ['='] expr FIN
    //   expr    := term   (('+' | '-') term)*       asociativo a la izquierda
    //   term    := unary  (('*' | '/') unary)*      asociativo a la izquierda
    //   unary   := ('-' | '+') unary | primary
    //   primary := NUMERO | CELDA | '(' expr ')'
    //
    // Cada regla devuelve el subarbol que reconocio. La precedencia sale de
    // la propia gramatica: '*' queda mas abajo en el arbol que '+', por eso
    // se evalua antes.
    // ------------------------------------------------------------------
    class Parser {
        const std::string& m_text;
        std::size_t        m_pos = 0;

    public:
        explicit Parser(const std::string& text) : m_text(text) {}

        Node* parseFormula() {
            skipSpaces();
            if (peek() == '=') { ++m_pos; }

            skipSpaces();
            if (atEnd()) throw FormulaError("Formula vacia", m_pos);

            Node* root = parseExpr();
            skipSpaces();
            if (!atEnd()) {
                destroy(root);
                throw FormulaError(std::string("Caracter inesperado '") + peek() + "'", m_pos);
            }
            return root;
        }

    private:
        bool atEnd() const { return m_pos >= m_text.size(); }
        char peek()  const { return atEnd() ? '\0' : m_text[m_pos]; }

        void skipSpaces() {
            while (!atEnd() && std::isspace((unsigned char)m_text[m_pos])) ++m_pos;
        }

        static Node* makeNode(const Token& t, std::size_t pos, Node* left = nullptr, Node* right = nullptr) {
            Node* n = new Node(t, Ref(pos));
            n->setLeft(left);
            n->setRight(right);
            return n;
        }

        // Libera un subarbol parcial cuando hay error a mitad del parseo.
        static void destroy(Node* n) {
            if (!n) return;
            destroy(n->getLeft());
            destroy(n->getRight());
            delete n;
        }

        // Ejecuta una regla; si falla, libera 'pending' antes de propagar.
        template <typename Rule>
        Node* parseOrFree(Rule rule, Node* pending) {
            try { return (this->*rule)(); }
            catch (...) { destroy(pending); throw; }
        }

        // Patron comun de expr y term: operando (OP operando)*, donde OP es
        // una de las dos operaciones permitidas en ese nivel de precedencia.
        // El simbolo leido se traduce a su indice en Operations[] y ese
        // indice es lo que se guarda en el nodo.
        Node* parseBinaryLevel(Node* (Parser::*operand)(), std::size_t opA, std::size_t opB) {
            Node* left = (this->*operand)();
            while (true) {
                skipSpaces();
                std::size_t index = OpIndexOf(peek());
                if (index != opA && index != opB) return left;
                std::size_t opPos = m_pos++;
                Node* right = parseOrFree(operand, left);
                left = makeNode(Token::Op(index), opPos, left, right);  // (left op right)
            }
        }

        // expr := term (('+' | '-') term)*
        Node* parseExpr() { return parseBinaryLevel(&Parser::parseTerm, OP_ADD, OP_SUB); }

        // term := unary (('*' | '/') unary)*
        Node* parseTerm() { return parseBinaryLevel(&Parser::parseUnary, OP_MUL, OP_DIV); }

        // unary := ('-' | '+') unary | primary
        Node* parseUnary() {
            skipSpaces();
            if (peek() == '-') {
                std::size_t opPos = m_pos++;
                return makeNode(Token::Neg(), opPos, parseUnary());
            }
            if (peek() == '+') {            // '+' unario: no hace nada
                ++m_pos;
                return parseUnary();
            }
            return parsePrimary();
        }

        // primary := NUMERO | CELDA | '(' expr ')'
        Node* parsePrimary() {
            skipSpaces();
            std::size_t start = m_pos;
            char c = peek();

            if (atEnd())
                throw FormulaError("Falta un operando al final", m_pos);

            if (c == '(') {
                ++m_pos;
                Node* inner = parseExpr();
                skipSpaces();
                if (peek() != ')') {
                    destroy(inner);
                    throw FormulaError("Falta ')'", m_pos);
                }
                ++m_pos;
                return inner;   // los parentesis solo agrupan: no generan nodo
            }

            if (std::isdigit((unsigned char)c) || c == '.')
                return makeNode(Token::Num(readNumber()), start);

            if (std::isalpha((unsigned char)c))
                return readCell(start);

            throw FormulaError(std::string("Caracter inesperado '") + c + "'", m_pos);
        }

        double readNumber() {
            std::size_t start = m_pos;
            bool dot = false;
            while (!atEnd() && (std::isdigit((unsigned char)peek()) || (peek() == '.' && !dot))) {
                if (peek() == '.') dot = true;
                ++m_pos;
            }
            std::string s = m_text.substr(start, m_pos - start);
            if (s == ".") throw FormulaError("Numero invalido", start);
            return std::stod(s);
        }

        // CELDA := LETRA DIGITO+    (A1, b8, J10 ...)
        Node* readCell(std::size_t start) {
            std::size_t col = std::toupper((unsigned char)peek()) - 'A';
            ++m_pos;
            if (!std::isdigit((unsigned char)peek()))
                throw FormulaError("Referencia invalida: se esperaba un numero de fila", m_pos);

            std::size_t row = 0;
            while (std::isdigit((unsigned char)peek())) {
                row = row * 10 + (peek() - '0');
                ++m_pos;
                if (row > 1000000) throw FormulaError("Fila demasiado grande", start);
            }
            if (row == 0) throw FormulaError("Las filas empiezan en 1", start);
            return makeNode(Token::Cell(row - 1, col), start);
        }
    };
};

#endif // __EXPRESSIONTREE_H__
