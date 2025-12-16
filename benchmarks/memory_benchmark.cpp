#include <benchmark/benchmark.h>
#include "search_engine.hpp"

using namespace search_engine;

static void BM_MemoryPerDocument(benchmark::State& state) {
    // TODO: Measure memory overhead per document
    // Use system APIs to measure process memory
    // Index documents and track memory growth
    
    for (auto _ : state) {
        SearchEngine engine;
        
        // Index documents
        // Measure memory before and after
        // Calculate bytes per document
    }
}

BENCHMARK(BM_MemoryPerDocument)->Arg(1000)->Arg(10000);

static void BM_IndexSize(benchmark::State& state) {
    // TODO: Measure inverted index size vs corpus size
}

BENCHMARK(BM_IndexSize);

BENCHMARK_MAIN();
