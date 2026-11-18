#include <iostream>
#include "Demos.h"
#include "containers/containers/vector.h"

using namespace std;

// Compilar asi: make
// Ejecutar asi: ./main
int main() {
    DemoVector();
    DemoRaceCondition();
    DemoLinkedList();
    DemoCircularLinkedList();
    DemoDoublyLinkedList();
    DemoCircularDoublyLinkedList();
    DemoHeap();
    DemoBinaryTree();
    DemoHashTable();
    DemoMerkleTree();
    return 0;
}