#include <benchmark/benchmark.h>
#include "search_engine.hpp"

using namespace search_engine;

static void BM_Search(benchmark::State& state) {
    SearchEngine engine;
    
    // TODO: Index documents for testing
    // for (int i = 0; i < state.range(0); ++i) {
    //     Document doc(i, "document " + std::to_string(i));
    //     engine.indexDocument(doc);
    // }
    
    for (auto _ : state) {
        // TODO: Benchmark search latency
        // auto results = engine.search("document");
        // benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_Search)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

static void BM_SearchComplexQuery(benchmark::State& state) {
    // TODO: Benchmark multi-term queries
}

BENCHMARK(BM_SearchComplexQuery)->Arg(10000);

static void BM_SearchWithTfIdf(benchmark::State& state) {
    // TODO: Benchmark TF-IDF vs BM25
}

static void BM_SearchWithBm25(benchmark::State& state) {
    // TODO: Benchmark BM25 specifically
}

BENCHMARK_MAIN();
