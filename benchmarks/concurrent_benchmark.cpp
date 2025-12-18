#include <benchmark/benchmark.h>
#include "search_engine.hpp"
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include <atomic>

using namespace search_engine;

// Helper function to load Wikipedia sample data
std::vector<std::pair<std::string, std::string>> loadWikipediaSample() {
    std::vector<std::pair<std::string, std::string>> docs;
    
    // Try multiple paths to find the data file
    std::vector<std::string> paths = {
        "data/wikipedia_sample.txt",
        "../data/wikipedia_sample.txt",
        "../../data/wikipedia_sample.txt"
    };
    
    std::ifstream file;
    bool found = false;
    for (const auto& path : paths) {
        file.open(path);
        if (file.is_open()) {
            found = true;
            break;
        }
        file.clear(); // Clear error flags before trying next path
    }
    
    if (!found) {
        return docs; // Return empty if file not found
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            auto pos = line.find('|');
            if (pos != std::string::npos) {
                std::string title = line.substr(0, pos);
                std::string content = line.substr(pos + 1);
                docs.emplace_back(title, content);
            }
        }
    }
    
    return docs;
}

static void BM_ConcurrentSearches(benchmark::State& state) {
    auto docs = loadWikipediaSample();
    if (docs.empty()) {
        state.SkipWithError("No Wikipedia sample data found");
        return;
    }
    
    SearchEngine engine;
    
    // Index documents
    for (const auto& [title, content] : docs) {
        Document doc;
        doc.content = title + " " + content;
        engine.indexDocument(doc);
    }
    
    // Sample queries to use
    std::vector<std::string> queries = {
        "computer science",
        "artificial intelligence",
        "machine learning",
        "database systems",
        "programming language"
    };
    
    int num_threads = state.range(0);
    int64_t total_queries = 0;
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<int> queries_completed{0};
        
        // Launch multiple threads performing searches (read-only, should be safe)
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i]() {
                auto results = engine.search(queries[i % queries.size()]);
                queries_completed.fetch_add(1, std::memory_order_relaxed);
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        total_queries += queries_completed.load();
    }
    
    state.SetItemsProcessed(total_queries);
}

BENCHMARK(BM_ConcurrentSearches)
    ->Arg(1)   // Single thread
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16);

static void BM_ConcurrentUpdates(benchmark::State& state) {
    auto docs = loadWikipediaSample();
    if (docs.empty()) {
        state.SkipWithError("No Wikipedia sample data found");
        return;
    }
    
    int num_threads = state.range(0);
    int64_t total_operations = 0;
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<int> operations{0};
        
        // Each thread gets its own SearchEngine to avoid race conditions
        // This benchmarks the overhead of parallel independent indexing operations
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i]() {
                SearchEngine engine;
                
                // Each thread indexes a subset of documents
                for (size_t j = i; j < docs.size(); j += num_threads) {
                    Document doc;
                    doc.id = j;
                    doc.content = docs[j].first + " " + docs[j].second;
                    engine.indexDocument(doc);
                    operations.fetch_add(1, std::memory_order_relaxed);
                }
                
                // Then perform some searches
                for (int k = 0; k < 10; ++k) {
                    auto results = engine.search("computer");
                    operations.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        total_operations += operations.load();
    }
    
    state.SetItemsProcessed(total_operations);
}

BENCHMARK(BM_ConcurrentUpdates)
    ->Arg(2)
    ->Arg(4);

BENCHMARK_MAIN();
