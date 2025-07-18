#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include <locale>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <limits>

namespace ahocorasick {

// Constantes y tipos básicos mejorados
static constexpr int ALPHABET_SIZE = 28; // 26 letras + espacio + guión
using TimeDuration = std::chrono::milliseconds;
using HighResClock = std::chrono::high_resolution_clock;
using PatternID = size_t;

/**
 * Convierte un carácter a su índice correspondiente en el Trie con soporte para más caracteres
 */
inline int char_to_index(char c) {
    if (c == ' ') return 26;
    if (c == '-') return 27;
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= 'A' && c <= 'Z') return std::tolower(c) - 'a';
    return -1; // Carácter fuera del alfabeto admitido
}

/**
 * Estructura de resultado mejorada con más información contextual
 */
struct MatchResult {
    size_t line;
    size_t column;
    std::string pattern;
    std::string context; // Texto alrededor de la coincidencia
    PatternID pattern_id;
    
    // Para ordenamiento de resultados
    bool operator<(const MatchResult& other) const {
        return std::tie(line, column, pattern_id) < 
               std::tie(other.line, other.column, other.pattern_id);
    }
};

/**
 * Nodo del Trie optimizado
 */
class TrieNode {
public:
    std::array<std::unique_ptr<TrieNode>, ALPHABET_SIZE> children;
    bool is_end_of_word = false;
    TrieNode* failure_link = nullptr;
    TrieNode* output_link = nullptr;
    std::vector<PatternID> pattern_indices;
    int depth = 0; // Profundidad del nodo en el trie

    TrieNode() = default;
    ~TrieNode() = default;

    // Eliminar copias no deseadas
    TrieNode(const TrieNode&) = delete;
    TrieNode& operator=(const TrieNode&) = delete;
};

/**
 * Clase principal mejorada con más funcionalidades
 */
class PatternMatcher {
public:
    explicit PatternMatcher(bool verbose = false, bool case_sensitive = false) 
        : root_(new TrieNode()), verbose_(verbose), 
          case_sensitive_(case_sensitive) {
        root_->depth = 0;
    }

    /**
     * Inicializa el autómata con nuevos patrones
     */
    void initialize(const std::vector<std::string>& patterns) {
        if (patterns.empty()) {
            throw std::invalid_argument("La lista de patrones no puede estar vacía");
        }

        patterns_ = patterns;
        clear_trie();
        
        auto build_start = HighResClock::now();
        build_trie();
        build_failure_links();
        
        if (verbose_) {
            auto build_end = HighResClock::now();
            auto duration = std::chrono::duration_cast<TimeDuration>(build_end - build_start);
            std::cout << "[INFO] Autómata construido en " << duration.count() << " ms\n";
            std::cout << "[INFO] Total de nodos creados: " << node_count_ << "\n";
            std::cout << "[INFO] Profundidad máxima del trie: " << max_depth_ << "\n";
        }
    }

    /**
     * Limpia el texto conservando más caracteres relevantes para literatura
     */
    std::string clean_text(const std::string& text) const {
        std::string cleaned;
        cleaned.reserve(text.size());

        for (unsigned char c : text) {
            if (std::isalpha(c)) {
                cleaned += case_sensitive_ ? c : std::tolower(c);
            } else if (c == ' ' || c == '-' || c == '\n') {
                cleaned += c;
            } else if (c == '\t') {
                cleaned += ' '; // Reemplazar tabs por espacios
            }
            // Ignorar otros caracteres no relevantes
        }

        return cleaned;
    }

    /**
     * Búsqueda mejorada con captura de contexto
     */
    std::vector<MatchResult> search(const std::string& text,
                                    size_t context_size = 20) const {
        auto start_time = HighResClock::now();
        std::vector<MatchResult> matches;
        
        const std::string cleaned_text = clean_text(text);
        std::vector<std::string> lines;
        split_lines(cleaned_text, lines);

        // Preprocesamiento para manejo de líneas y columnas
        for (size_t line_num = 0; line_num < lines.size(); ++line_num) {
            const std::string& line = lines[line_num];
            TrieNode* current_node = root_.get();

            for (size_t col = 0; col < line.size(); ++col) {
                char c = line[col];
                
                int idx = char_to_index(c);
                if (idx == -1) {
                    // Carácter no admitido, se ignora
                    continue;
                }

                // Seguir enlaces de fallo
                while (current_node != root_.get() && !current_node->children[idx]) {
                    current_node = current_node->failure_link;
                }

                if (current_node->children[idx]) {
                    current_node = current_node->children[idx].get();
                }

                // Recolectar coincidencias con contexto
                if (!current_node->pattern_indices.empty()) {
                    collect_matches(current_node, matches, line_num + 1, col + 1,
                                    line, col, context_size);
                }
            }
        }

        // Ordenar resultados por posición
        std::sort(matches.begin(), matches.end());

        if (verbose_) {
            auto end_time = HighResClock::now();
            auto duration = std::chrono::duration_cast<TimeDuration>(end_time - start_time);
            std::cout << "[INFO] Búsqueda completada en " << duration.count() 
                      << " ms. Coincidencias encontradas: " << matches.size() << "\n";
        }

        return matches;
    }

    // Getters
    const std::vector<std::string>& patterns() const { return patterns_; }
    int node_count() const { return node_count_; }
    int max_depth() const { return max_depth_; }

private:
    std::unique_ptr<TrieNode> root_;
    std::vector<std::string> patterns_;
    bool verbose_;
    bool case_sensitive_;
    int node_count_ = 0;
    int max_depth_ = 0;

    /**
     * Divide el texto en líneas conservando los saltos de línea
     */
    void split_lines(const std::string& text, std::vector<std::string>& lines) const {
        std::stringstream ss(text);
        std::string line;
        
        while (std::getline(ss, line)) {
            lines.push_back(line);
        }
    }

    /**
     * Recolecta coincidencias con información contextual
     */
    void collect_matches(TrieNode* node, std::vector<MatchResult>& matches,
                        size_t line, size_t column,
                        const std::string& line_text, size_t pos,
                        size_t context_size) const {
        
        for (TrieNode* temp = node; temp != nullptr; temp = temp->output_link) {
            for (PatternID pattern_idx : temp->pattern_indices) {
                const std::string& pattern = patterns_[pattern_idx];
                
                // Calcular contexto
                size_t start = (pos + 1 > pattern.length()) ? 
                    std::min(pos - pattern.length(), line_text.length()) : 0;
                size_t end = std::min(pos + context_size, line_text.length());
                std::string context = line_text.substr(start, end - start);
                
                // Eliminar múltiples espacios en el contexto
                context.erase(std::unique(context.begin(), context.end(), 
                    [](char a, char b) { return a == ' ' && b == ' '; }), context.end());
                
                matches.push_back({
                    line,
                    column - pattern.length() + 1,
                    pattern,
                    context,
                    pattern_idx
                });
            }
        }
    }

    void clear_trie() {
        root_.reset(new TrieNode());
        node_count_ = 1; // Solo la raíz
        max_depth_ = 0;
    }

    /**
     * Construcción del Trie optimizada
     */
    void build_trie() {
        for (PatternID i = 0; i < patterns_.size(); ++i) {
            std::string pattern = clean_text(patterns_[i]);
            if (pattern.empty()) continue;

            TrieNode* node = root_.get();

            for (char c : pattern) {
                int idx = char_to_index(c);
                if (!node->children[idx]) {
                    node->children[idx].reset(new TrieNode());
                    node->children[idx]->depth = node->depth + 1;
                    max_depth_ = std::max(max_depth_, static_cast<int>(node->children[idx]->depth));
                    node_count_++;
                }
                node = node->children[idx].get();
            }

            node->is_end_of_word = true;
            node->pattern_indices.push_back(i);
        }
    }

    /**
     * Construcción de enlaces de fallo optimizada
     */
    void build_failure_links() {
        std::queue<TrieNode*> node_queue;
        
        // Inicialización
        root_->failure_link = root_.get();
        for (int i = 0; i < ALPHABET_SIZE; ++i) {
            if (root_->children[i]) {
                root_->children[i]->failure_link = root_.get();
                node_queue.push(root_->children[i].get());
            }
        }

        // BFS para construir los enlaces
        while (!node_queue.empty()) {
            TrieNode* current = node_queue.front();
            node_queue.pop();

            for (int i = 0; i < ALPHABET_SIZE; ++i) {
                if (!current->children[i]) continue;

                TrieNode* child = current->children[i].get();
                node_queue.push(child);

                TrieNode* failure = current->failure_link;
                while (failure != root_.get() && !failure->children[i]) {
                    failure = failure->failure_link;
                }

                child->failure_link = failure->children[i] ? 
                    failure->children[i].get() : root_.get();

                // Enlace de salida optimizado
                child->output_link = child->failure_link->pattern_indices.empty() ? 
                    child->failure_link->output_link : child->failure_link;
            }
        }
    }
};

} // namespace ahocorasick

// Interfaz de usuario mejorada
namespace ui {

/**
 * Genera un resumen estadístico de los resultados
 */
void generate_summary(const std::vector<ahocorasick::MatchResult>& results, 
                     const std::vector<std::string>& patterns) {
    if (results.empty()) {
        std::cout << "No se encontraron coincidencias para generar resumen.\n";
        return;
    }

    // Estadísticas por patrón
    std::unordered_map<ahocorasick::PatternID, size_t> pattern_counts;
    for (size_t i = 0; i < results.size(); ++i) {
        pattern_counts[results[i].pattern_id]++;
    }

    std::cout << "\n=== RESUMEN ESTADÍSTICO ===\n";
    std::cout << "Total de coincidencias: " << results.size() << "\n";
    std::cout << "Coincidencias por patrón:\n";
    
    for (auto it = pattern_counts.begin(); it != pattern_counts.end(); ++it) {
        std::cout << " - " << std::setw(30) << std::left << patterns[it->first] 
                  << ": " << it->second << " coincidencias\n";
    }

    // Distribución por líneas
    if (!results.empty()) {
        size_t min_line = results.front().line;
        size_t max_line = results.back().line;
        std::cout << "\nDistribución desde línea " << min_line << " hasta " << max_line << "\n";
    }
}

/**
 * Muestra los resultados con formato mejorado
 */
void display_results(const std::vector<ahocorasick::MatchResult>& results, 
                    bool show_context = true) {
    if (results.empty()) {
        std::cout << "No se encontraron coincidencias.\n";
        return;
    }

    std::cout << "\n=== RESULTADOS DETALLADOS ===\n";
    std::cout << "Coincidencias encontradas: " << results.size() << "\n\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        std::cout << "Línea " << std::setw(4) << result.line 
                  << ", Columna " << std::setw(4) << result.column
                  << ": \"" << result.pattern << "\"";
        
        if (show_context) {
            std::cout << "\n   Contexto: \"" << result.context << "\"";
        }
        
        std::cout << "\n";
    }
}

/**
 * Exporta resultados a formato HTML accesible
 */
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

    // Estadísticas por patrón
    std::unordered_map<ahocorasick::PatternID, size_t> pattern_counts;
    for (size_t i = 0; i < results.size(); ++i) {
        pattern_counts[results[i].pattern_id]++;
    }

    out_file << "<h3>Coincidencias por patrón:</h3>\n<ul>\n";
    for (auto it = pattern_counts.begin(); it != pattern_counts.end(); ++it) {
        out_file << "<li>" << patterns[it->first] << ": " << it->second << " coincidencias</li>\n";
    }
    out_file << "</ul>\n</div>\n";

    // Resultados detallados
    out_file << "<h2>Detalles de Coincidencias</h2>\n";
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        out_file << "<div class='match'>\n"
                 << "<p><strong>Línea " << result.line << ", Columna " << result.column << ":</strong> "
                 << "Patrón: <span class='pattern'>" << result.pattern << "</span></p>\n"
                 << "<p class='context'>Contexto: \"" << result.context << "\"</p>\n"
                 << "</div>\n";
    }

    out_file << "</body>\n</html>\n";
    std::cout << "Resultados exportados a " << output_path << "\n";
}

/**
 * Carga patrones desde un archivo
 */
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

/**
 * Carga texto desde un archivo (versión simplificada sin codecvt)
 */
std::string load_text_from_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo de texto: " + file_path);
    }

    // Leer todo el contenido
    std::string content((std::istreambuf_iterator<char>(file)), 
                     std::istreambuf_iterator<char>());

    return content;
}

/**
 * Menú interactivo mejorado con más opciones
 */
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