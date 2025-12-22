/**
 * Skip Pointer Demonstration
 * 
 * Shows how skip pointers accelerate AND queries in the inverted index
 */

#include "inverted_index.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace search_engine;
using namespace std::chrono;

void printPostingList(const PostingList& list, const std::string& term) {
    std::cout << "Term: \"" << term << "\" (" << list.postings.size() << " documents)\n";
    std::cout << "  Postings: [";
    for (size_t i = 0; i < std::min(size_t(10), list.postings.size()); ++i) {
        std::cout << list.postings[i].doc_id;
        if (i < std::min(size_t(10), list.postings.size()) - 1) std::cout << ", ";
    }
    if (list.postings.size() > 10) std::cout << ", ...";
    std::cout << "]\n";
    
    std::cout << "  Skip Pointers (" << list.skip_pointers.size() << "): [";
    for (size_t i = 0; i < std::min(size_t(5), list.skip_pointers.size()); ++i) {
        std::cout << "{pos:" << list.skip_pointers[i].position 
                  << ",doc:" << list.skip_pointers[i].doc_id << "}";
        if (i < std::min(size_t(5), list.skip_pointers.size()) - 1) std::cout << ", ";
    }
    if (list.skip_pointers.size() > 5) std::cout << ", ...";
    std::cout << "]\n\n";
}

// Naive intersection without skip pointers
std::vector<uint64_t> naiveIntersect(
    const std::vector<Posting>& list1,
    const std::vector<Posting>& list2
) {
    std::vector<uint64_t> result;
    size_t i = 0, j = 0;
    
    while (i < list1.size() && j < list2.size()) {
        if (list1[i].doc_id == list2[j].doc_id) {
            result.push_back(list1[i].doc_id);
            ++i; ++j;
        } else if (list1[i].doc_id < list2[j].doc_id) {
            ++i;
        } else {
            ++j;
        }
    }
    
    return result;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  Skip Pointer Demonstration\n";
    std::cout << "========================================\n\n";
    
    InvertedIndex index;
    
    // Simulate indexing documents with two terms
    // "search" appears in: 10, 20, 30, ..., 10000 (1000 docs, every 10th)
    // "engine" appears in: 100, 200, 300, ..., 10000 (100 docs, every 100th)
    
    std::cout << "1. Building inverted index...\n";
    std::cout << "   Adding \"search\" to documents: 10, 20, 30, ..., 10000 (1000 docs)\n";
    for (uint64_t doc_id = 10; doc_id <= 10000; doc_id += 10) {
        index.addTerm("search", doc_id, 0);
    }
    
    std::cout << "   Adding \"engine\" to documents: 100, 200, 300, ..., 10000 (100 docs)\n\n";
    for (uint64_t doc_id = 100; doc_id <= 10000; doc_id += 100) {
        index.addTerm("engine", doc_id, 0);
    }
    
    // Get posting lists with skip pointers
    std::cout << "2. Building skip pointers...\n";
    PostingList search_list = index.getPostingList("search");
    PostingList engine_list = index.getPostingList("engine");
    
    printPostingList(search_list, "search");
    printPostingList(engine_list, "engine");
    
    // Show skip pointer intervals
    if (!search_list.skip_pointers.empty()) {
        size_t interval = search_list.skip_pointers.size() > 1 ? 
            search_list.skip_pointers[1].position - search_list.skip_pointers[0].position : 0;
        std::cout << "Skip interval for \"search\": ~" << interval << " postings\n";
        std::cout << "Skip interval formula: sqrt(" << search_list.postings.size() 
                  << ") ≈ " << static_cast<int>(std::sqrt(search_list.postings.size())) << "\n\n";
    }
    
    // Perform AND query with and without skip pointers
    std::cout << "3. Query: \"search AND engine\"\n\n";
    
    // Without skip pointers (naive)
    auto start = high_resolution_clock::now();
    auto result_naive = naiveIntersect(search_list.postings, engine_list.postings);
    auto end = high_resolution_clock::now();
    auto duration_naive = duration_cast<microseconds>(end - start);
    
    // With skip pointers
    start = high_resolution_clock::now();
    auto result_skips = intersectWithSkips(search_list, engine_list);
    end = high_resolution_clock::now();
    auto duration_skips = duration_cast<microseconds>(end - start);
    
    std::cout << "Results:\n";
    std::cout << "  Documents matching both terms: " << result_naive.size() << "\n";
    std::cout << "  Sample results: [";
    for (size_t i = 0; i < std::min(size_t(5), result_naive.size()); ++i) {
        std::cout << result_naive[i];
        if (i < std::min(size_t(5), result_naive.size()) - 1) std::cout << ", ";
    }
    if (result_naive.size() > 5) std::cout << ", ...";
    std::cout << "]\n\n";
    
    std::cout << "Performance:\n";
    std::cout << "  Naive intersection:    " << std::setw(6) << duration_naive.count() << " μs\n";
    std::cout << "  With skip pointers:    " << std::setw(6) << duration_skips.count() << " μs\n";
    
    if (duration_naive.count() > 0) {
        double speedup = static_cast<double>(duration_naive.count()) / duration_skips.count();
        std::cout << "  Speedup:               " << std::fixed << std::setprecision(2) 
                  << speedup << "x\n";
    }
    std::cout << "\n";
    
    // Demonstrate skip pointer lookup
    std::cout << "4. Skip pointer lookup demonstration:\n";
    uint64_t target_doc = 500;
    size_t skip_pos = search_list.findSkipTarget(target_doc);
    
    std::cout << "  Looking for doc_id >= " << target_doc << " in \"search\" list\n";
    std::cout << "  Skip pointer suggests starting at position: " << skip_pos << "\n";
    std::cout << "  Document at that position: " << search_list.postings[skip_pos].doc_id << "\n";
    std::cout << "  (Skipped " << skip_pos << " postings instead of scanning from 0)\n\n";
    
    // Rebuild skip pointers with different intervals
    std::cout << "5. Testing different skip intervals:\n";
    
    PostingList custom_list = search_list;
    
    // Small interval (more skips, more memory, faster)
    custom_list.buildSkipPointers(16);
    std::cout << "  Interval=16:   " << custom_list.skip_pointers.size() << " skip pointers\n";
    
    // Medium interval (default sqrt)
    custom_list.buildSkipPointers(0);
    std::cout << "  Interval=sqrt: " << custom_list.skip_pointers.size() << " skip pointers\n";
    
    // Large interval (fewer skips, less memory, slower)
    custom_list.buildSkipPointers(256);
    std::cout << "  Interval=256:  " << custom_list.skip_pointers.size() << " skip pointers\n\n";
    
    std::cout << "6. Memory overhead:\n";
    size_t posting_size = sizeof(Posting) * search_list.postings.size();
    size_t skip_size = sizeof(SkipPointer) * search_list.skip_pointers.size();
    double overhead_pct = (static_cast<double>(skip_size) / posting_size) * 100.0;
    
    std::cout << "  Posting list size:  " << posting_size << " bytes\n";
    std::cout << "  Skip pointers size: " << skip_size << " bytes\n";
    std::cout << "  Memory overhead:    " << std::fixed << std::setprecision(1) 
              << overhead_pct << "%\n\n";
    
    std::cout << "========================================\n";
    std::cout << "  Demonstration Complete!\n";
    std::cout << "========================================\n";
    
    return 0;
}
