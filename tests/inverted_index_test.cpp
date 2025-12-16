#include <gtest/gtest.h>
#include "inverted_index.hpp"
#include <thread>
#include <vector>

using namespace search_engine;

class InvertedIndexTest : public ::testing::Test {
protected:
    InvertedIndex index;
};

TEST_F(InvertedIndexTest, AddAndRetrievePostings) {
    // TODO: Test adding and retrieving postings
}

TEST_F(InvertedIndexTest, DocumentRemoval) {
    // TODO: Test document removal
}

TEST_F(InvertedIndexTest, DocumentFrequency) {
    // TODO: Test document frequency calculation
}

TEST_F(InvertedIndexTest, ThreadSafety) {
    // TODO: Test thread-safe concurrent operations
    // Multiple threads adding terms
    // Verify correctness after all threads complete
}

TEST_F(InvertedIndexTest, Clear) {
    // TODO: Test clearing the index
}
