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
