#include "fuzzy_search.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace rtrv_search_engine {

FuzzySearch::FuzzySearch() = default;
FuzzySearch::~FuzzySearch() = default;

// ============================================================================
// N-gram Index Management
// ============================================================================

void FuzzySearch::buildNgramIndex(const std::unordered_set<std::string>& vocabulary) {
    clear();
    vocabulary_ = vocabulary;
    
    for (const auto& term : vocabulary_) {
        auto ngrams = extractNgrams(term);
        for (const auto& ngram : ngrams) {
            ngram_index_[ngram].insert(term);
        }
    }
    
    index_built_ = true;
}

void FuzzySearch::addTerm(const std::string& term) {
    if (vocabulary_.count(term)) {
        return; // Already exists
    }
    
    vocabulary_.insert(term);
    auto ngrams = extractNgrams(term);
    for (const auto& ngram : ngrams) {
        ngram_index_[ngram].insert(term);
    }
    
    // Mark as built even for incremental adds
    index_built_ = true;
}

void FuzzySearch::removeTerm(const std::string& term) {
    if (!vocabulary_.count(term)) {
        return; // Not in vocabulary
    }
    
    auto ngrams = extractNgrams(term);
    for (const auto& ngram : ngrams) {
        auto it = ngram_index_.find(ngram);
        if (it != ngram_index_.end()) {
            it->second.erase(term);
            if (it->second.empty()) {
                ngram_index_.erase(it);
            }
        }
    }
    
    vocabulary_.erase(term);
}

void FuzzySearch::clear() {
    ngram_index_.clear();
    vocabulary_.clear();
    index_built_ = false;
}

// ============================================================================
// N-gram Extraction
// ============================================================================

std::vector<std::string> FuzzySearch::extractNgrams(const std::string& term) {
    std::vector<std::string> ngrams;
    
    if (term.empty()) {
        return ngrams;
    }
    
    // Pad with start/end markers for better boundary matching
    std::string padded = "^" + term + "$";
    
    for (size_t i = 0; i + NGRAM_SIZE <= padded.size(); ++i) {
        ngrams.push_back(padded.substr(i, NGRAM_SIZE));
    }
    
    return ngrams;
}

// ============================================================================
// Fuzzy Matching
// ============================================================================

std::vector<FuzzyMatch> FuzzySearch::findMatches(
    const std::string& term,
    uint32_t max_edit_distance,
    size_t max_candidates) const {
    
    std::vector<FuzzyMatch> matches;
    
    if (term.empty()) {
        return matches;
    }
    
    // Auto-determine max edit distance if not specified (0 means auto)
    if (max_edit_distance == 0) {
        max_edit_distance = autoMaxEditDistance(term.size());
    }
    
    // If max_edit_distance is still 0 (very short term), only exact match
    if (max_edit_distance == 0) {
        if (vocabulary_.count(term)) {
            matches.push_back({term, term, 0});
        }
        return matches;
    }
    
    // Step 1: Use n-gram index to find candidate terms
    // Decompose the query term into n-grams
    auto query_ngrams = extractNgrams(term);
    
    if (query_ngrams.empty()) {
        return matches;
    }
    
    // Count how many query n-grams each vocabulary term shares
    std::unordered_map<std::string, size_t> candidate_scores;
    
    for (const auto& ngram : query_ngrams) {
        auto it = ngram_index_.find(ngram);
        if (it != ngram_index_.end()) {
            for (const auto& candidate : it->second) {
                candidate_scores[candidate]++;
            }
        }
    }
    
    // Step 2: Filter candidates by n-gram overlap threshold
    // A transposition can destroy up to (NGRAM_SIZE + 1) shared n-grams per edit.
    // Use a lenient threshold: require at least 1 shared n-gram for short terms,
    // and scale up for longer terms.
    size_t max_destroyed = max_edit_distance * (NGRAM_SIZE + 1);
    size_t min_shared_ngrams = 1;
    if (query_ngrams.size() > max_destroyed + 1) {
        min_shared_ngrams = query_ngrams.size() - max_destroyed;
    }
    
    // Step 3: Compute exact edit distance for candidates passing the n-gram filter
    for (const auto& [candidate, shared_count] : candidate_scores) {
        if (shared_count >= min_shared_ngrams) {
            uint32_t dist = damerauLevenshteinDistance(term, candidate, max_edit_distance);
            if (dist <= max_edit_distance) {
                matches.push_back({term, candidate, dist});
            }
        }
    }
    
    // Sort by edit distance (ascending), then alphabetically for ties
    std::sort(matches.begin(), matches.end(),
              [](const FuzzyMatch& a, const FuzzyMatch& b) {
                  if (a.edit_distance != b.edit_distance) {
                      return a.edit_distance < b.edit_distance;
                  }
                  return a.matched_term < b.matched_term;
              });
    
    // Limit to max_candidates
    if (matches.size() > max_candidates) {
        matches.resize(max_candidates);
    }
    
    return matches;
}

// ============================================================================
// Damerau-Levenshtein Distance
// ============================================================================

uint32_t FuzzySearch::damerauLevenshteinDistance(
    const std::string& s1,
    const std::string& s2,
    uint32_t max_distance) {
    
    const size_t len1 = s1.size();
    const size_t len2 = s2.size();
    
    // Quick length check: if length difference exceeds max_distance, no need to compute
    if (len1 > len2 + max_distance || len2 > len1 + max_distance) {
        return max_distance + 1;
    }
    
    // Handle edge cases
    if (len1 == 0) return static_cast<uint32_t>(len2);
    if (len2 == 0) return static_cast<uint32_t>(len1);
    if (s1 == s2) return 0;
    
    // Full Damerau-Levenshtein using the optimal string alignment algorithm
    // Uses a 2D DP table but bounded by max_distance for early termination
    const size_t rows = len1 + 1;
    const size_t cols = len2 + 1;
    
    // Use a flat vector for the DP table
    std::vector<uint32_t> dp(rows * cols, 0);
    
    // Helper to access dp[i][j] => dp[i * cols + j]
    auto at = [&](size_t i, size_t j) -> uint32_t& {
        return dp[i * cols + j];
    };
    
    // Initialize base cases
    for (size_t i = 0; i <= len1; ++i) {
        at(i, 0) = static_cast<uint32_t>(i);
    }
    for (size_t j = 0; j <= len2; ++j) {
        at(0, j) = static_cast<uint32_t>(j);
    }
    
    // Fill the DP table
    for (size_t i = 1; i <= len1; ++i) {
        // Track minimum value in this row for early termination
        uint32_t row_min = std::numeric_limits<uint32_t>::max();
        
        for (size_t j = 1; j <= len2; ++j) {
            uint32_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            
            // Standard Levenshtein operations
            uint32_t deletion = at(i - 1, j) + 1;
            uint32_t insertion = at(i, j - 1) + 1;
            uint32_t substitution = at(i - 1, j - 1) + cost;
            
            at(i, j) = std::min({deletion, insertion, substitution});
            
            // Damerau extension: transposition
            if (i > 1 && j > 1 &&
                s1[i - 1] == s2[j - 2] &&
                s1[i - 2] == s2[j - 1]) {
                at(i, j) = std::min(at(i, j), at(i - 2, j - 2) + cost);
            }
            
            row_min = std::min(row_min, at(i, j));
        }
        
        // Early termination: if minimum in this row exceeds max_distance,
        // the final result will certainly exceed it too
        if (row_min > max_distance) {
            return max_distance + 1;
        }
    }
    
    return at(len1, len2);
}

// ============================================================================
// Auto Edit Distance
// ============================================================================

uint32_t FuzzySearch::autoMaxEditDistance(size_t term_length) {
    if (term_length <= 2) {
        return 0;  // Exact match only for very short terms
    } else if (term_length <= 4) {
        return 1;  // Allow 1 edit for medium-short terms
    } else {
        return 2;  // Allow 2 edits for longer terms
    }
}

} // namespace rtrv_search_engine
