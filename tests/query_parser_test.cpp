#include <gtest/gtest.h>
#include "query_parser.hpp"

using namespace search_engine;

class QueryParserTest : public ::testing::Test {
protected:
    QueryParser parser;
};

TEST_F(QueryParserTest, SimpleTerms) {
    // TODO: Test parsing simple terms
}

TEST_F(QueryParserTest, BooleanAND) {
    // TODO: Test parsing AND operator
}

TEST_F(QueryParserTest, BooleanOR) {
    // TODO: Test parsing OR operator
}

TEST_F(QueryParserTest, BooleanNOT) {
    // TODO: Test parsing NOT operator
}

TEST_F(QueryParserTest, PhraseQuery) {
    // TODO: Test parsing phrase in quotes
}

TEST_F(QueryParserTest, MalformedQuery) {
    // TODO: Test handling of malformed queries
}
