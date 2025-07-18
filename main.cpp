#include "ui.h"
#include <iostream>

int main() {
    std::cout << "Sistema de Detección de Conceptos Clave en Literatura Educativa\n";
    std::cout << "Implementación del Autómata Aho-Corasick Mejorado\n";
    std::cout << "=============================================================\n";

    try {
        ui::interactive_menu();
    } catch (const std::exception& e) {
        std::cerr << "Error crítico: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
