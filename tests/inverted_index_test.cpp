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

// ==================== Skip Pointer Tests ====================

TEST_F(InvertedIndexTest, SkipPointerBuilding) {
    // Add a term to many documents
    for (uint64_t doc_id = 1; doc_id <= 100; ++doc_id) {
        index.addTerm("popular", doc_id);
    }
    
    // Get posting list with skip pointers
    PostingList list = index.getPostingList("popular");
    
    // Verify postings exist
    EXPECT_EQ(list.postings.size(), 100);
    
    // Verify skip pointers are built (sqrt(100) = 10)
    EXPECT_GT(list.skip_pointers.size(), 0);
    EXPECT_LE(list.skip_pointers.size(), 15);  // Roughly sqrt(100)
    
    // Verify skip pointers are in ascending order
    for (size_t i = 1; i < list.skip_pointers.size(); ++i) {
        EXPECT_GT(list.skip_pointers[i].position, list.skip_pointers[i-1].position);
        EXPECT_GT(list.skip_pointers[i].doc_id, list.skip_pointers[i-1].doc_id);
    }
    
    // Verify first skip pointer points to beginning
    EXPECT_EQ(list.skip_pointers[0].position, 0);
    EXPECT_EQ(list.skip_pointers[0].doc_id, 1);
}

TEST_F(InvertedIndexTest, SkipPointerCustomInterval) {
    // Add a term to many documents
    for (uint64_t doc_id = 1; doc_id <= 100; ++doc_id) {
        index.addTerm("test", doc_id);
    }
    
    PostingList list = index.getPostingList("test");
    
    // Test different skip intervals
    list.buildSkipPointers(10);  // Every 10 postings
    EXPECT_EQ(list.skip_pointers.size(), 10);  // 100/10 = 10
    
    list.buildSkipPointers(25);  // Every 25 postings
    EXPECT_EQ(list.skip_pointers.size(), 4);   // 100/25 = 4
    
    list.buildSkipPointers(1);   // Every posting (not practical but valid)
    EXPECT_EQ(list.skip_pointers.size(), 100);
}

TEST_F(InvertedIndexTest, SkipPointerFindTarget) {
    // Create a posting list with documents 10, 20, 30, ..., 1000
    for (uint64_t doc_id = 10; doc_id <= 1000; doc_id += 10) {
        index.addTerm("sequence", doc_id);
    }
    
    PostingList list = index.getPostingList("sequence");
    ASSERT_EQ(list.postings.size(), 100);
    
    // Test finding skip targets
    size_t pos = list.findSkipTarget(250);
    EXPECT_LE(list.postings[pos].doc_id, 250);  // Should be at or before target
    
    pos = list.findSkipTarget(500);
    EXPECT_LE(list.postings[pos].doc_id, 500);
    
    pos = list.findSkipTarget(1);  // Before all documents
    EXPECT_EQ(pos, 0);
    
    pos = list.findSkipTarget(2000);  // After all documents
    EXPECT_LT(pos, list.postings.size());
}

TEST_F(InvertedIndexTest, IntersectWithSkips) {
    // Create two posting lists with some overlap
    // List 1: documents 1, 2, 3, ..., 100
    for (uint64_t doc_id = 1; doc_id <= 100; ++doc_id) {
        index.addTerm("term1", doc_id);
    }
    
    // List 2: documents 50, 60, 70, ..., 150 (overlap at 50-100)
    for (uint64_t doc_id = 50; doc_id <= 150; doc_id += 10) {
        index.addTerm("term2", doc_id);
    }
    
    PostingList list1 = index.getPostingList("term1");
    PostingList list2 = index.getPostingList("term2");
    
    // Perform intersection with skip pointers
    std::vector<uint64_t> result = intersectWithSkips(list1, list2);
    
    // Expected: 50, 60, 70, 80, 90, 100 (6 documents)
    EXPECT_EQ(result.size(), 6);
    
    // Verify results are correct
    std::vector<uint64_t> expected = {50, 60, 70, 80, 90, 100};
    EXPECT_EQ(result, expected);
}

TEST_F(InvertedIndexTest, IntersectWithSkipsNoOverlap) {
    // Create two posting lists with NO overlap
    for (uint64_t doc_id = 1; doc_id <= 50; ++doc_id) {
        index.addTerm("early", doc_id);
    }
    
    for (uint64_t doc_id = 100; doc_id <= 150; ++doc_id) {
        index.addTerm("late", doc_id);
    }
    
    PostingList list1 = index.getPostingList("early");
    PostingList list2 = index.getPostingList("late");
    
    std::vector<uint64_t> result = intersectWithSkips(list1, list2);
    
    // No overlap expected
    EXPECT_TRUE(result.empty());
}

TEST_F(InvertedIndexTest, IntersectWithSkipsCompleteOverlap) {
    // Create two posting lists with complete overlap
    for (uint64_t doc_id = 1; doc_id <= 50; ++doc_id) {
        index.addTerm("alpha", doc_id);
        index.addTerm("beta", doc_id);
    }
    
    PostingList list1 = index.getPostingList("alpha");
    PostingList list2 = index.getPostingList("beta");
    
    std::vector<uint64_t> result = intersectWithSkips(list1, list2);
    
    // Complete overlap expected
    EXPECT_EQ(result.size(), 50);
    
    // Verify all documents are present
    for (size_t i = 0; i < result.size(); ++i) {
        EXPECT_EQ(result[i], i + 1);
    }
}

TEST_F(InvertedIndexTest, SkipPointerLazyBuilding) {
    // Add terms
    for (uint64_t doc_id = 1; doc_id <= 100; ++doc_id) {
        index.addTerm("lazy", doc_id);
    }
    
    // Get posting list - skip pointers should be built lazily
    PostingList list1 = index.getPostingList("lazy");
    EXPECT_GT(list1.skip_pointers.size(), 0);
    EXPECT_FALSE(list1.needsSkipRebuild());
    
    // Add more terms - should mark skips as dirty
    index.addTerm("lazy", 101);
    
    // Get list again - skip pointers should be rebuilt
    PostingList list2 = index.getPostingList("lazy");
    EXPECT_GT(list2.skip_pointers.size(), 0);
    EXPECT_EQ(list2.postings.size(), 101);
}

TEST_F(InvertedIndexTest, SkipPointerRebuildAll) {
    // Add multiple terms
    for (uint64_t doc_id = 1; doc_id <= 100; ++doc_id) {
        index.addTerm("term_a", doc_id);
        index.addTerm("term_b", doc_id);
        index.addTerm("term_c", doc_id);
    }
    
    // Manually rebuild all skip pointers
    index.rebuildSkipPointers();
    
    // Verify skip pointers are built for all terms
    PostingList list_a = index.getPostingList("term_a");
    PostingList list_b = index.getPostingList("term_b");
    PostingList list_c = index.getPostingList("term_c");
    
    EXPECT_GT(list_a.skip_pointers.size(), 0);
    EXPECT_GT(list_b.skip_pointers.size(), 0);
    EXPECT_GT(list_c.skip_pointers.size(), 0);
}

TEST_F(InvertedIndexTest, SkipPointerWithPositions) {
    // Add terms with positions (for phrase search)
    index.addTerm("positioned", 1, 10);
    index.addTerm("positioned", 1, 20);
    index.addTerm("positioned", 2, 5);
    index.addTerm("positioned", 3, 15);
    
    PostingList list = index.getPostingList("positioned");
    
    // Verify postings have positions
    EXPECT_EQ(list.postings.size(), 3);
    
    auto it = std::find_if(list.postings.begin(), list.postings.end(),
                          [](const Posting& p) { return p.doc_id == 1; });
    ASSERT_NE(it, list.postings.end());
    EXPECT_EQ(it->positions.size(), 2);
    EXPECT_EQ(it->positions[0], 10);
    EXPECT_EQ(it->positions[1], 20);
    
    // Skip pointers should still work
    EXPECT_GT(list.skip_pointers.size(), 0);
}

TEST_F(InvertedIndexTest, SkipPointerEmptyList) {
    // Get posting list for non-existent term
    PostingList empty_list = index.getPostingList("nonexistent");
    
    EXPECT_TRUE(empty_list.postings.empty());
    EXPECT_TRUE(empty_list.skip_pointers.empty());
    
    // Building skip pointers on empty list should not crash
    empty_list.buildSkipPointers();
    EXPECT_TRUE(empty_list.skip_pointers.empty());
}

TEST_F(InvertedIndexTest, SkipPointerAfterDocumentRemoval) {
    // Add terms
    for (uint64_t doc_id = 1; doc_id <= 100; ++doc_id) {
        index.addTerm("removable", doc_id);
    }
    
    // Get initial posting list
    PostingList list1 = index.getPostingList("removable");
    EXPECT_EQ(list1.postings.size(), 100);
    EXPECT_GT(list1.skip_pointers.size(), 0);
    
    // Remove some documents
    for (uint64_t doc_id = 50; doc_id <= 60; ++doc_id) {
        index.removeDocument(doc_id);
    }
    
    // Get updated posting list
    PostingList list2 = index.getPostingList("removable");
    EXPECT_EQ(list2.postings.size(), 89);  // 100 - 11 removed
    
    // Skip pointers should be rebuilt with new size
    EXPECT_GT(list2.skip_pointers.size(), 0);
    
    // Intersection should still work correctly
    for (uint64_t doc_id = 1; doc_id <= 100; doc_id += 5) {
        index.addTerm("sparse", doc_id);
    }
    
    PostingList sparse_list = index.getPostingList("sparse");
    std::vector<uint64_t> result = intersectWithSkips(list2, sparse_list);
    
    EXPECT_GT(result.size(), 0);
}
