#include "snippet_extractor.hpp"
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <sstream>

namespace rtrv_search_engine {

// ==================== Public API ====================

std::vector<std::string> SnippetExtractor::generateSnippets(
        const std::string& text,
        const std::vector<std::string>& query_terms,
        const SnippetOptions& options) const {

    std::vector<std::string> snippets;

    if (text.empty() || query_terms.empty()) {
        return snippets;
    }

    // If the entire document is shorter than one snippet, just highlight the whole thing
    if (text.size() <= options.max_snippet_length) {
        snippets.push_back(
            highlightTerms(text, query_terms, options.highlight_open, options.highlight_close));
        return snippets;
    }

    // Find the best windows with highest query-term density
    auto windows = findBestWindows(text, query_terms,
                                   options.max_snippet_length, options.num_snippets);

    for (auto& win : windows) {
        // Snap to word boundaries
        snapToWordBoundaries(text, win.start, win.end);

        // Extract raw substring
        std::string raw = text.substr(win.start, win.end - win.start);

        // Highlight terms inside the snippet
        std::string highlighted = highlightTerms(raw, query_terms,
                                                 options.highlight_open, options.highlight_close);

        // Add ellipsis where applicable
        if (win.start > 0) {
            highlighted = "..." + highlighted;
        }
        if (win.end < text.size()) {
            highlighted = highlighted + "...";
        }

        snippets.push_back(highlighted);
    }

    return snippets;
}

std::string SnippetExtractor::highlightTerms(
        const std::string& text,
        const std::vector<std::string>& query_terms,
        const std::string& open_tag,
        const std::string& close_tag) const {

    if (text.empty() || query_terms.empty()) {
        return text;
    }

    // Build a set of lowercased query terms for fast lookup
    std::unordered_set<std::string> term_set;
    for (const auto& t : query_terms) {
        term_set.insert(toLower(t));
    }

    std::string result;
    result.reserve(text.size() + query_terms.size() * (open_tag.size() + close_tag.size()) * 2);

    size_t i = 0;
    while (i < text.size()) {
        // If we're at the start of a word, try to match a query term
        if (isWordChar(text[i])) {
            // Find the full word
            size_t word_start = i;
            while (i < text.size() && isWordChar(text[i])) {
                ++i;
            }
            std::string word = text.substr(word_start, i - word_start);
            std::string word_lower = toLower(word);

            if (term_set.count(word_lower)) {
                result += open_tag;
                result += word;  // Preserve original case
                result += close_tag;
            } else {
                result += word;
            }
        } else {
            result += text[i];
            ++i;
        }
    }

    return result;
}

// ==================== Private Helpers ====================

std::vector<SnippetExtractor::Window> SnippetExtractor::findBestWindows(
        const std::string& text,
        const std::vector<std::string>& query_terms,
        size_t window_size,
        size_t num_windows) const {

    // Build a set of lowercased query terms
    std::unordered_set<std::string> term_set;
    for (const auto& t : query_terms) {
        term_set.insert(toLower(t));
    }

    // Tokenize the document text to find term positions
    // We do a simple scan to find (start, end, lowered_word) tuples
    struct WordPos {
        size_t start;
        size_t end;
        std::string lower_word;
    };

    std::vector<WordPos> words;
    size_t i = 0;
    while (i < text.size()) {
        if (isWordChar(text[i])) {
            size_t ws = i;
            while (i < text.size() && isWordChar(text[i])) ++i;
            std::string w = text.substr(ws, i - ws);
            words.push_back({ws, i, toLower(w)});
        } else {
            ++i;
        }
    }

    if (words.empty()) {
        return {};
    }

    // Score every possible window of the given character width
    // Use a sliding-window approach over word positions
    struct ScoredWindow {
        size_t start;
        size_t end;
        size_t score;
    };

    std::vector<ScoredWindow> scored_windows;

    // Try window starting at each word position
    for (size_t wi = 0; wi < words.size(); ++wi) {
        size_t w_start = words[wi].start;
        size_t w_end = std::min(w_start + window_size, text.size());

        // Count query term matches inside [w_start, w_end)
        size_t score = 0;
        for (size_t wj = wi; wj < words.size() && words[wj].start < w_end; ++wj) {
            if (term_set.count(words[wj].lower_word)) {
                ++score;
            }
        }

        if (score > 0) {
            scored_windows.push_back({w_start, w_end, score});
        }
    }

    if (scored_windows.empty()) {
        // No matches at all — return a single snippet from the beginning
        Window fallback;
        fallback.start = 0;
        fallback.end = std::min(window_size, text.size());
        fallback.match_count = 0;
        return {fallback};
    }

    // Sort by score descending, then by position ascending (prefer earlier text)
    std::sort(scored_windows.begin(), scored_windows.end(),
        [](const ScoredWindow& a, const ScoredWindow& b) {
            if (a.score != b.score) return a.score > b.score;
            return a.start < b.start;
        });

    // Greedily pick non-overlapping windows
    std::vector<Window> result;
    for (const auto& sw : scored_windows) {
        if (result.size() >= num_windows) break;

        // Check overlap with already-selected windows
        bool overlaps = false;
        for (const auto& existing : result) {
            if (sw.start < existing.end && sw.end > existing.start) {
                overlaps = true;
                break;
            }
        }

        if (!overlaps) {
            result.push_back({sw.start, sw.end, sw.score});
        }
    }

    // Sort final windows by position for natural reading order
    std::sort(result.begin(), result.end(),
        [](const Window& a, const Window& b) { return a.start < b.start; });

    return result;
}

void SnippetExtractor::snapToWordBoundaries(const std::string& text, size_t& start, size_t& end) const {
    // Snap start forward to the beginning of the next word if we're mid-word
    if (start > 0 && start < text.size() && isWordChar(text[start]) && isWordChar(text[start - 1])) {
        // We're in the middle of a word — move start forward past this word
        while (start < text.size() && isWordChar(text[start])) {
            ++start;
        }
        // Skip any whitespace after the broken word
        while (start < text.size() && !isWordChar(text[start])) {
            ++start;
        }
    }

    // Snap end forward to include the full word if we're mid-word
    if (end < text.size() && end > 0 && isWordChar(text[end - 1]) && isWordChar(text[end])) {
        // We're cutting mid-word — include the rest of this word
        while (end < text.size() && isWordChar(text[end])) {
            ++end;
        }
    }

    // Guard: ensure start < end
    if (start >= end) {
        // Fallback: use original boundaries
        end = std::min(start + 1, text.size());
    }
}

std::string SnippetExtractor::toLower(const std::string& str) {
    std::string result = str;
    for (auto& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

bool SnippetExtractor::isWordChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '\'';
}

} // namespace rtrv_search_engine
