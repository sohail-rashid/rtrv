#include <benchmark/benchmark.h>
#include "search_engine.hpp"
#include <vector>
#include <random>

using namespace rtrv_search_engine;

// Helper function to generate synthetic documents
std::vector<std::pair<std::string, std::string>> generateSyntheticDocuments(size_t count) {
    std::vector<std::pair<std::string, std::string>> docs;
    
    std::vector<std::string> words = {
        "computer", "science", "algorithm", "data", "machine",
        "learning", "artificial", "intelligence", "programming", "software",
        "engineering", "database", "network", "system", "design",
        "pattern", "architecture", "development", "technology", "analysis",
        "structure", "management", "application", "research", "method"
    };
    
    std::vector<std::string> topics = {
        "Computer Science", "Machine Learning", "Data Structures",
        "Algorithms", "Software Engineering", "Artificial Intelligence",
        "Database Systems", "Network Programming", "System Design"
    };
    
    std::mt19937 gen(42);
    std::uniform_int_distribution<> word_dist(0, words.size() - 1);
    std::uniform_int_distribution<> count_dist(50, 200);
    std::uniform_int_distribution<> topic_dist(0, topics.size() - 1);
    
    docs.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        std::string title = topics[topic_dist(gen)] + " " + std::to_string(i);
        
        std::string content;
        int num_words = count_dist(gen);
        for (int j = 0; j < num_words; ++j) {
            content += words[word_dist(gen)] + " ";
        }
        
        docs.emplace_back(title, content);
    }
    
    return docs;
}

static void BM_Search(benchmark::State& state) {
    int num_docs = state.range(0);
    auto docs = generateSyntheticDocuments(num_docs);
    
    SearchEngine engine;
    
    // Index documents for testing
    for (int i = 0; i < num_docs; ++i) {
        Document doc;
        doc.id = i;
        doc.fields["title"] = docs[i].first;
        doc.fields["content"] = docs[i].second;
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
    ->Arg(5000)
    ->MinTime(0.1);

static void BM_SearchComplexQuery(benchmark::State& state) {
    int num_docs = state.range(0);
    auto docs = generateSyntheticDocuments(num_docs);
    
    SearchEngine engine;
    
    // Index documents
    for (int i = 0; i < num_docs; ++i) {
        Document doc;
        doc.id = i;
        doc.fields["title"] = docs[i].first;
        doc.fields["content"] = docs[i].second;
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
    ->Arg(5000)
    ->MinTime(0.1);

static void BM_SearchWithTfIdf(benchmark::State& state) {
    int num_docs = state.range(0);
    auto docs = generateSyntheticDocuments(num_docs);
    
    SearchEngine engine;
    
    // Index documents
    for (int i = 0; i < num_docs; ++i) {
        Document doc;
        doc.id = i;
        doc.fields["title"] = docs[i].first;
        doc.fields["content"] = docs[i].second;
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
    ->Arg(5000)
    ->MinTime(0.1);

static void BM_SearchWithBm25(benchmark::State& state) {
    int num_docs = state.range(0);
    auto docs = generateSyntheticDocuments(num_docs);
    
    SearchEngine engine;
    
    // Index documents
    for (int i = 0; i < num_docs; ++i) {
        Document doc;
        doc.id = i;
        doc.fields["title"] = docs[i].first;
        doc.fields["content"] = docs[i].second;
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
    ->Arg(5000)
    ->MinTime(0.1);

static void BM_SearchResultSize(benchmark::State& state) {
    size_t num_docs = 1000;  // Fixed dataset size
    auto docs = generateSyntheticDocuments(num_docs);
    
    SearchEngine engine;
    
    // Index all documents
    for (size_t i = 0; i < num_docs; ++i) {
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
    ->MinTime(0.1);

BENCHMARK_MAIN();
