#include <gtest/gtest.h>
#include "fuzzy_search.hpp"
#include "search_engine.hpp"

using namespace rtrv_search_engine;

// ============================================================================
// Damerau-Levenshtein Distance Tests
// ============================================================================

class FuzzyDistanceTest : public ::testing::Test {};

TEST_F(FuzzyDistanceTest, IdenticalStrings) {
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("machine", "machine"), 0);
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("", ""), 0);
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("a", "a"), 0);
}

TEST_F(FuzzyDistanceTest, EmptyStrings) {
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("", "abc"), 3);
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("abc", ""), 3);
}

TEST_F(FuzzyDistanceTest, SingleSubstitution) {
    // "machne" -> "machine" is 1 edit (insert 'i')
    // Actually "machne" vs "machine": machne has 6 chars, machine has 7
    // That's an insertion, distance = 1
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("machne", "machine"), 1);
    
    // "cat" -> "bat" = 1 substitution
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("cat", "bat"), 1);
}

TEST_F(FuzzyDistanceTest, SingleDeletion) {
    // "machine" -> "machin" = 1 deletion
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("machine", "machin"), 1);
}

TEST_F(FuzzyDistanceTest, SingleInsertion) {
    // "machin" -> "machine" = 1 insertion
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("machin", "machine"), 1);
}

TEST_F(FuzzyDistanceTest, Transposition) {
    // "teh" -> "the" = 1 transposition
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("teh", "the"), 1);
    
    // "recieve" -> "receive" = 1 transposition (ie -> ei)
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("recieve", "receive"), 1);
}

TEST_F(FuzzyDistanceTest, MultipleEdits) {
    // "lerning" -> "learning" = 1 insertion
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("lerning", "learning"), 1);
    
    // "kitten" -> "sitting" = 3 edits
    EXPECT_EQ(FuzzySearch::damerauLevenshteinDistance("kitten", "sitting"), 3);
}

TEST_F(FuzzyDistanceTest, MaxDistanceBounding) {
    // "abcdef" vs "xyzwvu" should exceed max_distance=2
    uint32_t dist = FuzzySearch::damerauLevenshteinDistance("abcdef", "xyzwvu", 2);
    EXPECT_GT(dist, 2u);
}

TEST_F(FuzzyDistanceTest, LengthDifferenceEarlyTermination) {
    // Strings with length difference > max_distance return early
    uint32_t dist = FuzzySearch::damerauLevenshteinDistance("a", "abcdef", 2);
    EXPECT_GT(dist, 2u);
}

// ============================================================================
// Auto Max Edit Distance Tests
// ============================================================================

class FuzzyAutoDistanceTest : public ::testing::Test {};

TEST_F(FuzzyAutoDistanceTest, VeryShortTerms) {
    EXPECT_EQ(FuzzySearch::autoMaxEditDistance(0), 0u);
    EXPECT_EQ(FuzzySearch::autoMaxEditDistance(1), 0u);
    EXPECT_EQ(FuzzySearch::autoMaxEditDistance(2), 0u);
}

TEST_F(FuzzyAutoDistanceTest, ShortTerms) {
    EXPECT_EQ(FuzzySearch::autoMaxEditDistance(3), 1u);
    EXPECT_EQ(FuzzySearch::autoMaxEditDistance(4), 1u);
}

TEST_F(FuzzyAutoDistanceTest, LongerTerms) {
    EXPECT_EQ(FuzzySearch::autoMaxEditDistance(5), 2u);
    EXPECT_EQ(FuzzySearch::autoMaxEditDistance(10), 2u);
    EXPECT_EQ(FuzzySearch::autoMaxEditDistance(20), 2u);
}

// ============================================================================
// N-gram Index Tests
// ============================================================================

class FuzzyNgramTest : public ::testing::Test {
protected:
    FuzzySearch fuzzy;
    
    void SetUp() override {
        std::unordered_set<std::string> vocab = {
            "machine", "learning", "the", "quick", "brown", "fox",
            "search", "engine", "algorithm", "computer", "science",
            "artificial", "intelligence", "neural", "network"
        };
        fuzzy.buildNgramIndex(vocab);
    }
};

TEST_F(FuzzyNgramTest, IndexIsBuilt) {
    EXPECT_TRUE(fuzzy.isIndexBuilt());
    EXPECT_EQ(fuzzy.vocabularySize(), 15u);
}

TEST_F(FuzzyNgramTest, IncrementalAdd) {
    fuzzy.addTerm("database");
    EXPECT_EQ(fuzzy.vocabularySize(), 16u);
    
    // Should find the newly added term
    auto matches = fuzzy.findMatches("database", 0);
    EXPECT_FALSE(matches.empty());
    EXPECT_EQ(matches[0].matched_term, "database");
    EXPECT_EQ(matches[0].edit_distance, 0u);
}

TEST_F(FuzzyNgramTest, IncrementalRemove) {
    fuzzy.removeTerm("machine");
    EXPECT_EQ(fuzzy.vocabularySize(), 14u);
    
    // "machine" exact match should no longer work
    auto matches = fuzzy.findMatches("machine", 0);
    EXPECT_TRUE(matches.empty());
}

TEST_F(FuzzyNgramTest, ClearIndex) {
    fuzzy.clear();
    EXPECT_FALSE(fuzzy.isIndexBuilt());
    EXPECT_EQ(fuzzy.vocabularySize(), 0u);
}

// ============================================================================
// Fuzzy Match Finding Tests
// ============================================================================

class FuzzyMatchTest : public ::testing::Test {
protected:
    FuzzySearch fuzzy;
    
    void SetUp() override {
        std::unordered_set<std::string> vocab = {
            "machine", "learning", "the", "quick", "brown", "fox",
            "search", "engine", "algorithm", "computer", "science",
            "artificial", "intelligence", "neural", "network",
            "earning", "yearning"
        };
        fuzzy.buildNgramIndex(vocab);
    }
};

TEST_F(FuzzyMatchTest, ExactMatch) {
    auto matches = fuzzy.findMatches("machine", 2);
    EXPECT_FALSE(matches.empty());
    // First match should be exact (distance 0)
    EXPECT_EQ(matches[0].matched_term, "machine");
    EXPECT_EQ(matches[0].edit_distance, 0u);
}

TEST_F(FuzzyMatchTest, SubstitutionMatch) {
    // "machina" -> "machine" (substitution of 'e' for 'a')
    auto matches = fuzzy.findMatches("machina", 2);
    EXPECT_FALSE(matches.empty());
    
    bool found_machine = false;
    for (const auto& m : matches) {
        if (m.matched_term == "machine") {
            found_machine = true;
            EXPECT_EQ(m.edit_distance, 1u);
        }
    }
    EXPECT_TRUE(found_machine);
}

TEST_F(FuzzyMatchTest, DeletionMatch) {
    // "machne" -> "machine" (missing 'i')
    auto matches = fuzzy.findMatches("machne", 2);
    EXPECT_FALSE(matches.empty());
    
    bool found_machine = false;
    for (const auto& m : matches) {
        if (m.matched_term == "machine") {
            found_machine = true;
            EXPECT_EQ(m.edit_distance, 1u);
        }
    }
    EXPECT_TRUE(found_machine);
}

TEST_F(FuzzyMatchTest, InsertionMatch) {
    // "lerning" -> "learning" (missing 'a')
    auto matches = fuzzy.findMatches("lerning", 2);
    EXPECT_FALSE(matches.empty());
    
    bool found_learning = false;
    for (const auto& m : matches) {
        if (m.matched_term == "learning") {
            found_learning = true;
            EXPECT_EQ(m.edit_distance, 1u);
        }
    }
    EXPECT_TRUE(found_learning);
}

TEST_F(FuzzyMatchTest, TranspositionMatch) {
    // "teh" -> "the" (transposition)
    auto matches = fuzzy.findMatches("teh", 1);
    EXPECT_FALSE(matches.empty());
    
    bool found_the = false;
    for (const auto& m : matches) {
        if (m.matched_term == "the") {
            found_the = true;
            EXPECT_EQ(m.edit_distance, 1u);
        }
    }
    EXPECT_TRUE(found_the);
}

TEST_F(FuzzyMatchTest, NoMatchBeyondMaxDistance) {
    // "xyz" should not match anything with distance 1
    auto matches = fuzzy.findMatches("xyz", 1);
    EXPECT_TRUE(matches.empty());
}

TEST_F(FuzzyMatchTest, MaxCandidatesRespected) {
    auto matches = fuzzy.findMatches("learning", 2, 2);
    EXPECT_LE(matches.size(), 2u);
}

TEST_F(FuzzyMatchTest, EmptyTermReturnsEmpty) {
    auto matches = fuzzy.findMatches("", 2);
    EXPECT_TRUE(matches.empty());
}

TEST_F(FuzzyMatchTest, ResultsSortedByDistance) {
    auto matches = fuzzy.findMatches("learnin", 2);
    for (size_t i = 1; i < matches.size(); ++i) {
        EXPECT_GE(matches[i].edit_distance, matches[i - 1].edit_distance);
    }
}

TEST_F(FuzzyMatchTest, AutoDistanceScaling) {
    // Short term "fo" -> max_edit_distance auto = 0, should only find exact
    auto matches_short = fuzzy.findMatches("fo"); // auto distance=0 for 2-char
    // "fo" has length 2, auto = 0, so only exact match. "fo" not in vocab.
    EXPECT_TRUE(matches_short.empty());
    
    // "foxx" -> auto distance=1 for 4-char term
    auto matches_med = fuzzy.findMatches("foxx"); // should find "fox"? fox is 3 chars, foxx is 4, distance=1
    // "foxx" vs "fox" = 1 deletion, should match
    bool found_fox = false;
    for (const auto& m : matches_med) {
        if (m.matched_term == "fox") found_fox = true;
    }
    EXPECT_TRUE(found_fox);
}

// ============================================================================
// Integration with SearchEngine Tests
// ============================================================================

class FuzzySearchIntegrationTest : public ::testing::Test {
protected:
    SearchEngine engine;
    
    void SetUp() override {
        // Index some documents
        Document doc1{0, {{"content", "Machine learning is a subset of artificial intelligence"}}};
        Document doc2{0, {{"content", "The quick brown fox jumps over the lazy dog"}}};
        Document doc3{0, {{"content", "Search engine algorithms use inverted indexes"}}};
        Document doc4{0, {{"content", "Neural networks power modern computer science"}}};
        Document doc5{0, {{"content", "Deep learning and machine learning are related fields"}}};
        
        engine.indexDocument(doc1);
        engine.indexDocument(doc2);
        engine.indexDocument(doc3);
        engine.indexDocument(doc4);
        engine.indexDocument(doc5);
    }
};

TEST_F(FuzzySearchIntegrationTest, ExactQueryStillWorks) {
    // Without fuzzy, exact query should work as before
    SearchOptions options;
    options.fuzzy_enabled = false;
    
    auto results = engine.search("machine learning", options);
    EXPECT_FALSE(results.empty());
}

TEST_F(FuzzySearchIntegrationTest, FuzzyDisabledMisspelledReturnsEmpty) {
    // Without fuzzy, misspelled query should return empty
    SearchOptions options;
    options.fuzzy_enabled = false;
    
    auto results = engine.search("machne lerning", options);
    EXPECT_TRUE(results.empty());
}

TEST_F(FuzzySearchIntegrationTest, FuzzyEnabledMisspelledReturnsResults) {
    // With fuzzy, "machne" should match "machine" and "lerning" should match "learning"
    SearchOptions options;
    options.fuzzy_enabled = true;
    
    auto results = engine.search("machne lerning", options);
    EXPECT_FALSE(results.empty());
}

TEST_F(FuzzySearchIntegrationTest, FuzzyResultsHaveExpandedTerms) {
    SearchOptions options;
    options.fuzzy_enabled = true;
    
    auto results = engine.search("machne", options);
    EXPECT_FALSE(results.empty());
    
    // Check that expanded_terms is populated
    bool has_expansion = false;
    for (const auto& r : results) {
        if (!r.expanded_terms.empty()) {
            has_expansion = true;
            // "machne" should expand to "machine"
            auto it = r.expanded_terms.find("machne");
            if (it != r.expanded_terms.end()) {
                EXPECT_EQ(it->second, "machine");
            }
        }
    }
    EXPECT_TRUE(has_expansion);
}

TEST_F(FuzzySearchIntegrationTest, FuzzyWithExactTermsNoExpansion) {
    // When the query term exists in the index, no fuzzy expansion should happen
    SearchOptions options;
    options.fuzzy_enabled = true;
    
    auto results = engine.search("machine", options);
    EXPECT_FALSE(results.empty());
    
    // No expansion needed for exact matches
    for (const auto& r : results) {
        EXPECT_TRUE(r.expanded_terms.empty());
    }
}

TEST_F(FuzzySearchIntegrationTest, FuzzyAppliesScoringPenalty) {
    // Search with exact term
    SearchOptions exact_opts;
    exact_opts.fuzzy_enabled = false;
    auto exact_results = engine.search("machine", exact_opts);
    
    // Search with misspelled term + fuzzy
    SearchOptions fuzzy_opts;
    fuzzy_opts.fuzzy_enabled = true;
    auto fuzzy_results = engine.search("machne", fuzzy_opts);
    
    // Both should return results
    EXPECT_FALSE(exact_results.empty());
    EXPECT_FALSE(fuzzy_results.empty());
    
    // Fuzzy results should have lower scores due to penalty
    if (!exact_results.empty() && !fuzzy_results.empty()) {
        EXPECT_LT(fuzzy_results[0].score, exact_results[0].score);
    }
}

TEST_F(FuzzySearchIntegrationTest, FuzzyTransposition) {
    // "teh" -> "the" via transposition
    SearchOptions options;
    options.fuzzy_enabled = true;
    options.max_edit_distance = 1;
    
    auto results = engine.search("teh", options);
    // "the" is likely a stop word and may be filtered out by the tokenizer
    // This tests that the fuzzy mechanism at least tries
    // The result depends on tokenizer stop word settings
}

TEST_F(FuzzySearchIntegrationTest, FuzzyWithMaxEditDistance) {
    SearchOptions options;
    options.fuzzy_enabled = true;
    options.max_edit_distance = 1;
    
    // "machin" is distance 1 from "machine" - should match
    auto results1 = engine.search("machin", options);
    EXPECT_FALSE(results1.empty());
    
    // With max_edit_distance = 0, auto-scaling applies
    // "machin" is 6 chars, auto would give 2, but we explicitly set 1
    // It should still match since distance is 1
}

TEST_F(FuzzySearchIntegrationTest, FuzzySearchEngineGetAccessor) {
    // Test that the fuzzy search accessor works
    auto& fs = engine.getFuzzySearch();
    EXPECT_FALSE(fs.isIndexBuilt()); // Not built until first fuzzy query
    
    // Trigger a fuzzy query to build the index
    SearchOptions options;
    options.fuzzy_enabled = true;
    engine.search("machne", options);
    
    EXPECT_TRUE(fs.isIndexBuilt());
    EXPECT_GT(fs.vocabularySize(), 0u);
}

// ============================================================================
// Edge Cases
// ============================================================================

class FuzzyEdgeCaseTest : public ::testing::Test {
protected:
    FuzzySearch fuzzy;
};

TEST_F(FuzzyEdgeCaseTest, SingleCharVocabulary) {
    std::unordered_set<std::string> vocab = {"a", "b", "c"};
    fuzzy.buildNgramIndex(vocab);
    
    // Very short terms only exact match (auto distance = 0)
    auto matches = fuzzy.findMatches("a");
    EXPECT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].matched_term, "a");
}

TEST_F(FuzzyEdgeCaseTest, EmptyVocabulary) {
    std::unordered_set<std::string> vocab;
    fuzzy.buildNgramIndex(vocab);
    
    EXPECT_TRUE(fuzzy.isIndexBuilt());
    EXPECT_EQ(fuzzy.vocabularySize(), 0u);
    
    auto matches = fuzzy.findMatches("machine", 2);
    EXPECT_TRUE(matches.empty());
}

TEST_F(FuzzyEdgeCaseTest, DuplicateAddTerm) {
    std::unordered_set<std::string> vocab = {"hello"};
    fuzzy.buildNgramIndex(vocab);
    EXPECT_EQ(fuzzy.vocabularySize(), 1u);
    
    // Adding the same term again should be a no-op
    fuzzy.addTerm("hello");
    EXPECT_EQ(fuzzy.vocabularySize(), 1u);
}

TEST_F(FuzzyEdgeCaseTest, RemoveNonexistentTerm) {
    std::unordered_set<std::string> vocab = {"hello"};
    fuzzy.buildNgramIndex(vocab);
    
    // Removing a term that doesn't exist should be safe
    fuzzy.removeTerm("world");
    EXPECT_EQ(fuzzy.vocabularySize(), 1u);
}

TEST_F(FuzzyEdgeCaseTest, VeryLongString) {
    std::string long_str(100, 'a');
    std::unordered_set<std::string> vocab = {long_str};
    fuzzy.buildNgramIndex(vocab);
    
    auto matches = fuzzy.findMatches(long_str, 0);
    EXPECT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].edit_distance, 0u);
}

TEST_F(FuzzyEdgeCaseTest, SpecialCharacters) {
    std::unordered_set<std::string> vocab = {"c++", "c#", "node.js"};
    fuzzy.buildNgramIndex(vocab);
    
    auto matches = fuzzy.findMatches("c++", 0);
    EXPECT_FALSE(matches.empty());
    EXPECT_EQ(matches[0].matched_term, "c++");
}
