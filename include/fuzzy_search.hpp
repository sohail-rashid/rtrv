#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

namespace rtrv_search_engine {

/**
 * Result of a fuzzy term match
 */
struct FuzzyMatch {
    std::string original_term;    // The misspelled query term
    std::string matched_term;     // The matched vocabulary term
    uint32_t edit_distance;       // Edit distance between original and matched
    
    bool operator<(const FuzzyMatch& other) const {
        return edit_distance < other.edit_distance;
    }
};

/**
 * Fuzzy Search Engine with N-gram index for efficient candidate generation
 * and Damerau-Levenshtein distance for edit distance computation.
 */
class FuzzySearch {
public:
    FuzzySearch();
    ~FuzzySearch();

    /**
     * Build the n-gram index from a vocabulary (set of terms).
     * Should be called after indexing or when the vocabulary changes.
     * 
     * @param vocabulary Set of all unique terms in the inverted index
     */
    void buildNgramIndex(const std::unordered_set<std::string>& vocabulary);

    /**
     * Add a single term to the n-gram index (incremental update).
     * 
     * @param term The term to add
     */
    void addTerm(const std::string& term);

    /**
     * Remove a term from the n-gram index.
     * 
     * @param term The term to remove
     */
    void removeTerm(const std::string& term);

    /**
     * Find fuzzy matches for a misspelled term.
     * Uses n-gram index for candidate generation, then filters by edit distance.
     * 
     * @param term The (potentially misspelled) query term
     * @param max_edit_distance Maximum allowed edit distance (0 = auto based on term length)
     * @param max_candidates Maximum number of candidates to return
     * @return Vector of fuzzy matches sorted by edit distance
     */
    std::vector<FuzzyMatch> findMatches(
        const std::string& term,
        uint32_t max_edit_distance = 0,
        size_t max_candidates = 10) const;

    /**
     * Calculate Damerau-Levenshtein distance between two strings.
     * Supports transposition, insertion, deletion, and substitution.
     * Uses bounded DP with early termination when distance exceeds max_distance.
     * 
     * @param s1 First string
     * @param s2 Second string
     * @param max_distance Maximum distance threshold (for early termination)
     * @return Edit distance, or max_distance+1 if distance exceeds threshold
     */
    static uint32_t damerauLevenshteinDistance(
        const std::string& s1,
        const std::string& s2,
        uint32_t max_distance = 255);

    /**
     * Determine the appropriate max edit distance based on term length.
     * - Terms <= 2 chars: 0 (exact match only)
     * - Terms 3-4 chars: 1
     * - Terms >= 5 chars: 2
     * 
     * @param term_length Length of the term
     * @return Recommended max edit distance
     */
    static uint32_t autoMaxEditDistance(size_t term_length);

    /**
     * Check if the n-gram index has been built.
     */
    bool isIndexBuilt() const { return index_built_; }

    /**
     * Get the number of terms in the n-gram index.
     */
    size_t vocabularySize() const { return vocabulary_.size(); }

    /**
     * Clear the n-gram index.
     */
    void clear();

private:
    /**
     * Extract character n-grams (bigrams) from a term.
     * Pads with ^ (start) and $ (end) markers.
     * 
     * @param term The term to decompose
     * @return Vector of n-gram strings
     */
    static std::vector<std::string> extractNgrams(const std::string& term);

    // N-gram index: maps each n-gram to the set of vocabulary terms containing it
    std::unordered_map<std::string, std::unordered_set<std::string>> ngram_index_;

    // Full vocabulary for verification
    std::unordered_set<std::string> vocabulary_;

    // Whether the index has been built
    bool index_built_ = false;

    // N-gram size (default: 2 for bigrams)
    static constexpr size_t NGRAM_SIZE = 2;
};

} // namespace rtrv_search_engine
