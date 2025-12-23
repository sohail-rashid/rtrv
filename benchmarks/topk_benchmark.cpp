#include <benchmark/benchmark.h>
#include "search_engine.hpp"
#include "top_k_heap.hpp"
#include <random>
#include <algorithm>
#include <fstream>

using namespace search_engine;

// Helper to generate random documents
std::vector<std::pair<std::string, std::string>> generateRandomDocuments(size_t count) {
    std::vector<std::pair<std::string, std::string>> docs;
    std::vector<std::string> words = {
        "machine", "learning", "algorithm", "data", "science",
        "computer", "programming", "software", "engineering", "artificial",
        "intelligence", "neural", "network", "deep", "model"
    };
    
    std::mt19937 gen(42);
    std::uniform_int_distribution<> word_dist(0, words.size() - 1);
    std::uniform_int_distribution<> count_dist(50, 200);
    
    for (size_t i = 0; i < count; ++i) {
        std::string content;
        int num_words = count_dist(gen);
        for (int j = 0; j < num_words; ++j) {
            content += words[word_dist(gen)] + " ";
        }
        docs.emplace_back("Doc " + std::to_string(i), content);
    }
    
    return docs;
}

// Benchmark: Top-K heap vs traditional full sort
static void BM_TopK_Heap_vs_Sort(benchmark::State& state) {
    size_t k = state.range(0);
    size_t total_docs = state.range(1);
    bool use_heap = state.range(2) != 0;
    
    // Generate documents with varying scores
    std::mt19937 gen(42);
    std::uniform_real_distribution<> score_dist(0.0, 100.0);
    
    std::vector<SearchResult> results;
    results.reserve(total_docs);
    
    Document dummy_doc;
    dummy_doc.id = 0;
    dummy_doc.fields["content"] = "test";
    
    for (size_t i = 0; i < total_docs; ++i) {
        SearchResult result;
        result.document = dummy_doc;
        result.document.id = i;
        result.score = score_dist(gen);
        results.push_back(result);
    }
    
    for (auto _ : state) {
        if (use_heap) {
            // Use Top-K heap
            BoundedPriorityQueue<SearchResult> heap(k);
            for (const auto& result : results) {
                if (!heap.isFull() || result.score > heap.minScore()) {
                    heap.push(result);
                }
            }
            auto top_k = heap.getSorted();
            benchmark::DoNotOptimize(top_k);
        } else {
            // Traditional full sort
            auto sorted = results;
            std::sort(sorted.begin(), sorted.end(),
                     [](const SearchResult& a, const SearchResult& b) {
                         return a.score > b.score;
                     });
            sorted.resize(std::min(k, sorted.size()));
            benchmark::DoNotOptimize(sorted);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * total_docs);
    state.SetLabel(use_heap ? "Heap" : "Sort");
}

// Benchmark different K values with heap
BENCHMARK(BM_TopK_Heap_vs_Sort)
    ->Args({10, 10000, 1})    // K=10, N=10K, Heap
    ->Args({10, 10000, 0})    // K=10, N=10K, Sort
    ->Args({100, 10000, 1})   // K=100, N=10K, Heap
    ->Args({100, 10000, 0})   // K=100, N=10K, Sort
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.1);           // Run for minimum 0.1s

// Benchmark: Top-K heap with different K values
static void BM_TopK_VaryingK(benchmark::State& state) {
    size_t k = state.range(0);
    size_t total_docs = 10000;  // Reduced from 100K
    
    auto docs = generateRandomDocuments(total_docs);
    SearchEngine engine;
    
    // Index documents
    for (size_t i = 0; i < docs.size(); ++i) {
        Document doc;
        doc.id = i;
        doc.fields["title"] = docs[i].first;
        doc.fields["content"] = docs[i].second;
        engine.indexDocument(doc);
    }
    
    SearchOptions options;
    options.max_results = k;
    options.use_top_k_heap = true;
    
    for (auto _ : state) {
        auto results = engine.search("machine learning", options);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["K"] = static_cast<double>(k);
}

BENCHMARK(BM_TopK_VaryingK)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.1);  // Quick run

// Benchmark: Heap efficiency with early termination
static void BM_TopK_EarlyTermination(benchmark::State& state) {
    size_t k = 10;
    size_t total_candidates = state.range(0);
    
    // Generate results with decreasing scores (best-case scenario)
    std::vector<SearchResult> results;
    results.reserve(total_candidates);
    
    Document dummy_doc;
    dummy_doc.id = 0;
    dummy_doc.fields["content"] = "test";
    
    for (size_t i = 0; i < total_candidates; ++i) {
        SearchResult result;
        result.document = dummy_doc;
        result.document.id = i;
        result.score = static_cast<double>(total_candidates - i);
        results.push_back(result);
    }
    
    for (auto _ : state) {
        BoundedPriorityQueue<SearchResult> heap(k);
        size_t early_exits = 0;
        
        for (const auto& result : results) {
            if (heap.isFull() && result.score <= heap.minScore()) {
                early_exits++;
                continue; // Early termination
            }
            heap.push(result);
        }
        
        auto top_k = heap.getSorted();
        benchmark::DoNotOptimize(top_k);
        benchmark::DoNotOptimize(early_exits);
    }
    
    state.SetItemsProcessed(state.iterations() * total_candidates);
}

BENCHMARK(BM_TopK_EarlyTermination)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(50000)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.1);  // Quick run

// Benchmark: Search with different algorithms (BM25 vs TF-IDF) using Top-K heap
static void BM_TopK_RankerComparison(benchmark::State& state) {
    size_t num_docs = 5000;  // Reduced for speed
    bool use_bm25 = state.range(0) != 0;
    
    auto docs = generateRandomDocuments(num_docs);
    SearchEngine engine;
    
    for (size_t i = 0; i < docs.size(); ++i) {
        Document doc;
        doc.id = i;
        doc.fields["title"] = docs[i].first;
        doc.fields["content"] = docs[i].second;
        engine.indexDocument(doc);
    }
    
    SearchOptions options;
    options.max_results = 10;
    options.use_top_k_heap = true;
    options.algorithm = use_bm25 ? SearchOptions::BM25 : SearchOptions::TF_IDF;
    
    for (auto _ : state) {
        auto results = engine.search("machine learning algorithm", options);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel(use_bm25 ? "BM25" : "TF-IDF");
}

BENCHMARK(BM_TopK_RankerComparison)
    ->Arg(0)  // TF-IDF
    ->Arg(1)  // BM25
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.1);

// Benchmark: Query complexity with Top-K heap
static void BM_TopK_QueryComplexity(benchmark::State& state) {
    size_t num_terms = state.range(0);
    size_t num_docs = 5000;  // Reduced for speed
    
    auto docs = generateRandomDocuments(num_docs);
    SearchEngine engine;
    
    for (size_t i = 0; i < docs.size(); ++i) {
        Document doc;
        doc.id = i;
        doc.fields["content"] = docs[i].second;
        engine.indexDocument(doc);
    }
    
    // Build query with variable number of terms
    std::vector<std::string> terms = {
        "machine", "learning", "algorithm", "data", "science",
        "artificial", "intelligence", "neural", "network", "deep"
    };
    
    std::string query;
    for (size_t i = 0; i < num_terms && i < terms.size(); ++i) {
        if (i > 0) query += " ";
        query += terms[i];
    }
    
    SearchOptions options;
    options.max_results = 10;
    options.use_top_k_heap = true;
    
    for (auto _ : state) {
        auto results = engine.search(query, options);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["query_terms"] = static_cast<double>(num_terms);
}

BENCHMARK(BM_TopK_QueryComplexity)
    ->Arg(1)
    ->Arg(3)
    ->Arg(5)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.1);  // Quick run

// Benchmark: Memory efficiency of Top-K heap
static void BM_TopK_MemoryEfficiency(benchmark::State& state) {
    size_t k = state.range(0);
    size_t total_candidates = 10000;  // Reduced for speed
    
    std::mt19937 gen(42);
    std::uniform_real_distribution<> score_dist(0.0, 100.0);
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Generate results on the fly
        BoundedPriorityQueue<SearchResult> heap(k);
        
        state.ResumeTiming();
        
        for (size_t i = 0; i < total_candidates; ++i) {
            SearchResult result;
            result.document.id = i;
            result.score = score_dist(gen);
            
            if (!heap.isFull() || result.score > heap.minScore()) {
                heap.push(result);
            }
        }
        
        auto top_k = heap.getSorted();
        benchmark::DoNotOptimize(top_k);
    }
    
    state.SetItemsProcessed(state.iterations() * total_candidates);
    state.SetLabel("K=" + std::to_string(k));
}

BENCHMARK(BM_TopK_MemoryEfficiency)
    ->Arg(10)
    ->Arg(100)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.1);  // Quick run

BENCHMARK_MAIN();
