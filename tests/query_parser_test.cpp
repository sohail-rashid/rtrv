#include <gtest/gtest.h>
#include "query_parser.hpp"

using namespace rtrv_search_engine;

class QueryParserTest : public ::testing::Test {
protected:
    QueryParser parser;
};

TEST_F(QueryParserTest, SimpleTerms) {
    // Test single term
    auto node = parser.parse("hello");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::TERM);
    auto* term_node = dynamic_cast<TermNode*>(node.get());
    ASSERT_NE(term_node, nullptr);
    EXPECT_EQ(term_node->term, "hello");
    
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
    EXPECT_EQ(node->getType(), QueryNode::Type::AND);
    
    auto* and_node = dynamic_cast<AndNode*>(node.get());
    ASSERT_NE(and_node, nullptr);
    EXPECT_EQ(and_node->children.size(), 2);
    
    // Check left child
    auto* left = dynamic_cast<TermNode*>(and_node->children[0].get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->term, "search");
    
    // Check right child
    auto* right = dynamic_cast<TermNode*>(and_node->children[1].get());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->term, "engine");
    
    // Test extractTerms filters out AND
    auto terms = parser.extractTerms("search AND engine");
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0], "search");
    EXPECT_EQ(terms[1], "engine");
}

TEST_F(QueryParserTest, BooleanOR) {
    auto node = parser.parse("cat OR dog");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::OR);
    
    auto* or_node = dynamic_cast<OrNode*>(node.get());
    ASSERT_NE(or_node, nullptr);
    EXPECT_EQ(or_node->children.size(), 2);
    
    // Check left child
    auto* left = dynamic_cast<TermNode*>(or_node->children[0].get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->term, "cat");
    
    // Check right child
    auto* right = dynamic_cast<TermNode*>(or_node->children[1].get());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->term, "dog");
    
    // Test extractTerms filters out OR
    auto terms = parser.extractTerms("cat OR dog");
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0], "cat");
    EXPECT_EQ(terms[1], "dog");
}

TEST_F(QueryParserTest, BooleanNOT) {
    auto node = parser.parse("NOT spam");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::NOT);
    
    auto* not_node = dynamic_cast<NotNode*>(node.get());
    ASSERT_NE(not_node, nullptr);
    ASSERT_NE(not_node->child, nullptr);
    
    // Check child
    auto* child = dynamic_cast<TermNode*>(not_node->child.get());
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->term, "spam");
    
    // Test extractTerms filters out NOT
    auto terms = parser.extractTerms("NOT spam");
    ASSERT_EQ(terms.size(), 1);
    EXPECT_EQ(terms[0], "spam");
}

TEST_F(QueryParserTest, PhraseQuery) {
    auto node = parser.parse("\"search engine\"");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::PHRASE);
    
    auto* phrase_node = dynamic_cast<PhraseNode*>(node.get());
    ASSERT_NE(phrase_node, nullptr);
    ASSERT_EQ(phrase_node->terms.size(), 2);
    EXPECT_EQ(phrase_node->terms[0], "search");
    EXPECT_EQ(phrase_node->terms[1], "engine");
    EXPECT_EQ(phrase_node->max_distance, 0);
    
    // Test extractTerms preserves phrase as single term
    auto terms = parser.extractTerms("\"search engine\"");
    ASSERT_EQ(terms.size(), 1);
    EXPECT_EQ(terms[0], "search engine");
    
    // Test phrase with multiple words
    node = parser.parse("\"the quick brown fox\"");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::PHRASE);
    phrase_node = dynamic_cast<PhraseNode*>(node.get());
    ASSERT_NE(phrase_node, nullptr);
    ASSERT_EQ(phrase_node->terms.size(), 4);
    EXPECT_EQ(phrase_node->terms[0], "the");
    EXPECT_EQ(phrase_node->terms[3], "fox");
}

TEST_F(QueryParserTest, MalformedQuery) {
    // Empty query
    auto terms = parser.extractTerms("");
    EXPECT_TRUE(terms.empty());
    
    // Only whitespace
    terms = parser.extractTerms("   \t\n  ");
    EXPECT_TRUE(terms.empty());
    
    // Unclosed quote - should fallback to simple term
    auto node = parser.parse("\"incomplete");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::TERM);
    
    // Only boolean operators
    terms = parser.extractTerms("AND OR NOT");
    EXPECT_TRUE(terms.empty());
    
    // Multiple spaces
    terms = parser.extractTerms("hello    world");
    ASSERT_EQ(terms.size(), 2);
    EXPECT_EQ(terms[0], "hello");
    EXPECT_EQ(terms[1], "world");
}

// New tests for enhanced features

TEST_F(QueryParserTest, FieldedQuery) {
    auto node = parser.parse("title:machine");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::FIELD);
    
    auto* field_node = dynamic_cast<FieldNode*>(node.get());
    ASSERT_NE(field_node, nullptr);
    EXPECT_EQ(field_node->field_name, "title");
    
    auto* term = dynamic_cast<TermNode*>(field_node->query.get());
    ASSERT_NE(term, nullptr);
    EXPECT_EQ(term->term, "machine");
}

TEST_F(QueryParserTest, FieldedPhraseQuery) {
    auto node = parser.parse("content:\"machine learning\"");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::FIELD);
    
    auto* field_node = dynamic_cast<FieldNode*>(node.get());
    ASSERT_NE(field_node, nullptr);
    EXPECT_EQ(field_node->field_name, "content");
    
    auto* phrase = dynamic_cast<PhraseNode*>(field_node->query.get());
    ASSERT_NE(phrase, nullptr);
    ASSERT_EQ(phrase->terms.size(), 2);
    EXPECT_EQ(phrase->terms[0], "machine");
    EXPECT_EQ(phrase->terms[1], "learning");
}

TEST_F(QueryParserTest, ProximityQuery) {
    auto node = parser.parse("\"machine learning\"~5");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::PHRASE);
    
    auto* phrase_node = dynamic_cast<PhraseNode*>(node.get());
    ASSERT_NE(phrase_node, nullptr);
    ASSERT_EQ(phrase_node->terms.size(), 2);
    EXPECT_EQ(phrase_node->terms[0], "machine");
    EXPECT_EQ(phrase_node->terms[1], "learning");
    EXPECT_EQ(phrase_node->max_distance, 5);
}

TEST_F(QueryParserTest, NestedQuery) {
    auto node = parser.parse("(cat OR dog) AND animal");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::AND);
    
    auto* and_node = dynamic_cast<AndNode*>(node.get());
    ASSERT_NE(and_node, nullptr);
    ASSERT_EQ(and_node->children.size(), 2);
    
    // Left should be OR
    auto* or_node = dynamic_cast<OrNode*>(and_node->children[0].get());
    ASSERT_NE(or_node, nullptr);
    EXPECT_EQ(or_node->children.size(), 2);
    
    // Right should be term
    auto* term = dynamic_cast<TermNode*>(and_node->children[1].get());
    ASSERT_NE(term, nullptr);
    EXPECT_EQ(term->term, "animal");
}

TEST_F(QueryParserTest, ImplicitAND) {
    auto node = parser.parse("machine learning AI");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::AND);
    
    auto* and_node = dynamic_cast<AndNode*>(node.get());
    ASSERT_NE(and_node, nullptr);
    EXPECT_EQ(and_node->children.size(), 3);
}

TEST_F(QueryParserTest, ComplexQuery) {
    auto node = parser.parse("(title:ai OR title:machine) AND content:learning NOT deprecated");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->getType(), QueryNode::Type::AND);
    
    // Verify toString works
    std::string str = node->toString();
    EXPECT_FALSE(str.empty());
}
