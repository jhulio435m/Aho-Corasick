#include "PatternMatcher.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

namespace ahocorasick {

int char_to_index(char c) {
    if (c == ' ') return 26;
    if (c == '-') return 27;
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= 'A' && c <= 'Z') return std::tolower(c) - 'a';
    return -1;
}

bool MatchResult::operator<(const MatchResult& other) const {
    return std::tie(line, column, pattern_id) <
           std::tie(other.line, other.column, other.pattern_id);
}

PatternMatcher::PatternMatcher(bool verbose, bool case_sensitive)
    : root_(new TrieNode()), verbose_(verbose), case_sensitive_(case_sensitive) {
    root_->depth = 0;
}

void PatternMatcher::initialize(const std::vector<std::string>& patterns) {
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

std::string PatternMatcher::clean_text(const std::string& text) const {
    std::string cleaned;
    cleaned.reserve(text.size());

    for (unsigned char c : text) {
        if (std::isalpha(c)) {
            cleaned += case_sensitive_ ? c : std::tolower(c);
        } else if (c == ' ' || c == '-' || c == '\n') {
            cleaned += c;
        } else if (c == '\t') {
            cleaned += ' ';
        }
    }
    return cleaned;
}

std::vector<MatchResult> PatternMatcher::search(const std::string& text,
                                                size_t context_size) const {
    auto start_time = HighResClock::now();
    std::vector<MatchResult> matches;

    const std::string cleaned_text = clean_text(text);
    std::vector<std::string> lines;
    split_lines(cleaned_text, lines);

    for (size_t line_num = 0; line_num < lines.size(); ++line_num) {
        const std::string& line = lines[line_num];
        TrieNode* current_node = root_.get();

        for (size_t col = 0; col < line.size(); ++col) {
            char c = line[col];
            int idx = char_to_index(c);
            if (idx == -1) continue;

            while (current_node != root_.get() && !current_node->children[idx]) {
                current_node = current_node->failure_link;
            }
            if (current_node->children[idx]) {
                current_node = current_node->children[idx].get();
            }

            if (!current_node->pattern_indices.empty()) {
                collect_matches(current_node, matches, line_num + 1, col + 1,
                                line, col, context_size);
            }
        }
    }

    std::sort(matches.begin(), matches.end());

    if (verbose_) {
        auto end_time = HighResClock::now();
        auto duration = std::chrono::duration_cast<TimeDuration>(end_time - start_time);
        std::cout << "[INFO] Búsqueda completada en " << duration.count()
                  << " ms. Coincidencias encontradas: " << matches.size() << "\n";
    }
    return matches;
}

const std::vector<std::string>& PatternMatcher::patterns() const { return patterns_; }
int PatternMatcher::node_count() const { return node_count_; }
int PatternMatcher::max_depth() const { return max_depth_; }

void PatternMatcher::split_lines(const std::string& text, std::vector<std::string>& lines) const {
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
}

void PatternMatcher::collect_matches(TrieNode* node, std::vector<MatchResult>& matches,
                                     size_t line, size_t column,
                                     const std::string& line_text, size_t pos,
                                     size_t context_size) const {
    for (TrieNode* temp = node; temp != nullptr; temp = temp->output_link) {
        for (PatternID pattern_idx : temp->pattern_indices) {
            const std::string& pattern = patterns_[pattern_idx];
            size_t start = (pos + 1 > pattern.length()) ?
                            std::min(pos - pattern.length(), line_text.length()) : 0;
            size_t end = std::min(pos + context_size, line_text.length());
            std::string context = line_text.substr(start, end - start);
            context.erase(std::unique(context.begin(), context.end(),
                                     [](char a, char b){return a==' ' && b==' ';}),
                          context.end());
            matches.push_back({line,
                               column - pattern.length() + 1,
                               pattern,
                               context,
                               pattern_idx});
        }
    }
}

void PatternMatcher::clear_trie() {
    root_.reset(new TrieNode());
    node_count_ = 1;
    max_depth_ = 0;
}

void PatternMatcher::build_trie() {
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

void PatternMatcher::build_failure_links() {
    std::queue<TrieNode*> node_queue;
    root_->failure_link = root_.get();
    for (int i = 0; i < ALPHABET_SIZE; ++i) {
        if (root_->children[i]) {
            root_->children[i]->failure_link = root_.get();
            node_queue.push(root_->children[i].get());
        }
    }
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
            child->failure_link = failure->children[i] ? failure->children[i].get() : root_.get();
            child->output_link = child->failure_link->pattern_indices.empty() ?
                                  child->failure_link->output_link : child->failure_link;
        }
    }
}

} // namespace ahocorasick
