#include <benchmark/benchmark.h>
#include "search_engine.hpp"
#include <fstream>
#include <vector>
#include <sstream>

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
        file.clear();
    }
    
    if (!found) {
        return docs;
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

static void BM_Search(benchmark::State& state) {
    auto docs = loadWikipediaSample();
    if (docs.empty()) {
        state.SkipWithError("No Wikipedia sample data found");
        return;
    }
    
    SearchEngine engine;
    
    // Index documents for testing
    int num_docs = state.range(0);
    for (int i = 0; i < num_docs && i < static_cast<int>(docs.size()); ++i) {
        Document doc;
        doc.id = i;
        doc.fields["title"] = docs[i % docs.size()].first;
        doc.fields["content"] = docs[i % docs.size()].second;
        engine.indexDocument(doc);
    }
    
    // Sample queries
    std::vector<std::string> queries = {
        "computer", "science", "algorithm", "data", "machine"
    };
    
    size_t query_idx = 0;
    
    for (auto _ : state) {
        auto results = engine.search(queries[query_idx % queries.size()]);
        benchmark::DoNotOptimize(results);
        query_idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_Search)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

static void BM_SearchComplexQuery(benchmark::State& state) {
    auto docs = loadWikipediaSample();
    if (docs.empty()) {
        state.SkipWithError("No Wikipedia sample data found");
        return;
    }
    
    SearchEngine engine;
    
    int num_docs = state.range(0);
    for (int i = 0; i < num_docs && i < static_cast<int>(docs.size()); ++i) {
        Document doc;
        doc.id = i;
        doc.fields["title"] = docs[i % docs.size()].first;
        doc.fields["content"] = docs[i % docs.size()].second;
        engine.indexDocument(doc);
    }
    
    // Multi-term complex queries
    std::vector<std::string> complex_queries = {
        "computer science programming",
        "artificial intelligence machine learning",
        "data structures algorithms",
        "software engineering design patterns",
        "database management systems"
    };
    
    size_t query_idx = 0;
    
    for (auto _ : state) {
        auto results = engine.search(complex_queries[query_idx % complex_queries.size()]);
        benchmark::DoNotOptimize(results);
        query_idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_SearchComplexQuery)
    ->Arg(1000)
    ->Arg(10000);

static void BM_SearchWithTfIdf(benchmark::State& state) {
    auto docs = loadWikipediaSample();
    if (docs.empty()) {
        state.SkipWithError("No Wikipedia sample data found");
        return;
    }
    
    SearchEngine engine;
    
    int num_docs = state.range(0);
    for (int i = 0; i < num_docs && i < static_cast<int>(docs.size()); ++i) {
        Document doc;
        doc.id = i;
        doc.fields["title"] = docs[i % docs.size()].first;
        doc.fields["content"] = docs[i % docs.size()].second;
        engine.indexDocument(doc);
    }
    
    SearchOptions options;
    options.algorithm = SearchOptions::TF_IDF;
    options.max_results = 10;
    
    for (auto _ : state) {
        auto results = engine.search("computer science", options);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_SearchWithTfIdf)
    ->Arg(1000)
    ->Arg(10000);

static void BM_SearchWithBm25(benchmark::State& state) {
    auto docs = loadWikipediaSample();
    if (docs.empty()) {
        state.SkipWithError("No Wikipedia sample data found");
        return;
    }
    
    SearchEngine engine;
    
    int num_docs = state.range(0);
    for (int i = 0; i < num_docs && i < static_cast<int>(docs.size()); ++i) {
        Document doc;
        doc.id = i;
        doc.fields["title"] = docs[i % docs.size()].first;
        doc.fields["content"] = docs[i % docs.size()].second;
        engine.indexDocument(doc);
    }
    
    SearchOptions options;
    options.algorithm = SearchOptions::BM25;
    options.max_results = 10;
    
    for (auto _ : state) {
        auto results = engine.search("computer science", options);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_SearchWithBm25)
    ->Arg(1000)
    ->Arg(10000);

static void BM_SearchResultSize(benchmark::State& state) {
    auto docs = loadWikipediaSample();
    if (docs.empty()) {
        state.SkipWithError("No Wikipedia sample data found");
        return;
    }
    
    SearchEngine engine;
    
    // Index all documents
    for (size_t i = 0; i < docs.size(); ++i) {
        Document doc;
        doc.id = i;
        doc.fields["title"] = docs[i].first;
        doc.fields["content"] = docs[i].second;
        engine.indexDocument(doc);
    }
    
    SearchOptions options;
    options.max_results = state.range(0);  // Variable result size
    
    for (auto _ : state) {
        auto results = engine.search("computer", options);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["result_count"] = static_cast<double>(state.range(0));
}

BENCHMARK(BM_SearchResultSize)
    ->Arg(1)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100);

BENCHMARK_MAIN();
