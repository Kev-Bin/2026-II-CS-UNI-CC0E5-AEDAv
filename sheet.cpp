#include <iostream>
#include <string>
#include <vector>
#include "containers/containers/Spreadsheet/Spreadsheet.h"

using namespace std;

// Hoja de calculo interactiva (ejecutable aparte porque lee de stdin).
// Compilar: make sheet    Ejecutar: ./sheet
//
// Comandos:
//   B8 = 72            numero
//   A1 = hola          texto
//   C1 = =A1+B8*2      formula (el valor empieza con '=')
//   B8 =               borra la celda
//   ver C1             muestra la formula, su arbol, sus dependientes y su valor
//   afecta A1          celdas afectadas si cambia A1 (orden de recalculo)
//   ayuda | salir

using Sheet = Spreadsheet<10, 10>;

static string Trim(const string &s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static void Help() {
    cout << "Comandos:\n"
            "  B8 = 72          numero\n"
            "  A1 = hola        texto\n"
            "  C1 = =A1+B8*2    formula (+ - * / y parentesis)\n"
            "  B8 =             borra la celda\n"
            "  ver C1           arbol de la formula, dependientes y valor\n"
            "  afecta A1        celdas afectadas si cambia A1\n"
            "  salir\n";
}

// Imprime una lista de celdas: " C1 D1 E1" o " ninguna".
static void PrintCells(const vector<CellId> &cells) {
    if (cells.empty()) cout << " ninguna";
    for (CellId id : cells) cout << ' ' << Sheet::NameOf(id);
    cout << '\n';
}

// Dependientes directos de una celda: el contenido de su lista enlazada.
static void PrintDependents(const Cell &cell) {
    cout << "Dependientes (lista enlazada):";
    if (!cell.dependents) {
        cout << " nullptr\n";
        return;
    }
    for (auto &node : *cell.dependents)
        cout << ' ' << Sheet::NameOf(node.getValue());
    cout << '\n';
}

// Trazabilidad: todas las celdas que cambian si cambia 'ref'.
static void ShowAffected(Sheet &sheet, const string &ref) {
    size_t r, c;
    if (!Sheet::ParseRef(ref, r, c)) {
        cout << "Celda invalida: '" << ref << "'. Use A1..J10.\n";
        return;
    }
    cout << "Si cambia " << Token::CellName(r, c) << " se recalculan:";
    PrintCells(sheet.Affected(r, c));
}

// Muestra el detalle de una celda: texto, arbol de expresion, dependientes y valor.
static void Show(Sheet &sheet, const string &ref) {
    size_t r, c;
    if (!Sheet::ParseRef(ref, r, c)) {
        cout << "Celda invalida: '" << ref << "'. Use A1..J10.\n";
        return;
    }
    string name = Token::CellName(r, c);
    Cell *pCell = sheet.Get(r, c);

    if (!pCell) {                       // nullptr: nadie la uso ni la referencia
        cout << name << " esta vacia (nullptr).\n";
        return;
    }
    Cell &cell = *pCell;
    if (cell.empty())                   // existe solo para guardar sus dependientes
        cout << name << " esta vacia (existe porque otras formulas la usan).\n";
    else
        cout << name << " = " << cell.raw << '\n';
    PrintDependents(cell);

    if (cell.isFormula()) {
        if (!cell.formula) {
            cout << "Error de sintaxis: " << cell.parseError << '\n';
            return;
        }
        cout << "Agrupacion: " << cell.formula->toInfix() << '\n';
        cout << "Arbol (nodo,posicion en el texto):\n" << *cell.formula;
        cout << "Usa (referencias):";
        auto refs = cell.formula->references();
        if (refs.empty()) cout << " ninguna";
        for (auto &[rr, cc] : refs) cout << ' ' << Token::CellName(rr, cc);
        cout << '\n';
    }

    CellValue v = sheet.Value(r, c);
    if (v.ok) cout << "Valor: " << Sheet::FormatNumber(v.number) << '\n';
    else      cout << "Valor: " << v.code << "  (" << v.message << ")\n";
}

int main() {
    Sheet sheet;
    sheet.Print();
    cout << "Escriba 'ayuda' para ver los comandos.\n";

    string line;
    while (true) {
        cout << "> ";
        if (!getline(cin, line)) break;
        line = Trim(line);
        if (line.empty()) continue;
        if (line == "salir" || line == "exit" || line == "q") break;
        if (line == "ayuda" || line == "help") { Help(); continue; }

        if (line.rfind("ver ", 0) == 0) {
            Show(sheet, Trim(line.substr(4)));
            continue;
        }
        if (line.rfind("afecta ", 0) == 0) {
            ShowAffected(sheet, Trim(line.substr(7)));
            continue;
        }

        size_t eq = line.find('=');
        if (eq == string::npos) {
            cout << "Formato: <Celda> = <valor>   (ej. B8 = 72). 'ayuda' para mas.\n";
            continue;
        }
        string ref   = Trim(line.substr(0, eq));
        string value = Trim(line.substr(eq + 1));

        if (!sheet.Set(ref, value)) {
            cout << "Celda invalida: '" << ref << "'. Use A1..J10.\n";
            continue;
        }

        Cell *cell = sheet.Get(ref);    // nullptr si se acaba de borrar
        if (cell && cell->isFormula() && !cell->formula)
            cout << "Error de sintaxis: " << cell->parseError << '\n';

        sheet.Print();

        // Trazabilidad del cambio: que otras celdas se vieron afectadas.
        size_t r, c;
        Sheet::ParseRef(ref, r, c);
        vector<CellId> affected = sheet.Affected(r, c);
        if (!affected.empty()) {
            cout << "Afectadas por " << Token::CellName(r, c) << ":";
            PrintCells(affected);
        }
        cout << "Celdas recalculadas: " << sheet.LastRecalculated() << '\n';
    }
    return 0;
}
