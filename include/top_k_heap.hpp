#pragma once

#include <queue>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace rtrv_search_engine {

/**
 * Scored document for top-K heap
 */
struct ScoredDocument {
    uint64_t doc_id;
    double score;
    
    // For min-heap with std::greater comparator:
    // We want HIGHER scores to have LOWER priority in the heap
    // So lowest score stays at top and gets evicted first
    bool operator>(const ScoredDocument& other) const {
        // Higher score means "greater than" for sorting
        if (score != other.score) {
            return score > other.score;
        }
        // Tie-breaker: lower doc_id is "greater"
        return doc_id < other.doc_id;
    }
    
    bool operator<(const ScoredDocument& other) const {
        if (score != other.score) {
            return score < other.score;
        }
        return doc_id > other.doc_id;
    }
};

/**
 * Bounded Priority Queue (Min-Heap) for efficient Top-K retrieval
 * 
 * Maintains at most K elements with highest scores.
 * Uses min-heap so lowest score is always at top for easy eviction.
 * 
 * Time Complexity:
 * - push: O(log K)
 * - getSorted: O(K log K)
 * 
 * Space Complexity: O(K)
 * 
 * Example Usage:
 *   BoundedPriorityQueue<ScoredDocument> top_k(10);
 *   for (auto& candidate : candidates) {
 *       double score = computeScore(candidate);
 *       top_k.push({candidate.id, score});
 *   }
 *   auto results = top_k.getSorted();  // Top 10 in descending order
 */
template<typename T>
class BoundedPriorityQueue {
private:
    size_t capacity_;
    // Min-heap: smallest element at top
    std::priority_queue<T, std::vector<T>, std::greater<T>> heap_;
    
public:
    /**
     * Create bounded priority queue with capacity K
     */
    explicit BoundedPriorityQueue(size_t k) : capacity_(k) {
        // Allow capacity of 0 for edge cases
    }
    
    /**
     * Insert element into heap (O(log K))
     * Only keeps top K elements by score
     */
    void push(T item) {
        if (capacity_ == 0) {
            return;  // Can't add anything with 0 capacity
        }
        
        if (heap_.size() < capacity_) {
            // Heap not full, always insert
            heap_.push(std::move(item));
        } else if (item > heap_.top()) {
            // Item better than worst in heap, replace worst
            heap_.pop();
            heap_.push(std::move(item));
        }
        // Otherwise item is worse than all K elements, discard
    }
    
    /**
     * Extract all elements in sorted order (highest to lowest)
     * Empties the heap
     */
    std::vector<T> getSorted() {
        std::vector<T> result;
        result.reserve(heap_.size());
        
        // Extract all elements (min to max)
        while (!heap_.empty()) {
            result.push_back(heap_.top());
            heap_.pop();
        }
        
        // Reverse to get max to min (descending order)
        std::reverse(result.begin(), result.end());
        return result;
    }
    
    /**
     * Peek at all elements without removing them
     * Returns in arbitrary order
     */
    std::vector<T> peek() const {
        std::vector<T> result;
        std::priority_queue<T, std::vector<T>, std::greater<T>> temp_heap = heap_;
        
        while (!temp_heap.empty()) {
            result.push_back(temp_heap.top());
            temp_heap.pop();
        }
        
        std::reverse(result.begin(), result.end());
        return result;
    }
    
    /**
     * Get current size
     */
    size_t size() const { 
        return heap_.size(); 
    }
    
    /**
     * Check if heap is at capacity
     */
    bool isFull() const { 
        return heap_.size() >= capacity_; 
    }
    
    /**
     * Get minimum score in heap (lowest score that's still in top-K)
     * Useful for early termination: skip docs with score < minScore()
     */
    double minScore() const { 
        return heap_.empty() ? 0.0 : heap_.top().score; 
    }
    
    /**
     * Check if heap is empty
     */
    bool empty() const {
        return heap_.size() == 0;
    }
    
    /**
     * Get capacity
     */
    size_t capacity() const {
        return capacity_;
    }
    
    /**
     * Clear the heap
     */
    void clear() {
        while (!heap_.empty()) {
            heap_.pop();
        }
    }
};

} // namespace rtrv_search_engine
