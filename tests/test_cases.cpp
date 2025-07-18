#include "catch.hpp"
#include "../PatternMatcher.h"
#include "../ui.h"
#include <fstream>
#include <cstdio>

TEST_CASE(trie_construction) {
    ahocorasick::PatternMatcher matcher;
    matcher.initialize({"he", "she", "hers"});
    REQUIRE(matcher.patterns().size() == 3);
    REQUIRE(matcher.node_count() > 3);
    REQUIRE(matcher.max_depth() >= 3);
}

TEST_CASE(pattern_search) {
    ahocorasick::PatternMatcher matcher;
    matcher.initialize({"he", "she", "hers"});
    auto results = matcher.search("ushers");
    REQUIRE(results.size() == 3);
    REQUIRE(results[0].pattern == "she");
}

TEST_CASE(file_loading) {
    const char* fname = "tmp_patterns.txt";
    {
        std::ofstream out(fname);
        out << "alpha\nbeta\n";
    }
    auto patterns = ui::load_patterns_from_file(fname);
    std::remove(fname);
    REQUIRE(patterns.size() == 2);
    REQUIRE(patterns[0] == "alpha");
}
