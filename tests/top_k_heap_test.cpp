#include <gtest/gtest.h>
#include "top_k_heap.hpp"

using namespace rtrv_search_engine;

class TopKHeapTest : public ::testing::Test {
protected:
    // Helper to create scored documents
    ScoredDocument makeDoc(uint64_t id, double score) {
        return {id, score};
    }
};

TEST_F(TopKHeapTest, BasicInsertion) {
    BoundedPriorityQueue<ScoredDocument> heap(3);
    
    heap.push(makeDoc(1, 10.0));
    heap.push(makeDoc(2, 20.0));
    heap.push(makeDoc(3, 15.0));
    
    EXPECT_EQ(heap.size(), 3);
    EXPECT_TRUE(heap.isFull());
    
    auto results = heap.getSorted();
    ASSERT_EQ(results.size(), 3);
    
    // Should be in descending order
    EXPECT_EQ(results[0].doc_id, 2);
    EXPECT_DOUBLE_EQ(results[0].score, 20.0);
    EXPECT_EQ(results[1].doc_id, 3);
    EXPECT_DOUBLE_EQ(results[1].score, 15.0);
    EXPECT_EQ(results[2].doc_id, 1);
    EXPECT_DOUBLE_EQ(results[2].score, 10.0);
}

TEST_F(TopKHeapTest, BoundedCapacity) {
    BoundedPriorityQueue<ScoredDocument> heap(3);
    
    // Insert 5 documents, only top 3 should remain
    heap.push(makeDoc(1, 10.0));
    heap.push(makeDoc(2, 20.0));
    heap.push(makeDoc(3, 15.0));
    heap.push(makeDoc(4, 5.0));   // Should be rejected
    heap.push(makeDoc(5, 25.0));  // Should replace worst
    
    EXPECT_EQ(heap.size(), 3);
    
    auto results = heap.getSorted();
    ASSERT_EQ(results.size(), 3);
    
    // Top 3: 25.0, 20.0, 15.0
    EXPECT_EQ(results[0].doc_id, 5);
    EXPECT_DOUBLE_EQ(results[0].score, 25.0);
    EXPECT_EQ(results[1].doc_id, 2);
    EXPECT_DOUBLE_EQ(results[1].score, 20.0);
    EXPECT_EQ(results[2].doc_id, 3);
    EXPECT_DOUBLE_EQ(results[2].score, 15.0);
}

TEST_F(TopKHeapTest, MinScoreTracking) {
    BoundedPriorityQueue<ScoredDocument> heap(3);
    
    EXPECT_DOUBLE_EQ(heap.minScore(), 0.0);  // Empty heap
    
    heap.push(makeDoc(1, 10.0));
    EXPECT_DOUBLE_EQ(heap.minScore(), 10.0);
    
    heap.push(makeDoc(2, 20.0));
    EXPECT_DOUBLE_EQ(heap.minScore(), 10.0);
    
    heap.push(makeDoc(3, 15.0));
    EXPECT_DOUBLE_EQ(heap.minScore(), 10.0);
    
    heap.push(makeDoc(4, 25.0));
    EXPECT_DOUBLE_EQ(heap.minScore(), 15.0);  // 10.0 was evicted
}

TEST_F(TopKHeapTest, EmptyHeap) {
    BoundedPriorityQueue<ScoredDocument> heap(5);
    
    EXPECT_TRUE(heap.empty());
    EXPECT_EQ(heap.size(), 0);
    EXPECT_FALSE(heap.isFull());
    
    auto results = heap.getSorted();
    EXPECT_TRUE(results.empty());
}

TEST_F(TopKHeapTest, SingleElement) {
    BoundedPriorityQueue<ScoredDocument> heap(1);
    
    heap.push(makeDoc(1, 10.0));
    EXPECT_TRUE(heap.isFull());
    
    heap.push(makeDoc(2, 5.0));   // Should be rejected
    EXPECT_EQ(heap.size(), 1);
    
    heap.push(makeDoc(3, 15.0));  // Should replace
    EXPECT_EQ(heap.size(), 1);
    
    auto results = heap.getSorted();
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].doc_id, 3);
    EXPECT_DOUBLE_EQ(results[0].score, 15.0);
}

TEST_F(TopKHeapTest, DuplicateScores) {
    BoundedPriorityQueue<ScoredDocument> heap(5);
    
    heap.push(makeDoc(1, 10.0));
    heap.push(makeDoc(2, 10.0));
    heap.push(makeDoc(3, 10.0));
    heap.push(makeDoc(4, 20.0));
    heap.push(makeDoc(5, 5.0));
    
    auto results = heap.getSorted();
    ASSERT_EQ(results.size(), 5);
    
    // Highest score first
    EXPECT_DOUBLE_EQ(results[0].score, 20.0);
    
    // Three documents with score 10.0 (deterministic order by doc_id)
    EXPECT_DOUBLE_EQ(results[1].score, 10.0);
    EXPECT_DOUBLE_EQ(results[2].score, 10.0);
    EXPECT_DOUBLE_EQ(results[3].score, 10.0);
    
    // Lowest score last
    EXPECT_DOUBLE_EQ(results[4].score, 5.0);
}

TEST_F(TopKHeapTest, PeekWithoutModifying) {
    BoundedPriorityQueue<ScoredDocument> heap(3);
    
    heap.push(makeDoc(1, 10.0));
    heap.push(makeDoc(2, 20.0));
    heap.push(makeDoc(3, 15.0));
    
    auto peeked = heap.peek();
    ASSERT_EQ(peeked.size(), 3);
    EXPECT_EQ(heap.size(), 3);  // Still has elements
    
    // Can still get sorted results
    auto sorted = heap.getSorted();
    EXPECT_EQ(sorted.size(), 3);
    EXPECT_EQ(heap.size(), 0);  // Now empty
}

TEST_F(TopKHeapTest, ClearHeap) {
    BoundedPriorityQueue<ScoredDocument> heap(3);
    
    heap.push(makeDoc(1, 10.0));
    heap.push(makeDoc(2, 20.0));
    heap.push(makeDoc(3, 15.0));
    
    EXPECT_EQ(heap.size(), 3);
    
    heap.clear();
    
    EXPECT_TRUE(heap.empty());
    EXPECT_EQ(heap.size(), 0);
}

TEST_F(TopKHeapTest, LargeDataset) {
    BoundedPriorityQueue<ScoredDocument> heap(10);
    
    // Insert 1000 documents, only top 10 should remain
    for (uint64_t i = 1; i <= 1000; ++i) {
        heap.push(makeDoc(i, static_cast<double>(i)));
    }
    
    EXPECT_EQ(heap.size(), 10);
    EXPECT_TRUE(heap.isFull());
    
    auto results = heap.getSorted();
    ASSERT_EQ(results.size(), 10);
    
    // Top 10: 1000, 999, 998, ..., 991
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(results[i].doc_id, 1000 - i);
        EXPECT_DOUBLE_EQ(results[i].score, 1000.0 - i);
    }
}

TEST_F(TopKHeapTest, DescendingInsertion) {
    BoundedPriorityQueue<ScoredDocument> heap(5);
    
    // Insert in descending order
    for (uint64_t i = 100; i > 0; --i) {
        heap.push(makeDoc(i, static_cast<double>(i)));
    }
    
    auto results = heap.getSorted();
    ASSERT_EQ(results.size(), 5);
    
    // Top 5: 100, 99, 98, 97, 96
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(results[i].doc_id, 100 - i);
        EXPECT_DOUBLE_EQ(results[i].score, 100.0 - i);
    }
}

TEST_F(TopKHeapTest, RandomScores) {
    BoundedPriorityQueue<ScoredDocument> heap(5);
    
    // Insert with random-ish scores
    heap.push(makeDoc(1, 42.5));
    heap.push(makeDoc(2, 17.3));
    heap.push(makeDoc(3, 99.9));
    heap.push(makeDoc(4, 3.14));
    heap.push(makeDoc(5, 50.0));
    heap.push(makeDoc(6, 75.5));
    heap.push(makeDoc(7, 8.88));
    
    auto results = heap.getSorted();
    ASSERT_EQ(results.size(), 5);
    
    // Verify descending order
    for (size_t i = 0; i < results.size() - 1; ++i) {
        EXPECT_GE(results[i].score, results[i + 1].score);
    }
    
    // Verify top score
    EXPECT_DOUBLE_EQ(results[0].score, 99.9);
}

TEST_F(TopKHeapTest, ZeroCapacity) {
    // Edge case: capacity 0 should allow 0 elements
    BoundedPriorityQueue<ScoredDocument> heap(0);
    
    EXPECT_EQ(heap.capacity(), 0);
    
    heap.push(makeDoc(1, 10.0));
    EXPECT_EQ(heap.size(), 0);  // Can't add anything
    EXPECT_TRUE(heap.empty());
}
