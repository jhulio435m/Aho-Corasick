#include "ui.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace ui {

void generate_summary(const std::vector<ahocorasick::MatchResult>& results,
                      const std::vector<std::string>& patterns) {
    if (results.empty()) {
        std::cout << "No se encontraron coincidencias para generar resumen.\n";
        return;
    }

    std::unordered_map<ahocorasick::PatternID, size_t> pattern_counts;
    for (const auto& r : results) {
        pattern_counts[r.pattern_id]++;
    }

    std::cout << "\n=== RESUMEN ESTADÍSTICO ===\n";
    std::cout << "Total de coincidencias: " << results.size() << "\n";
    std::cout << "Coincidencias por patrón:\n";
    for (const auto& kv : pattern_counts) {
        std::cout << " - " << std::setw(30) << std::left << patterns[kv.first]
                  << ": " << kv.second << " coincidencias\n";
    }
    if (!results.empty()) {
        size_t min_line = results.front().line;
        size_t max_line = results.back().line;
        std::cout << "\nDistribución desde línea " << min_line << " hasta "
                  << max_line << "\n";
    }
}

void display_results(const std::vector<ahocorasick::MatchResult>& results,
                     bool show_context) {
    if (results.empty()) {
        std::cout << "No se encontraron coincidencias.\n";
        return;
    }
    std::cout << "\n=== RESULTADOS DETALLADOS ===\n";
    std::cout << "Coincidencias encontradas: " << results.size() << "\n\n";
    for (const auto& result : results) {
        std::cout << "Línea " << std::setw(4) << result.line
                  << ", Columna " << std::setw(4) << result.column
                  << ": \"" << result.pattern << "\"";
        if (show_context) {
            std::cout << "\n   Contexto: \"" << result.context << "\"";
        }
        std::cout << "\n";
    }
}

void export_to_html(const std::vector<ahocorasick::MatchResult>& results,
                    const std::vector<std::string>& patterns,
                    const std::string& output_path) {
    std::ofstream out_file(output_path);
    if (!out_file) {
        throw std::runtime_error("No se pudo abrir el archivo para escritura");
    }

    out_file << "<!DOCTYPE html>\n<html lang='es'>\n<head>\n"
             << "<meta charset='UTF-8'>\n"
             << "<title>Resultados de Análisis</title>\n"
             << "<style>\n"
             << "body { font-family: Arial, sans-serif; line-height: 1.6; }\n"
             << ".match { margin-bottom: 15px; border-left: 3px solid #3498db; padding-left: 10px; }\n"
             << ".pattern { font-weight: bold; color: #e74c3c; }\n"
             << ".context { color: #7f8c8d; font-style: italic; }\n"
             << ".summary { background-color: #f9f9f9; padding: 15px; margin-bottom: 20px; }\n"
             << "</style>\n</head>\n<body>\n"
             << "<h1>Resultados de Análisis de Texto</h1>\n"
             << "<div class='summary'>\n"
             << "<h2>Resumen</h2>\n"
             << "<p>Total de coincidencias: " << results.size() << "</p>\n";

    std::unordered_map<ahocorasick::PatternID, size_t> pattern_counts;
    for (const auto& r : results) {
        pattern_counts[r.pattern_id]++;
    }

    out_file << "<h3>Coincidencias por patrón:</h3>\n<ul>\n";
    for (const auto& kv : pattern_counts) {
        out_file << "<li>" << patterns[kv.first] << ": " << kv.second << " coincidencias</li>\n";
    }
    out_file << "</ul>\n</div>\n";

    out_file << "<h2>Detalles de Coincidencias</h2>\n";
    for (const auto& result : results) {
        out_file << "<div class='match'>\n"
                 << "<p><strong>Línea " << result.line << ", Columna " << result.column << ":</strong> "
                 << "Patrón: <span class='pattern'>" << result.pattern << "</span></p>\n"
                 << "<p class='context'>Contexto: \"" << result.context << "\"</p>\n"
                 << "</div>\n";
    }

    out_file << "</body>\n</html>\n";
    std::cout << "Resultados exportados a " << output_path << "\n";
}

std::vector<std::string> load_patterns_from_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo de patrones: " + file_path);
    }
    std::vector<std::string> patterns;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            patterns.push_back(line);
        }
    }
    if (patterns.empty()) {
        throw std::runtime_error("El archivo de patrones está vacío o no contiene patrones válidos");
    }
    return patterns;
}

std::string load_text_from_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo de texto: " + file_path);
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

void interactive_menu() {
    bool verbose = true;
    bool case_sensitive = false;
    ahocorasick::PatternMatcher matcher(verbose, case_sensitive);
    std::vector<std::string> patterns;
    std::string text;
    std::vector<ahocorasick::MatchResult> last_results;
    size_t context_size = 20;

    auto print_status = [&]() {
        std::cout << "\n=== ESTADO ACTUAL ===\n";
        std::cout << "Modo verboso: " << (verbose ? "ON" : "OFF") << "\n";
        std::cout << "Sensibilidad a mayúsculas: " << (case_sensitive ? "ON" : "OFF") << "\n";
        std::cout << "Patrones cargados: " << matcher.patterns().size() << "\n";
        std::cout << "Tamaño del texto: " << text.size() << " caracteres\n";
        std::cout << "Tamaño del contexto: " << context_size << " caracteres\n";
        std::cout << "Última búsqueda: " << last_results.size() << " coincidencias\n\n";
    };

    while (true) {
        print_status();

        std::cout << "=== MENÚ PRINCIPAL ===\n"
                  << "1. Cargar patrones desde archivo\n"
                  << "2. Ingresar patrones manualmente\n"
                  << "3. Cargar texto desde archivo\n"
                  << "4. Ingresar texto manualmente\n"
                  << "5. Configurar opciones\n"
                  << "6. Buscar patrones en el texto\n"
                  << "7. Mostrar resultados\n"
                  << "8. Generar resumen estadístico\n"
                  << "9. Exportar resultados a HTML\n"
                  << "0. Salir\n"
                  << "Seleccione una opción: ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Entrada inválida. Por favor ingrese un número.\n";
            continue;
        }
        std::cin.ignore();

        try {
            switch (choice) {
                case 1: {
                    std::cout << "Ingrese la ruta del archivo de patrones: ";
                    std::string path;
                    std::getline(std::cin, path);
                    patterns = load_patterns_from_file(path);
                    matcher.initialize(patterns);
                    break;
                }
                case 2: {
                    patterns.clear();
                    std::cout << "Ingrese los patrones (línea vacía para terminar):\n";
                    std::string line;
                    while (std::getline(std::cin, line) && !line.empty()) {
                        patterns.push_back(line);
                    }
                    if (patterns.empty()) {
                        std::cout << "No se ingresaron patrones.\n";
                        continue;
                    }
                    matcher.initialize(patterns);
                    break;
                }
                case 3: {
                    std::cout << "Ingrese la ruta del archivo de texto: ";
                    std::string path;
                    std::getline(std::cin, path);
                    text = load_text_from_file(path);
                    std::cout << "Texto cargado (" << text.size() << " caracteres)\n";
                    break;
                }
                case 4: {
                    std::cout << "Ingrese el texto (escriba 'FIN' en una línea para terminar):\n";
                    text.clear();
                    std::string line;
                    while (std::getline(std::cin, line)) {
                        if (line == "FIN") break;
                        text += line + "\n";
                    }
                    break;
                }
                case 5: {
                    std::cout << "1. " << (verbose ? "Desactivar" : "Activar") << " modo verboso\n"
                              << "2. " << (case_sensitive ? "Desactivar" : "Activar")
                              << " sensibilidad a mayúsculas\n"
                              << "3. Definir tamaño del contexto (actual: " << context_size << ")\n"
                              << "Opción: ";
                    int opt;
                    std::cin >> opt;
                    if (opt == 1) {
                        verbose = !verbose;
                    } else if (opt == 2) {
                        case_sensitive = !case_sensitive;
                    } else if (opt == 3) {
                        std::cout << "Nuevo tamaño de contexto: ";
                        std::cin >> context_size;
                    }
                    matcher = ahocorasick::PatternMatcher(verbose, case_sensitive);
                    if (!patterns.empty()) matcher.initialize(patterns);
                    break;
                }
                case 6: {
                    if (matcher.patterns().empty() || text.empty()) {
                        std::cout << "Error: Debe cargar patrones y texto primero.\n";
                        break;
                    }
                    last_results = matcher.search(text, context_size);
                    std::cout << "Búsqueda completada. " << last_results.size()
                              << " coincidencias encontradas.\n";
                    break;
                }
                case 7: {
                    if (last_results.empty()) {
                        std::cout << "No hay resultados para mostrar.\n";
                        break;
                    }
                    display_results(last_results);
                    break;
                }
                case 8: {
                    if (last_results.empty()) {
                        std::cout << "No hay resultados para generar resumen.\n";
                        break;
                    }
                    generate_summary(last_results, matcher.patterns());
                    break;
                }
                case 9: {
                    if (last_results.empty()) {
                        std::cout << "No hay resultados para exportar.\n";
                        break;
                    }
                    std::cout << "Ingrese la ruta de salida para el HTML: ";
                    std::string path;
                    std::getline(std::cin, path);
                    export_to_html(last_results, matcher.patterns(), path);
                    break;
                }
                case 0: {
                    return;
                }
                default: {
                    std::cout << "Opción no válida. Intente nuevamente.\n";
                    break;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
}

} // namespace ui
