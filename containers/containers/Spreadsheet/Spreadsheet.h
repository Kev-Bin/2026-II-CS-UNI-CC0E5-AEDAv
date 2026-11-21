#ifndef __SPREADSHEET_H__
#define __SPREADSHEET_H__

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <cstddef>
#include "ExpressionTree.h"
#include "../Lists/linkedlist.h"

// Identificador de una celda dentro de la hoja: fila * COLS + col.
// Es lo que se guarda en las listas de dependientes.
using CellId = std::size_t;

// Lista enlazada con los dependientes de una celda.
using DependentList = LinkedList<LinkedListAscTraits<CellId>>;

// Resultado de evaluar una celda.
struct CellValue {
    bool        ok = true;
    double      number = 0;
    std::string code;       // si !ok: "#ERROR", "#DIV/0", "#CICLO", "#VALOR", "#REF"
    std::string message;    // si !ok: explicacion larga
};

struct Cell {
    std::string     raw;                    // lo que escribio el usuario
    ExpressionTree* formula    = nullptr;   // nullptr salvo que raw sea una formula valida
    std::string     parseError;             // error de sintaxis (si hubo)
    std::string     parseCode;

    // Ultimo valor calculado. Solo se recalcula cuando cambia esta celda o
    // alguna de las que usa (ver Spreadsheet::Recalculate); imprimir la hoja
    // solo lee este valor.
    CellValue       value;

    // Celdas cuya formula usa a esta celda (sus DEPENDIENTES).
    // Si C1 = =A1+B1, entonces C1 esta en la lista de A1 y en la de B1.
    // Es nullptr hasta que aparece el primer dependiente, y vuelve a nullptr
    // cuando se quita el ultimo.
    DependentList*  dependents = nullptr;

    Cell() = default;

    // La celda es duena de su arbol y de su lista: los borra al destruirse.
    ~Cell() {
        delete formula;
        delete dependents;
    }

    // Copiar la celda copiaria los punteros: dos celdas borrarian el mismo
    // arbol o la misma lista (doble delete). Se prohibe la copia.
    Cell(const Cell&)            = delete;
    Cell& operator=(const Cell&) = delete;

    // Libera el arbol actual (si hay) y deja el puntero en nullptr.
    void clearFormula() {
        delete formula;
        formula = nullptr;
    }

    bool hasDependent(CellId id) const {
        if (!dependents) return false;
        for (auto &node : *dependents)
            if (node.getValue() == id) return true;
        return false;
    }

    // Registra un dependiente (sin repetir: =A1+A1 registra una sola vez).
    void addDependent(CellId id) {
        if (!dependents)
            dependents = new DependentList;     // primer dependiente: recien se crea
        if (!hasDependent(id))
            dependents->pushBack(id, 0);
    }

    void removeDependent(CellId id) {
        if (!hasDependent(id)) return;          // remove() exige que exista
        dependents->remove(id, 0);
        if (dependents->size() == 0) {          // ultimo dependiente: vuelve a nullptr
            delete dependents;
            dependents = nullptr;
        }
    }

    bool empty()     const { return raw.empty(); }
    bool isFormula() const { return !raw.empty() && raw[0] == '='; }

    // Sin contenido y sin dependientes: ya no hace falta que exista.
    // (Una celda vacia puede seguir existiendo solo para guardar su lista,
    //  ej. C1 = =A1+1 con A1 todavia vacia.)
    bool unused()    const { return empty() && !dependents; }
};

template <std::size_t ROWS = 10, std::size_t COLS = 10>
class Spreadsheet {
    static_assert(COLS <= 26, "Por ahora las columnas van de A a Z");

    // Cada posicion es nullptr hasta que se necesita: cuando se le asigna un
    // valor o cuando otra formula la referencia (para guardar su lista de
    // dependientes). Vuelve a nullptr cuando queda sin contenido y sin
    // dependientes. La hoja es duena de sus celdas (new / delete).
    Cell* m_cells[ROWS][COLS] = {};

    std::size_t m_lastRecalculated = 0;

    // Estado de cada celda durante UNA resolucion (un recalculo).
    enum class State { Pending, Visiting, Done };

    // Memoria del recalculo actual:
    //   dirty    : celdas que hay que recalcular (la que cambio y sus
    //              afectadas). Las demas no se tocan: se usa su valor guardado.
    //   Done     : ya se recalculo en esta resolucion (no se repite).
    //   Visiting : esta en la cadena actual de llamadas recursivas; si se
    //              vuelve a ella, hay un ciclo.
    struct Resolution {
        bool      dirty[ROWS][COLS] = {};
        State     state[ROWS][COLS] = {};
        CellValue value[ROWS][COLS];
    };

public:
    // Cada columna ocupa lo mismo que un tab (8 caracteres): 7 para el
    // contenido y 1 para la linea '|' que la separa de la siguiente.
    static constexpr int COLUMN_WIDTH = 8;
    static constexpr int CELL_WIDTH   = COLUMN_WIDTH - 1;

    Spreadsheet() = default;

    ~Spreadsheet() {
        for (std::size_t r = 0; r < ROWS; ++r)
            for (std::size_t c = 0; c < COLS; ++c)
                delete m_cells[r][c];       // delete nullptr no hace nada
    }

    // Copiar la hoja copiaria los punteros: dos hojas borrarian las mismas
    // celdas (doble delete). Se prohibe la copia.
    Spreadsheet(const Spreadsheet&)            = delete;
    Spreadsheet& operator=(const Spreadsheet&) = delete;

    static CellId      IdOf(std::size_t row, std::size_t col) { return row * COLS + col; }
    static std::size_t RowOf(CellId id)                       { return id / COLS; }
    static std::size_t ColOf(CellId id)                       { return id % COLS; }
    static std::string NameOf(CellId id)                      { return Token::CellName(RowOf(id), ColOf(id)); }

    // Convierte "B8" -> (fila=7, col=1). Devuelve false si es invalido.
    static bool ParseRef(const std::string &ref, std::size_t &row, std::size_t &col) {
        if (ref.size() < 2 || !std::isalpha((unsigned char)ref[0]))
            return false;
        int c = std::toupper((unsigned char)ref[0]) - 'A';
        if (c < 0 || (std::size_t)c >= COLS)
            return false;
        std::size_t r = 0;
        for (std::size_t i = 1; i < ref.size(); ++i) {
            if (!std::isdigit((unsigned char)ref[i]))
                return false;
            r = r * 10 + (ref[i] - '0');
            if (r > ROWS) return false;
        }
        if (r < 1) return false;
        row = r - 1;
        col = (std::size_t)c;
        return true;
    }

    // Guarda el texto en la celda. Si es formula, la parsea en ese momento;
    // un error de sintaxis no impide guardarla (la celda mostrara #ERROR).
    // Un valor vacio borra el contenido de la celda.
    //
    // Mantiene las listas de dependientes:
    //   1. Quita esta celda de las listas de las celdas que usaba su formula
    //      anterior.
    //   2. La agrega a las listas de las celdas que usa su formula nueva.
    bool Set(const std::string &ref, const std::string &value) {
        std::size_t r, c;
        if (!ParseRef(ref, r, c)) return false;
        CellId id = IdOf(r, c);

        if (Cell *old = m_cells[r][c])                  // 1. desregistrar la formula anterior
            Unregister(id, *old);

        if (value.empty()) {                            // "B8 =" -> se borra el contenido
            if (Cell *cell = m_cells[r][c]) {
                cell->raw.clear();
                cell->clearFormula();
                cell->parseError.clear();
                cell->parseCode.clear();
            }
            ReleaseIfUnused(r, c);                      // si nadie la usa, vuelve a nullptr
            Recalculate(r, c);                          // sus dependientes ahora ven un 0
            return true;
        }

        Cell &cell = Ensure(r, c);                      // primera asignacion: recien se crea
        cell.raw = value;
        cell.clearFormula();                            // el arbol anterior (si habia) se borra
        cell.parseError.clear();
        cell.parseCode.clear();

        if (cell.isFormula()) {
            try {
                // Si el constructor lanza (error de sintaxis), C++ libera la
                // memoria que reservo este 'new' y formula sigue en nullptr.
                cell.formula = new ExpressionTree(value);
            } catch (const FormulaError &e) {
                cell.parseError = Describe(e, value);
                cell.parseCode  = e.code;
            }
        }

        if (cell.formula)                               // 2. registrar la formula nueva
            Register(id, cell);

        Recalculate(r, c);                              // 3. recalcular solo lo afectado
        return true;
    }

    // Recalcula la celda que cambio y SOLO las celdas afectadas por ella
    // (Affected). Devuelve cuantas celdas se recalcularon.
    //
    // La resolucion sigue siendo recursiva (asi se detectan los ciclos), pero
    // solo baja por las celdas sucias: para cualquier otra celda usa el valor
    // que ya tenia guardado.
    std::size_t Recalculate(std::size_t row, std::size_t col) {
        std::vector<CellId> dirty = Affected(row, col);
        dirty.insert(dirty.begin(), IdOf(row, col));    // la propia celda va primero

        Resolution res;
        for (CellId id : dirty)
            res.dirty[RowOf(id)][ColOf(id)] = true;

        for (CellId id : dirty)                         // orden de recalculo
            Resolve(RowOf(id), ColOf(id), res);

        for (CellId id : dirty)                         // se guardan los valores nuevos
            if (Cell *cell = m_cells[RowOf(id)][ColOf(id)])
                cell->value = res.value[RowOf(id)][ColOf(id)];

        m_lastRecalculated = dirty.size();
        return dirty.size();
    }

    // Cuantas celdas recalculo el ultimo cambio (para la demo).
    std::size_t LastRecalculated() const { return m_lastRecalculated; }

    // Celda en (fila, col), o nullptr si no existe.
    Cell       *Get(std::size_t row, std::size_t col)       { return m_cells[row][col]; }
    const Cell *Get(std::size_t row, std::size_t col) const { return m_cells[row][col]; }

    // Igual, por nombre ("B8"). Tambien nullptr si la referencia es invalida.
    Cell *Get(const std::string &ref) {
        std::size_t r, c;
        if (!ParseRef(ref, r, c)) return nullptr;
        return m_cells[r][c];
    }

    // Trazabilidad: todas las celdas afectadas (directa o indirectamente) si
    // cambia (row, col), en ORDEN DE RECALCULO: cada celda aparece despues de
    // las celdas de las que depende (orden topologico, por DFS sobre las
    // listas de dependientes). No incluye a la propia celda. Si hay ciclos el
    // orden no es topologico (no existe), pero cada celda aparece una vez.
    std::vector<CellId> Affected(std::size_t row, std::size_t col) const {
        bool visited[ROWS * COLS] = {};
        std::vector<CellId> postorder;
        CellId start = IdOf(row, col);

        CollectDependents(start, visited, postorder);

        // El DFS deja cada celda despues de todos sus dependientes; al
        // invertirlo, cada celda queda antes de sus dependientes.
        std::vector<CellId> order(postorder.rbegin(), postorder.rend());
        if (!order.empty() && order.front() == start)
            order.erase(order.begin());                 // la celda que cambio no se cuenta
        return order;
    }

    // Valor guardado de la celda (no recalcula nada). Vacia = 0.
    CellValue Value(std::size_t row, std::size_t col) const {
        const Cell *cell = m_cells[row][col];
        return cell ? cell->value : CellValue{};
    }

    // Formato compacto: una linea por fila, con lineas entre columnas.
    //
    //    |   A   |   B   |   C   | ...
    // ---+-------+-------+-------+
    //  1 | hola ~|       |0.3333 |
    //  8 |       |    72 |       |
    //
    // Imprimir no evalua formulas: solo muestra los valores guardados.
    void Print(std::ostream &os = std::cout) const {
        // Encabezado: la letra centrada sobre el contenido de la columna.
        os << "   |";
        for (std::size_t c = 0; c < COLS; ++c) {
            int left = (CELL_WIDTH - 1) / 2;
            os << std::string(left, ' ') << char('A' + c)
               << std::string(CELL_WIDTH - left - 1, ' ') << '|';
        }
        os << '\n';

        // Linea bajo el encabezado
        os << "---+";
        for (std::size_t c = 0; c < COLS; ++c)
            os << std::string(CELL_WIDTH, '-') << '+';
        os << '\n';

        // Filas
        for (std::size_t r = 0; r < ROWS; ++r) {
            os << std::setw(2) << (r + 1) << " |";
            for (std::size_t c = 0; c < COLS; ++c)
                os << Fit(Display(r, c)) << '|';
            os << '\n';
        }
    }

    static std::string FormatNumber(double v) {
        std::ostringstream os;
        os << std::setprecision(6) << v;    // 72 -> "72", 1/3 -> "0.333333"
        return os.str();
    }

    // Mensaje de error con una flecha bajo la posicion del problema.
    static std::string Describe(const FormulaError &e, const std::string &formula) {
        std::string msg = e.what();
        if (e.pos != std::string::npos && e.pos <= formula.size())
            msg += "\n    " + formula + "\n    " + std::string(e.pos, ' ') + "^";
        return msg;
    }

private:
    // ---------------------- Manejo de celdas y listas ----------------------

    // Devuelve la celda, creandola si todavia era nullptr.
    Cell &Ensure(std::size_t r, std::size_t c) {
        if (!m_cells[r][c])
            m_cells[r][c] = new Cell;
        return *m_cells[r][c];
    }

    // Si la celda ya no tiene contenido ni dependientes, se borra.
    void ReleaseIfUnused(std::size_t r, std::size_t c) {
        if (m_cells[r][c] && m_cells[r][c]->unused()) {
            delete m_cells[r][c];
            m_cells[r][c] = nullptr;
        }
    }

    // Agrega 'id' a la lista de dependientes de cada celda que usa su formula.
    // Las referencias fuera de la hoja se ignoran (al evaluar dan #REF).
    void Register(CellId id, Cell &cell) {
        for (auto &[r, c] : cell.formula->references())
            if (r < ROWS && c < COLS)
                Ensure(r, c).addDependent(id);
    }

    // Quita 'id' de las listas de las celdas que usaba su formula.
    void Unregister(CellId id, Cell &cell) {
        if (!cell.formula) return;
        for (auto &[r, c] : cell.formula->references()) {
            if (r >= ROWS || c >= COLS || !m_cells[r][c]) continue;
            m_cells[r][c]->removeDependent(id);
            if (IdOf(r, c) != id)           // la propia celda se libera en Set
                ReleaseIfUnused(r, c);
        }
    }

    // DFS sobre las listas de dependientes. Cada celda entra a 'postorder'
    // despues de todos sus dependientes. 'visited' evita repetir celdas y
    // evita recorrer un ciclo infinitamente.
    void CollectDependents(CellId id, bool (&visited)[ROWS * COLS], std::vector<CellId> &postorder) const {
        visited[id] = true;
        const Cell *cell = m_cells[RowOf(id)][ColOf(id)];
        if (cell && cell->dependents)
            for (auto &node : *cell->dependents)
                if (!visited[node.getValue()])
                    CollectDependents(node.getValue(), visited, postorder);
        postorder.push_back(id);
    }

    // ------------------------------ Evaluacion -----------------------------

    static CellValue Fail(const std::string &code, const std::string &message) {
        CellValue v;
        v.ok = false;
        v.code = code;
        v.message = message;
        return v;
    }

    // Resolucion recursiva con memoria:
    //   no sucia -> no cambio nada de lo que usa: se devuelve su valor guardado.
    //   Done     -> ya se recalculo en esta resolucion: se devuelve lo nuevo.
    //   Visiting -> se volvio a una celda de la cadena actual: CICLO.
    //   Pending  -> se recalcula (resolviendo antes las celdas que usa).
    CellValue Resolve(std::size_t row, std::size_t col, Resolution &res) const {
        if (!res.dirty[row][col])
            return Value(row, col);

        State &state = res.state[row][col];

        if (state == State::Done)
            return res.value[row][col];

        std::string name = Token::CellName(row, col);
        if (state == State::Visiting)
            return Fail("#CICLO", "Referencia circular que pasa por " + name);

        state = State::Visiting;
        CellValue result = Compute(row, col, res);
        state = State::Done;
        res.value[row][col] = result;
        return result;
    }

    CellValue Compute(std::size_t row, std::size_t col, Resolution &res) const {
        const Cell *cell = m_cells[row][col];
        std::string name = Token::CellName(row, col);

        if (!cell || cell->empty()) return CellValue{};          // vacia = 0

        if (!cell->isFormula()) {
            if (IsNumber(cell->raw)) {
                CellValue v;
                v.number = std::stod(cell->raw);
                return v;
            }
            return Fail("#VALOR", name + " contiene texto, no un numero");
        }

        if (!cell->formula)
            return Fail(cell->parseCode, "Error de sintaxis en " + name + ": " + cell->parseError);

        try {
            CellValue v;
            v.number = cell->formula->evaluate([&](std::size_t r, std::size_t c) {
                if (r >= ROWS || c >= COLS)
                    throw FormulaError(Token::CellName(r, c) + " esta fuera de la hoja",
                                       std::string::npos, "#REF");
                CellValue dep = Resolve(r, c, res);
                if (!dep.ok)    // el error de una dependencia se propaga
                    throw FormulaError(dep.message, std::string::npos, dep.code);
                return dep.number;
            });
            return v;
        } catch (const FormulaError &e) {
            return Fail(e.code, e.pos == std::string::npos
                                    ? std::string(e.what())
                                    : name + ": " + Describe(e, cell->raw));
        }
    }

    // ------------------------------ Impresion ------------------------------

    // Lo que se ve dentro de la celda (a partir del valor guardado).
    std::string Display(std::size_t r, std::size_t c) const {
        const Cell *cell = m_cells[r][c];
        if (!cell || cell->empty())
            return "";
        if (!cell->isFormula())
            return cell->raw;
        const CellValue &v = cell->value;
        return v.ok ? FitNumber(v.number) : v.code;
    }

    // De los 7 caracteres de la celda, 1 queda como margen para que el valor
    // no quede pegado a la linea '|' (si no, parece de la columna vecina).
    static constexpr int CONTENT_WIDTH = CELL_WIDTH - 1;

    // Numero que entra en la celda: reduce decimales hasta que quepa
    // (1/3 -> "0.3333"). Si ni asi entra, muestra "######" como Excel.
    static std::string FitNumber(double v) {
        for (int precision = 6; precision >= 1; --precision) {
            std::ostringstream os;
            os << std::setprecision(precision) << v;
            if ((int)os.str().size() <= CONTENT_WIDTH)
                return os.str();
        }
        return std::string(CONTENT_WIDTH, '#');
    }

    // Ajusta el texto a la celda: numeros a la derecha (margen a la derecha),
    // texto a la izquierda (margen a la izquierda); si no entra, se corta con '~'.
    //   "|     72 |"   "| hola  |"
    static std::string Fit(const std::string &s) {
        std::string text = (int)s.size() > CONTENT_WIDTH
                               ? s.substr(0, CONTENT_WIDTH - 1) + "~"
                               : s;
        std::string pad(CONTENT_WIDTH - text.size(), ' ');
        return IsNumber(text) ? pad + text + " " : " " + text + pad;
    }

    static bool IsNumber(const std::string &s) {
        if (s.empty()) return false;
        try {
            std::size_t pos;
            std::stod(s, &pos);
            return pos == s.size();
        } catch (...) {
            return false;
        }
    }
};

#endif // __SPREADSHEET_H__
