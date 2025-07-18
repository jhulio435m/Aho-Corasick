#ifndef UI_H
#define UI_H

#include "PatternMatcher.h"
#include <string>
#include <vector>

namespace ui {

void generate_summary(const std::vector<ahocorasick::MatchResult>& results,
                      const std::vector<std::string>& patterns);
void display_results(const std::vector<ahocorasick::MatchResult>& results,
                     bool show_context = true);
void export_to_html(const std::vector<ahocorasick::MatchResult>& results,
                    const std::vector<std::string>& patterns,
                    const std::string& output_path);
std::vector<std::string> load_patterns_from_file(const std::string& file_path);
std::string load_text_from_file(const std::string& file_path);
void interactive_menu();

} // namespace ui

#endif // UI_H
