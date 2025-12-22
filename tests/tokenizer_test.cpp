#include <gtest/gtest.h>
#include "tokenizer.hpp"
#include <fstream>
#include <sstream>

using namespace search_engine;

// Helper function to load stopwords from file
#include <unordered_set>
#include <string>
#include <algorithm>
#include <cctype>
#include <stdexcept>

std::unordered_set<std::string> loadStopWords(const std::string& path) {
    std::unordered_set<std::string> stopwords;
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open stopwords file: " + path);
    }

    std::string word;
    while (std::getline(file, word)) {

        // 1. Trim leading whitespace
        word.erase(0, word.find_first_not_of(" \t\r\n"));

        // 2. Trim trailing whitespace
        word.erase(word.find_last_not_of(" \t\r\n") + 1);

        // 3. Skip empty lines or comments
        if (word.empty() || word[0] == '#') {
            continue;
        }

        // 4. Normalize to lowercase
        std::transform(word.begin(), word.end(), word.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

        stopwords.insert(word);
    }

    return stopwords;
}

class TokenizerTest : public ::testing::Test {
protected:
    Tokenizer tokenizer;
};

TEST_F(TokenizerTest, BasicTokenization) {
    auto tokens = tokenizer.tokenize("Hello, World!");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
}

TEST_F(TokenizerTest, LowercaseNormalization) {
    auto tokens = tokenizer.tokenize("HELLO World");
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    
    // Test with lowercase disabled
    tokenizer.setLowercase(false);
    tokens = tokenizer.tokenize("HELLO World");
    EXPECT_EQ(tokens[0], "HELLO");
    EXPECT_EQ(tokens[1], "World");
}

TEST_F(TokenizerTest, StopWordRemoval) {
    // Load stopwords from file
    auto stopwords = loadStopWords("../data/stopwords.txt");
    EXPECT_FALSE(stopwords.empty());
    tokenizer.setStopWords(stopwords);
    
    auto tokens = tokenizer.tokenize("the quick brown fox");
    // "the" should be filtered as a stop word
    EXPECT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "quick");
    EXPECT_EQ(tokens[1], "brown");
    EXPECT_EQ(tokens[2], "fox");
    
    // Test with custom stop words
    std::unordered_set<std::string> custom_stops = {"quick"};
    tokenizer.setStopWords(custom_stops);
    tokens = tokenizer.tokenize("the quick brown fox");
    EXPECT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "the");
    EXPECT_EQ(tokens[1], "brown");
    EXPECT_EQ(tokens[2], "fox");
}

TEST_F(TokenizerTest, EmptyString) {
    auto tokens = tokenizer.tokenize("");
    EXPECT_TRUE(tokens.empty());
    
    // Test with only whitespace
    tokens = tokenizer.tokenize("   \t\n  ");
    EXPECT_TRUE(tokens.empty());
}

TEST_F(TokenizerTest, PunctuationHandling) {
    
    // Test punctuation retained within words
    auto tokens = tokenizer.tokenize("don't can't won't");
    EXPECT_EQ(tokens.size(), 3);
    EXPECT_FALSE(tokens.empty());
    
}
// ============================================================================
// Enhanced Tokenizer Tests
// ============================================================================

TEST_F(TokenizerTest, TokenizeWithPositions_BasicTest) {
    auto tokens = tokenizer.tokenizeWithPositions("Hello World");
    
    ASSERT_EQ(tokens.size(), 2);
    
    // First token
    EXPECT_EQ(tokens[0].text, "hello");
    EXPECT_EQ(tokens[0].position, 0);
    EXPECT_EQ(tokens[0].start_offset, 0);
    EXPECT_EQ(tokens[0].end_offset, 5);
    
    // Second token
    EXPECT_EQ(tokens[1].text, "world");
    EXPECT_EQ(tokens[1].position, 1);
    EXPECT_EQ(tokens[1].start_offset, 6);
    EXPECT_EQ(tokens[1].end_offset, 11);
}

TEST_F(TokenizerTest, TokenizeWithPositions_WithStopWords) {
    tokenizer.setRemoveStopwords(true);
    
    auto tokens = tokenizer.tokenizeWithPositions("the quick brown fox");
    
    // "the" should be filtered, positions should be 0, 1, 2 (not 1, 2, 3)
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].text, "quick");
    EXPECT_EQ(tokens[0].position, 0);
    EXPECT_EQ(tokens[1].text, "brown");
    EXPECT_EQ(tokens[1].position, 1);
    EXPECT_EQ(tokens[2].text, "fox");
    EXPECT_EQ(tokens[2].position, 2);
}

TEST_F(TokenizerTest, TokenizeWithPositions_CharacterOffsets) {
    auto tokens = tokenizer.tokenizeWithPositions("The quick brown");
    
    ASSERT_EQ(tokens.size(), 2);  // "The" is stopword
    
    // "quick" starts at position 4 (after "The ")
    EXPECT_EQ(tokens[0].text, "quick");
    EXPECT_EQ(tokens[0].start_offset, 4);
    EXPECT_EQ(tokens[0].end_offset, 9);
    
    // "brown" starts at position 10 (after "The quick ")
    EXPECT_EQ(tokens[1].text, "brown");
    EXPECT_EQ(tokens[1].start_offset, 10);
    EXPECT_EQ(tokens[1].end_offset, 15);
}

TEST_F(TokenizerTest, StopwordRemovalToggle) {
    // With stopword removal enabled (default)
    tokenizer.setRemoveStopwords(true);
    auto tokens = tokenizer.tokenize("the cat and dog");
    EXPECT_EQ(tokens.size(), 2);  // "the" and "and" removed
    EXPECT_EQ(tokens[0], "cat");
    EXPECT_EQ(tokens[1], "dog");
    
    // With stopword removal disabled
    tokenizer.setRemoveStopwords(false);
    tokens = tokenizer.tokenize("the cat and dog");
    EXPECT_EQ(tokens.size(), 4);  // All tokens kept
    EXPECT_EQ(tokens[0], "the");
    EXPECT_EQ(tokens[1], "cat");
    EXPECT_EQ(tokens[2], "and");
    EXPECT_EQ(tokens[3], "dog");
}

TEST_F(TokenizerTest, SimpleStemming) {
    tokenizer.setStemmer(StemmerType::SIMPLE);
    
    auto tokens = tokenizer.tokenize("running walked quickly");
    
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "runn");     // "running" -> "runn" (removes "ing")
    EXPECT_EQ(tokens[1], "walk");     // "walked" -> "walk" (removes "ed")
    EXPECT_EQ(tokens[2], "quick");    // "quickly" -> "quick" (removes "ly")
}

TEST_F(TokenizerTest, SimpleStemming_Plurals) {
    tokenizer.setStemmer(StemmerType::SIMPLE);
    
    auto tokens = tokenizer.tokenize("cats dogs running");
    
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "cat");      // "cats" -> "cat"
    EXPECT_EQ(tokens[1], "dog");      // "dogs" -> "dog"
    EXPECT_EQ(tokens[2], "runn");     // "running" -> "runn"
}

TEST_F(TokenizerTest, SimpleStemming_ComplexSuffixes) {
    tokenizer.setStemmer(StemmerType::SIMPLE);
    
    auto tokens = tokenizer.tokenize("educational national");
    
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "education");   // "educational" -> "education" (removes "al")
    EXPECT_EQ(tokens[1], "nation");      // "national" -> "nation" (removes "al")
}

TEST_F(TokenizerTest, NoStemming) {
    tokenizer.setStemmer(StemmerType::NONE);
    
    auto tokens = tokenizer.tokenize("running walked");
    
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "running");   // No stemming
    EXPECT_EQ(tokens[1], "walked");    // No stemming
}

TEST_F(TokenizerTest, ApostropheHandling) {
    tokenizer.setRemoveStopwords(false);  // Keep all words
    auto tokens = tokenizer.tokenize("don't can't won't it's");
    
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0], "don't");
    EXPECT_EQ(tokens[1], "can't");
    EXPECT_EQ(tokens[2], "won't");
    EXPECT_EQ(tokens[3], "it's");  // Apostrophe is preserved
}

TEST_F(TokenizerTest, EmptyAndWhitespace) {
    // Empty string
    auto tokens = tokenizer.tokenize("");
    EXPECT_TRUE(tokens.empty());
    
    // Only whitespace
    tokens = tokenizer.tokenize("   \t\n  ");
    EXPECT_TRUE(tokens.empty());
    
    // Multiple spaces between words
    tokens = tokenizer.tokenize("hello    world");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
}

TEST_F(TokenizerTest, SpecialCharacters) {
    auto tokens = tokenizer.tokenize("hello@world.com test#123");
    
    // Should split on special characters: hello, world, com, test, 123
    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "com");
    EXPECT_EQ(tokens[3], "test");
    EXPECT_EQ(tokens[4], "123");
}

TEST_F(TokenizerTest, NumbersHandling) {
    auto tokens = tokenizer.tokenize("2024 test 123abc");
    
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "2024");
    EXPECT_EQ(tokens[1], "test");
    EXPECT_EQ(tokens[2], "123abc");   // Alphanumeric kept together
}

TEST_F(TokenizerTest, MixedCase) {
    tokenizer.setLowercase(true);
    auto tokens = tokenizer.tokenize("HeLLo WoRLd");
    
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
}

TEST_F(TokenizerTest, LongText) {
    std::string long_text = "This is a longer text with many words to test the tokenizer's "
                            "performance and correctness on larger inputs with various punctuation marks!";
    
    auto tokens = tokenizer.tokenize(long_text);
    
    // Should handle long text without issues
    EXPECT_GT(tokens.size(), 10);
    EXPECT_LT(tokens.size(), 30);  // Reasonable upper bound after stopword removal
}

TEST_F(TokenizerTest, SIMD_Support_Detection) {
    // Test that SIMD detection works
    bool simd_supported = Tokenizer::detectSIMDSupport();
    
    // Should return true on modern x86_64 and ARM systems, false on others
    // This is platform-dependent
    #if defined(__AVX2__) || defined(__SSE4_2__) || defined(__ARM_NEON) || defined(__aarch64__)
        EXPECT_TRUE(simd_supported);
    #else
        EXPECT_FALSE(simd_supported);
    #endif
}

TEST_F(TokenizerTest, SIMD_Toggle) {
    Tokenizer t1, t2;
    
    // Disable SIMD
    t1.enableSIMD(false);
    auto tokens1 = t1.tokenize("HELLO World");
    
    // Enable SIMD (if supported)
    t2.enableSIMD(true);
    auto tokens2 = t2.tokenize("HELLO World");
    
    // Results should be identical regardless of SIMD usage
    ASSERT_EQ(tokens1.size(), tokens2.size());
    for (size_t i = 0; i < tokens1.size(); ++i) {
        EXPECT_EQ(tokens1[i], tokens2[i]);
    }
}

TEST_F(TokenizerTest, SIMD_LargeText) {
    // Test SIMD with text larger than SIMD register width (32 bytes for AVX2, 16 for SSE)
    std::string large_text = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG "
                             "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG "
                             "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG";
    
    Tokenizer t1, t2;
    t1.enableSIMD(false);
    t2.enableSIMD(true);
    
    auto tokens1 = t1.tokenize(large_text);
    auto tokens2 = t2.tokenize(large_text);
    
    // Results should be identical
    ASSERT_EQ(tokens1.size(), tokens2.size());
    for (size_t i = 0; i < tokens1.size(); ++i) {
        EXPECT_EQ(tokens1[i], tokens2[i]) << "Mismatch at position " << i;
    }
}

TEST_F(TokenizerTest, PositionTracking_WithStemming) {
    tokenizer.setStemmer(StemmerType::SIMPLE);
    
    auto tokens = tokenizer.tokenizeWithPositions("running quickly");
    
    ASSERT_EQ(tokens.size(), 2);
    
    // First token - stemmed but position/offset intact
    EXPECT_EQ(tokens[0].text, "runn");
    EXPECT_EQ(tokens[0].position, 0);
    EXPECT_EQ(tokens[0].start_offset, 0);
    
    // Second token
    EXPECT_EQ(tokens[1].text, "quick");
    EXPECT_EQ(tokens[1].position, 1);
    EXPECT_EQ(tokens[1].start_offset, 8);
}

TEST_F(TokenizerTest, DefaultStopWords) {
    // Tokenizer should have default stop words
    tokenizer.setRemoveStopwords(true);
    
    auto tokens = tokenizer.tokenize("the a an is are was were");
    
    // Most common stop words should be filtered, but not all might be in default set
    // Check that at least some were removed
    EXPECT_LT(tokens.size(), 7);  // Should have removed at least some
    
    // Test specific common stopwords
    tokens = tokenizer.tokenize("the quick brown fox");
    EXPECT_EQ(tokens.size(), 3);  // "the" should be removed
    EXPECT_EQ(tokens[0], "quick");
}

TEST_F(TokenizerTest, ShortWords) {
    // Test handling of very short words
    auto tokens = tokenizer.tokenize("a I be");
    
    // "a" is stopword, others might be too
    EXPECT_LE(tokens.size(), 3);
}

TEST_F(TokenizerTest, ConsecutivePunctuation) {
    auto tokens = tokenizer.tokenize("hello...world!!!test???");
    
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "test");
}

TEST_F(TokenizerTest, Unicode_BasicASCII) {
    // Test basic ASCII handling (full Unicode is complex)
    auto tokens = tokenizer.tokenize("hello world");
    
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
}