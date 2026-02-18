#pragma once

#include "document.hpp"
#include "snippet_extractor.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace rtrv_search_engine {

/**
 * Search options
 */
struct SearchOptions {
    std::string ranker_name = "";  // Empty = use default ranker
    size_t max_results = 10;
    bool explain_scores = false;
    bool use_top_k_heap = true;  // Use bounded priority queue for Top-K retrieval

    // Snippet / highlighting options
    bool generate_snippets = false;  // Enable snippet generation
    SnippetOptions snippet_options;  // Snippet configuration

    // Fuzzy search options
    bool fuzzy_enabled = false;     // Enable fuzzy matching for typo tolerance
    uint32_t max_edit_distance = 0; // 0 = auto (based on term length)

    // Cache control
    bool use_cache = true;  // Enable query result caching

    // Pagination: offset-based
    size_t offset = 0;  // Skip first N results (default: 0)

    // Pagination: cursor-based (search-after)
    std::optional<double> search_after_score;       // Score of last result on previous page
    std::optional<uint64_t> search_after_id;        // Doc ID of last result on previous page

    // Deprecated: Use ranker_name instead
    enum RankingAlgorithm { TF_IDF, BM25 };
    RankingAlgorithm algorithm = BM25;  // For backward compatibility
};

/**
 * Search result
 */
struct SearchResult {
    Document document;
    double score;
    std::string explanation;                 // Optional score breakdown
    std::vector<std::string> snippets;       // Highlighted snippets (populated when generate_snippets=true)
    std::unordered_map<std::string, std::string> expanded_terms;  // Fuzzy: original -> corrected term

    // Comparison operators for sorting and heap operations
    bool operator>(const SearchResult& other) const {
        return score > other.score;  // Higher scores are "greater"
    }

    bool operator<(const SearchResult& other) const {
        return score < other.score;
    }
};

/**
 * Index statistics
 */
struct IndexStatistics {
    size_t total_documents;
    size_t total_terms;
    double avg_doc_length;
};

/**
 * Cache statistics
 */
struct CacheStatistics {
    size_t hit_count = 0;
    size_t miss_count = 0;
    size_t eviction_count = 0;
    size_t current_size = 0;
    size_t max_size = 0;
    double hit_rate = 0.0;
};

/**
 * Pagination metadata returned alongside search results
 */
struct PaginationInfo {
    size_t total_hits = 0;      // Total number of matching documents
    size_t offset = 0;          // Offset used for this page
    size_t page_size = 0;       // Number of results in this page
    bool has_next_page = false;  // Whether more results are available
};

/**
 * Paginated search results — wraps results with pagination metadata
 */
struct PaginatedSearchResults {
    std::vector<SearchResult> results;
    PaginationInfo pagination;
};

} // namespace rtrv_search_engine
