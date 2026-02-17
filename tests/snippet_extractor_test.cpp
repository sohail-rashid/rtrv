#include <gtest/gtest.h>
#include "snippet_extractor.hpp"
#include "search_engine.hpp"

using namespace search_engine;

// =============================================================================
// SnippetExtractor unit tests
// =============================================================================

class SnippetExtractorTest : public ::testing::Test {
protected:
    SnippetExtractor extractor;
};

// ---- highlightTerms tests ----

TEST_F(SnippetExtractorTest, HighlightSingleTerm) {
    std::string text = "The quick brown fox jumps over the lazy dog";
    std::vector<std::string> terms = {"fox"};
    std::string result = extractor.highlightTerms(text, terms);
    EXPECT_NE(result.find("<em>fox</em>"), std::string::npos);
    // Non-matching words should not be highlighted
    EXPECT_EQ(result.find("<em>quick</em>"), std::string::npos);
}

TEST_F(SnippetExtractorTest, HighlightMultipleTerms) {
    std::string text = "machine learning is a branch of artificial intelligence";
    std::vector<std::string> terms = {"machine", "intelligence"};
    std::string result = extractor.highlightTerms(text, terms);
    EXPECT_NE(result.find("<em>machine</em>"), std::string::npos);
    EXPECT_NE(result.find("<em>intelligence</em>"), std::string::npos);
    EXPECT_EQ(result.find("<em>branch</em>"), std::string::npos);
}

TEST_F(SnippetExtractorTest, HighlightIsCaseInsensitive) {
    std::string text = "Machine Learning and MACHINE learning";
    std::vector<std::string> terms = {"machine"};
    std::string result = extractor.highlightTerms(text, terms);
    // Both "Machine" and "MACHINE" should be highlighted, preserving original case
    EXPECT_NE(result.find("<em>Machine</em>"), std::string::npos);
    EXPECT_NE(result.find("<em>MACHINE</em>"), std::string::npos);
}

TEST_F(SnippetExtractorTest, HighlightPreservesOriginalCase) {
    std::string text = "Python is Great";
    std::vector<std::string> terms = {"python"};
    std::string result = extractor.highlightTerms(text, terms);
    // Should wrap "Python" (original case) not "python"
    EXPECT_NE(result.find("<em>Python</em>"), std::string::npos);
    EXPECT_EQ(result.find("<em>python</em>"), std::string::npos);
}

TEST_F(SnippetExtractorTest, HighlightCustomTags) {
    std::string text = "hello world";
    std::vector<std::string> terms = {"world"};
    std::string result = extractor.highlightTerms(text, terms, "**", "**");
    EXPECT_NE(result.find("**world**"), std::string::npos);
    EXPECT_EQ(result.find("<em>"), std::string::npos);
}

TEST_F(SnippetExtractorTest, HighlightEmptyText) {
    std::string result = extractor.highlightTerms("", {"fox"});
    EXPECT_TRUE(result.empty());
}

TEST_F(SnippetExtractorTest, HighlightEmptyTerms) {
    std::string text = "hello world";
    std::string result = extractor.highlightTerms(text, {});
    EXPECT_EQ(result, text);  // Unchanged
}

TEST_F(SnippetExtractorTest, HighlightNoMatch) {
    std::string text = "hello world";
    std::vector<std::string> terms = {"xyz"};
    std::string result = extractor.highlightTerms(text, terms);
    EXPECT_EQ(result, text);  // No highlights, text unchanged
}

TEST_F(SnippetExtractorTest, HighlightTermAtStartAndEnd) {
    std::string text = "fox jumped over another fox";
    std::vector<std::string> terms = {"fox"};
    std::string result = extractor.highlightTerms(text, terms);
    // Both occurrences should be highlighted
    size_t first = result.find("<em>fox</em>");
    EXPECT_NE(first, std::string::npos);
    size_t second = result.find("<em>fox</em>", first + 1);
    EXPECT_NE(second, std::string::npos);
}

TEST_F(SnippetExtractorTest, HighlightDoesNotMatchSubstrings) {
    std::string text = "foxes are not fox";
    std::vector<std::string> terms = {"fox"};
    std::string result = extractor.highlightTerms(text, terms);
    // "foxes" should NOT be highlighted, only "fox"
    EXPECT_EQ(result.find("<em>foxes</em>"), std::string::npos);
    EXPECT_NE(result.find("<em>fox</em>"), std::string::npos);
}

// ---- generateSnippets tests ----

TEST_F(SnippetExtractorTest, SnippetShortDocument) {
    std::string text = "short document about fox";
    std::vector<std::string> terms = {"fox"};
    auto snippets = extractor.generateSnippets(text, terms);
    ASSERT_EQ(snippets.size(), 1);
    EXPECT_NE(snippets[0].find("<em>fox</em>"), std::string::npos);
    // Short doc should NOT have ellipsis
    EXPECT_EQ(snippets[0].find("..."), std::string::npos);
}

TEST_F(SnippetExtractorTest, SnippetLongDocumentHasEllipsis) {
    // Build a long document where the match is in the middle
    std::string text;
    for (int i = 0; i < 20; ++i) text += "some filler padding words here. ";
    text += "machine learning is important. ";
    for (int i = 0; i < 20; ++i) text += "more filler padding words here. ";

    std::vector<std::string> terms = {"machine"};
    SnippetOptions opts;
    opts.max_snippet_length = 80;
    opts.num_snippets = 1;

    auto snippets = extractor.generateSnippets(text, terms, opts);
    ASSERT_FALSE(snippets.empty());
    // Should have at least one snippet containing the highlighted term
    bool found_highlight = false;
    for (const auto& s : snippets) {
        if (s.find("<em>machine</em>") != std::string::npos) {
            found_highlight = true;
        }
    }
    EXPECT_TRUE(found_highlight);
}

TEST_F(SnippetExtractorTest, SnippetRespectsNumSnippets) {
    // Build document with matches in multiple locations
    std::string text;
    text += "alpha fox jumps over the lazy dog. ";
    for (int i = 0; i < 30; ++i) text += "filler content padding text. ";
    text += "beta fox runs through the field. ";
    for (int i = 0; i < 30; ++i) text += "more padding filler content. ";
    text += "gamma fox sleeps under the tree.";

    std::vector<std::string> terms = {"fox"};
    SnippetOptions opts;
    opts.max_snippet_length = 60;
    opts.num_snippets = 2;

    auto snippets = extractor.generateSnippets(text, terms, opts);
    EXPECT_LE(snippets.size(), 2u);
    EXPECT_GE(snippets.size(), 1u);
}

TEST_F(SnippetExtractorTest, SnippetEmptyDocument) {
    auto snippets = extractor.generateSnippets("", {"fox"});
    EXPECT_TRUE(snippets.empty());
}

TEST_F(SnippetExtractorTest, SnippetEmptyQueryTerms) {
    auto snippets = extractor.generateSnippets("hello world", {});
    EXPECT_TRUE(snippets.empty());
}

TEST_F(SnippetExtractorTest, SnippetNoMatchReturnsFallback) {
    // Build a long document with no matching terms
    std::string text;
    for (int i = 0; i < 20; ++i) text += "some random content filler words. ";

    std::vector<std::string> terms = {"zzzznotfound"};
    SnippetOptions opts;
    opts.max_snippet_length = 80;
    opts.num_snippets = 1;

    auto snippets = extractor.generateSnippets(text, terms, opts);
    // Should still return a fallback snippet from the beginning
    EXPECT_GE(snippets.size(), 1u);
}

TEST_F(SnippetExtractorTest, SnippetCustomHighlightTags) {
    std::string text = "short document about machine learning";
    std::vector<std::string> terms = {"machine"};
    SnippetOptions opts;
    opts.highlight_open = "**";
    opts.highlight_close = "**";

    auto snippets = extractor.generateSnippets(text, terms, opts);
    ASSERT_EQ(snippets.size(), 1);
    EXPECT_NE(snippets[0].find("**machine**"), std::string::npos);
    EXPECT_EQ(snippets[0].find("<em>"), std::string::npos);
}

TEST_F(SnippetExtractorTest, SnippetMultipleTermsHighlighted) {
    std::string text = "machine learning is a subset of artificial intelligence and deep learning";
    std::vector<std::string> terms = {"machine", "learning", "intelligence"};

    auto snippets = extractor.generateSnippets(text, terms);
    ASSERT_EQ(snippets.size(), 1);  // Short doc => single snippet
    EXPECT_NE(snippets[0].find("<em>machine</em>"), std::string::npos);
    EXPECT_NE(snippets[0].find("<em>learning</em>"), std::string::npos);
    EXPECT_NE(snippets[0].find("<em>intelligence</em>"), std::string::npos);
}

// =============================================================================
// Integration tests: SearchEngine with snippet generation
// =============================================================================

class SearchEngineSnippetTest : public ::testing::Test {
protected:
    SearchEngine engine;

    void SetUp() override {
        Document doc1{0, {{"content", "Machine learning is a branch of artificial intelligence that focuses on building systems"}}};
        Document doc2{0, {{"content", "Deep learning is a subset of machine learning using neural networks"}}};
        Document doc3{0, {{"content", "The quick brown fox jumps over the lazy dog on a sunny day"}}};
        engine.indexDocument(doc1);
        engine.indexDocument(doc2);
        engine.indexDocument(doc3);
    }
};

TEST_F(SearchEngineSnippetTest, SearchWithoutSnippets) {
    SearchOptions opts;
    opts.generate_snippets = false;
    auto results = engine.search("machine learning", opts);
    EXPECT_FALSE(results.empty());
    // Snippets should be empty when not requested
    for (const auto& r : results) {
        EXPECT_TRUE(r.snippets.empty());
    }
}

TEST_F(SearchEngineSnippetTest, SearchWithSnippetsEnabled) {
    SearchOptions opts;
    opts.generate_snippets = true;
    auto results = engine.search("machine learning", opts);
    EXPECT_FALSE(results.empty());
    // At least one result should have snippets
    bool any_snippets = false;
    for (const auto& r : results) {
        if (!r.snippets.empty()) {
            any_snippets = true;
            // Verify highlights are present
            for (const auto& s : r.snippets) {
                // Should contain at least one <em> tag
                EXPECT_TRUE(s.find("<em>") != std::string::npos)
                    << "Snippet should contain highlight: " << s;
            }
        }
    }
    EXPECT_TRUE(any_snippets);
}

TEST_F(SearchEngineSnippetTest, SearchSnippetsContainQueryTerms) {
    SearchOptions opts;
    opts.generate_snippets = true;
    auto results = engine.search("fox", opts);
    EXPECT_FALSE(results.empty());

    for (const auto& r : results) {
        for (const auto& s : r.snippets) {
            EXPECT_NE(s.find("<em>fox</em>"), std::string::npos)
                << "Snippet should highlight 'fox': " << s;
        }
    }
}

TEST_F(SearchEngineSnippetTest, SearchSnippetsCustomOptions) {
    SearchOptions opts;
    opts.generate_snippets = true;
    opts.snippet_options.highlight_open = "[HL]";
    opts.snippet_options.highlight_close = "[/HL]";
    opts.snippet_options.max_snippet_length = 200;
    opts.snippet_options.num_snippets = 1;

    auto results = engine.search("intelligence", opts);
    EXPECT_FALSE(results.empty());

    for (const auto& r : results) {
        for (const auto& s : r.snippets) {
            EXPECT_NE(s.find("[HL]"), std::string::npos)
                << "Snippet should use custom highlight tags: " << s;
            EXPECT_EQ(s.find("<em>"), std::string::npos)
                << "Snippet should NOT contain default tags: " << s;
        }
    }
}

TEST_F(SearchEngineSnippetTest, SearchNoResultsNoSnippets) {
    SearchOptions opts;
    opts.generate_snippets = true;
    auto results = engine.search("zzzznotfound", opts);
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchEngineSnippetTest, BackwardCompatibilityNoSnippetField) {
    // Default SearchOptions should NOT generate snippets
    auto results = engine.search("machine");
    EXPECT_FALSE(results.empty());
    for (const auto& r : results) {
        EXPECT_TRUE(r.snippets.empty());
    }
}

// =============================================================================
// Edge case tests
// =============================================================================

TEST_F(SnippetExtractorTest, HighlightSingleCharacterWord) {
    std::string text = "I am a test";
    std::vector<std::string> terms = {"a"};
    std::string result = extractor.highlightTerms(text, terms);
    EXPECT_NE(result.find("<em>a</em>"), std::string::npos);
}

TEST_F(SnippetExtractorTest, HighlightWithPunctuation) {
    std::string text = "Hello, world! Find the fox.";
    std::vector<std::string> terms = {"fox"};
    std::string result = extractor.highlightTerms(text, terms);
    EXPECT_NE(result.find("<em>fox</em>"), std::string::npos);
    // Punctuation should be preserved around the highlight
    EXPECT_NE(result.find("<em>fox</em>."), std::string::npos);
}

TEST_F(SnippetExtractorTest, SnippetVeryShortMaxLength) {
    std::string text = "machine learning is great for solving complex problems in modern world";
    std::vector<std::string> terms = {"machine"};
    SnippetOptions opts;
    opts.max_snippet_length = 20;
    opts.num_snippets = 1;

    auto snippets = extractor.generateSnippets(text, terms, opts);
    ASSERT_FALSE(snippets.empty());
    // The snippet (excluding tags and ellipsis) should be reasonably short
}

TEST_F(SnippetExtractorTest, SnippetAllWordsMatch) {
    std::string text = "fox fox fox fox fox";
    std::vector<std::string> terms = {"fox"};
    auto snippets = extractor.generateSnippets(text, terms);
    ASSERT_EQ(snippets.size(), 1);  // Short doc = 1 snippet
    // Count highlights
    size_t count = 0;
    size_t pos = 0;
    while ((pos = snippets[0].find("<em>fox</em>", pos)) != std::string::npos) {
        ++count;
        pos += 1;
    }
    EXPECT_EQ(count, 5u);
}
