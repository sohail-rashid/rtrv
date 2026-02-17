#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <cstdint>

namespace search_engine {

/**
 * Configuration for snippet generation and highlighting
 */
struct SnippetOptions {
    size_t max_snippet_length = 150;      // Max characters per snippet
    size_t num_snippets = 3;              // Number of snippets to generate
    std::string highlight_open = "<em>";   // Opening highlight tag
    std::string highlight_close = "</em>"; // Closing highlight tag
};

/**
 * Generates context-aware text snippets with query term highlighting.
 *
 * Given a document's raw text and a set of query terms, the extractor:
 * 1. Finds text windows with the highest density of query terms.
 * 2. Extracts snippets that break on word boundaries.
 * 3. Wraps matched terms with configurable highlight markers.
 * 4. Prepends/appends ellipsis when the snippet is a substring of the document.
 */
class SnippetExtractor {
public:
    SnippetExtractor() = default;
    ~SnippetExtractor() = default;

    /**
     * Generate highlighted snippets from document text.
     *
     * @param text         The full document text.
     * @param query_terms  The set of query terms (lowercased).
     * @param options      Snippet configuration options.
     * @return             A vector of highlighted snippet strings.
     */
    std::vector<std::string> generateSnippets(
        const std::string& text,
        const std::vector<std::string>& query_terms,
        const SnippetOptions& options = {}) const;

    /**
     * Highlight all occurrences of query terms in a given text.
     * Case-insensitive matching, preserves original case.
     *
     * @param text         The text to highlight.
     * @param query_terms  The set of query terms (lowercased).
     * @param open_tag     Opening highlight marker.
     * @param close_tag    Closing highlight marker.
     * @return             The text with matched terms wrapped in markers.
     */
    std::string highlightTerms(
        const std::string& text,
        const std::vector<std::string>& query_terms,
        const std::string& open_tag = "<em>",
        const std::string& close_tag = "</em>") const;

private:
    /**
     * Represents a candidate window in the document text for snippet extraction.
     */
    struct Window {
        size_t start;       // Start character offset in document
        size_t end;         // End character offset in document
        size_t match_count; // Number of query term matches in this window
    };

    /**
     * Find the best snippet windows in the document text.
     */
    std::vector<Window> findBestWindows(
        const std::string& text,
        const std::vector<std::string>& query_terms,
        size_t window_size,
        size_t num_windows) const;

    /**
     * Snap window boundaries to word boundaries.
     */
    void snapToWordBoundaries(const std::string& text, size_t& start, size_t& end) const;

    /**
     * Convert a string to lowercase.
     */
    static std::string toLower(const std::string& str);

    /**
     * Check if a character is a word character (alphanumeric or apostrophe).
     */
    static bool isWordChar(char c);
};

} // namespace search_engine
