#include <gtest/gtest.h>
#include "query_parser.hpp"

using namespace search_engine;

class QueryParserTest : public ::testing::Test {
protected:
    QueryParser parser;
};

TEST_F(QueryParserTest, SimpleTerms) {
    // Test single term
    auto node = parser.parse("hello");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->type, QueryType::TERM);
    EXPECT_EQ(node->value, "hello");
    
    // Test extractTerms with simple query
    auto terms = parser.extractTerms("hello world");
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0], "hello");
    EXPECT_EQ(terms[1], "world");
    
    // Test with punctuation
    terms = parser.extractTerms("hello, world!");
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0], "hello");
    EXPECT_EQ(terms[1], "world");
    
    // Test case insensitivity
    terms = parser.extractTerms("Hello WORLD");
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0], "hello");
    EXPECT_EQ(terms[1], "world");
}

TEST_F(QueryParserTest, BooleanAND) {
    auto node = parser.parse("search AND engine");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->type, QueryType::AND);
    EXPECT_EQ(node->children.size(), 2);
    
    // Check left child
    ASSERT_NE(node->children[0], nullptr);
    EXPECT_EQ(node->children[0]->type, QueryType::TERM);
    EXPECT_EQ(node->children[0]->value, "search");
    
    // Check right child
    ASSERT_NE(node->children[1], nullptr);
    EXPECT_EQ(node->children[1]->type, QueryType::TERM);
    EXPECT_EQ(node->children[1]->value, "engine");
    
    // Test extractTerms filters out AND
    auto terms = parser.extractTerms("search AND engine");
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0], "search");
    EXPECT_EQ(terms[1], "engine");
}

TEST_F(QueryParserTest, BooleanOR) {
    auto node = parser.parse("cat OR dog");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->type, QueryType::OR);
    EXPECT_EQ(node->children.size(), 2);
    
    // Check left child
    ASSERT_NE(node->children[0], nullptr);
    EXPECT_EQ(node->children[0]->type, QueryType::TERM);
    EXPECT_EQ(node->children[0]->value, "cat");
    
    // Check right child
    ASSERT_NE(node->children[1], nullptr);
    EXPECT_EQ(node->children[1]->type, QueryType::TERM);
    EXPECT_EQ(node->children[1]->value, "dog");
    
    // Test extractTerms filters out OR
    auto terms = parser.extractTerms("cat OR dog");
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0], "cat");
    EXPECT_EQ(terms[1], "dog");
}

TEST_F(QueryParserTest, BooleanNOT) {
    auto node = parser.parse("NOT spam");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->type, QueryType::NOT);
    EXPECT_EQ(node->children.size(), 1);
    
    // Check child
    ASSERT_NE(node->children[0], nullptr);
    EXPECT_EQ(node->children[0]->type, QueryType::TERM);
    EXPECT_EQ(node->children[0]->value, "spam");
    
    // Test extractTerms filters out NOT
    auto terms = parser.extractTerms("NOT spam");
    ASSERT_EQ(terms.size(), 1);
    EXPECT_EQ(terms[0], "spam");
}

TEST_F(QueryParserTest, PhraseQuery) {
    auto node = parser.parse("\"search engine\"");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->type, QueryType::PHRASE);
    EXPECT_EQ(node->value, "search engine");
    
    // Test extractTerms preserves phrase as single term
    auto terms = parser.extractTerms("\"search engine\"");
    ASSERT_EQ(terms.size(), 1);
    EXPECT_EQ(terms[0], "search engine");
    
    // Test phrase with multiple words
    node = parser.parse("\"the quick brown fox\"");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->type, QueryType::PHRASE);
    EXPECT_EQ(node->value, "the quick brown fox");
}

TEST_F(QueryParserTest, MalformedQuery) {
    // Empty query
    auto terms = parser.extractTerms("");
    EXPECT_TRUE(terms.empty());
    
    // Only whitespace
    terms = parser.extractTerms("   \t\n  ");
    EXPECT_TRUE(terms.empty());
    
    // Unclosed quote - should still parse
    auto node = parser.parse("\"incomplete");
    ASSERT_NE(node, nullptr);
    // Should treat as term since quote is not closed
    EXPECT_EQ(node->type, QueryType::TERM);
    
    // Only boolean operators
    terms = parser.extractTerms("AND OR NOT");
    EXPECT_TRUE(terms.empty());
    
    // Multiple spaces
    terms = parser.extractTerms("hello    world");
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0], "hello");
    EXPECT_EQ(terms[1], "world");
}
