#ifndef PATTERN_MATCHER_H
#define PATTERN_MATCHER_H

#include <array>
#include <chrono>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace ahocorasick {

static constexpr int ALPHABET_SIZE = 28; // 26 letras + espacio + guión
using TimeDuration = std::chrono::milliseconds;
using HighResClock = std::chrono::high_resolution_clock;
using PatternID = size_t;

int char_to_index(char c);

struct MatchResult {
    size_t line;
    size_t column;
    std::string pattern;
    std::string context;
    PatternID pattern_id;

    bool operator<(const MatchResult& other) const;
};

class PatternMatcher {
public:
    explicit PatternMatcher(bool verbose = false, bool case_sensitive = false);

    void initialize(const std::vector<std::string>& patterns);
    std::string clean_text(const std::string& text) const;
    std::vector<MatchResult> search(const std::string& text,
                                    size_t context_size = 20) const;

    const std::vector<std::string>& patterns() const;
    int node_count() const;
    int max_depth() const;

private:
    struct TrieNode;
    std::unique_ptr<TrieNode> root_;
    std::vector<std::string> patterns_;
    bool verbose_;
    bool case_sensitive_;
    int node_count_ = 0;
    int max_depth_ = 0;

    void split_lines(const std::string& text, std::vector<std::string>& lines) const;
    void collect_matches(TrieNode* node, std::vector<MatchResult>& matches,
                         size_t line, size_t column,
                         const std::string& line_text, size_t pos,
                         size_t context_size) const;
    void clear_trie();
    void build_trie();
    void build_failure_links();

    struct TrieNode {
        std::array<std::unique_ptr<TrieNode>, ALPHABET_SIZE> children;
        bool is_end_of_word = false;
        TrieNode* failure_link = nullptr;
        TrieNode* output_link = nullptr;
        std::vector<PatternID> pattern_indices;
        int depth = 0;

        TrieNode() = default;
        TrieNode(const TrieNode&) = delete;
        TrieNode& operator=(const TrieNode&) = delete;
    };
};

} // namespace ahocorasick

#endif // PATTERN_MATCHER_H
