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
    // Add terms for different documents
    index.addTerm("hello", 1);
    index.addTerm("world", 1);
    index.addTerm("hello", 2);
    index.addTerm("hello", 2);  // Add same term again to increment frequency
    
    // Retrieve postings for "hello"
    auto postings = index.getPostings("hello");
    ASSERT_EQ(postings.size(), 2);  // Should be in 2 documents
    
    // Check document 1
    auto it1 = std::find_if(postings.begin(), postings.end(),
                           [](const Posting& p) { return p.doc_id == 1; });
    ASSERT_NE(it1, postings.end());
    EXPECT_EQ(it1->term_frequency, 1);
    
    // Check document 2
    auto it2 = std::find_if(postings.begin(), postings.end(),
                           [](const Posting& p) { return p.doc_id == 2; });
    ASSERT_NE(it2, postings.end());
    EXPECT_EQ(it2->term_frequency, 2);  // Added twice
    
    // Retrieve postings for "world"
    auto world_postings = index.getPostings("world");
    ASSERT_EQ(world_postings.size(), 1);
    EXPECT_EQ(world_postings[0].doc_id, 1);
    
    // Non-existent term
    auto empty_postings = index.getPostings("nonexistent");
    EXPECT_TRUE(empty_postings.empty());
}

TEST_F(InvertedIndexTest, DocumentRemoval) {
    // Add terms for multiple documents
    index.addTerm("apple", 1);
    index.addTerm("banana", 1);
    index.addTerm("apple", 2);
    index.addTerm("cherry", 2);
    index.addTerm("apple", 3);
    
    // Verify initial state
    auto apple_postings = index.getPostings("apple");
    EXPECT_EQ(apple_postings.size(), 3);
    
    // Remove document 2
    index.removeDocument(2);
    
    // Verify "apple" now only in 2 documents
    apple_postings = index.getPostings("apple");
    EXPECT_EQ(apple_postings.size(), 2);
    
    // Verify document 2 is gone from "apple"
    auto it = std::find_if(apple_postings.begin(), apple_postings.end(),
                          [](const Posting& p) { return p.doc_id == 2; });
    EXPECT_EQ(it, apple_postings.end());
    
    // Verify "cherry" posting list is now empty or removed
    auto cherry_postings = index.getPostings("cherry");
    EXPECT_TRUE(cherry_postings.empty());
    
    // Verify "banana" exists
    auto banana_postings = index.getPostings("banana");
    EXPECT_FALSE(banana_postings.empty());
}

TEST_F(InvertedIndexTest, DocumentFrequency) {
    // Add terms across multiple documents
    index.addTerm("common", 1);
    index.addTerm("common", 2);
    index.addTerm("common", 3);
    index.addTerm("rare", 1);
    
    // Check document frequency
    EXPECT_EQ(index.getDocumentFrequency("common"), 3);
    EXPECT_EQ(index.getDocumentFrequency("rare"), 1);
    EXPECT_EQ(index.getDocumentFrequency("nonexistent"), 0);
    
    // Add more occurrences in same document - shouldn't change doc frequency
    index.addTerm("common", 1);
    index.addTerm("common", 1);
    EXPECT_EQ(index.getDocumentFrequency("common"), 3);  // Still 3 documents
}

TEST_F(InvertedIndexTest, ThreadSafety) {
    const int num_threads = 10;
    const int terms_per_thread = 100;
    std::vector<std::thread> threads;
    
    // Multiple threads adding terms concurrently
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < terms_per_thread; ++j) {
                std::string term = "term" + std::to_string(j);
                index.addTerm(term, i);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify correctness - each term should be in num_threads documents
    for (int j = 0; j < terms_per_thread; ++j) {
        std::string term = "term" + std::to_string(j);
        auto postings = index.getPostings(term);
        EXPECT_EQ(postings.size(), num_threads);
        EXPECT_EQ(index.getDocumentFrequency(term), num_threads);
    }
    
    // Test concurrent reads
    threads.clear();
    std::atomic<int> read_count{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &read_count]() {
            auto postings = index.getPostings("term0");
            if (!postings.empty()) {
                read_count++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(read_count.load(), num_threads);
}

TEST_F(InvertedIndexTest, Clear) {
    // Add some terms
    index.addTerm("term1", 1);
    index.addTerm("term2", 2);
    index.addTerm("term3", 3);
    
    // Verify index has terms
    EXPECT_GT(index.getTermCount(), 0);
    EXPECT_FALSE(index.getPostings("term1").empty());
    
    // Clear the index
    index.clear();
    
    // Verify index is empty
    EXPECT_EQ(index.getTermCount(), 0);
    EXPECT_TRUE(index.getPostings("term1").empty());
    EXPECT_TRUE(index.getPostings("term2").empty());
    EXPECT_EQ(index.getDocumentFrequency("term1"), 0);
}
