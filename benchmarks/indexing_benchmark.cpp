#include <benchmark/benchmark.h>
#include "search_engine.hpp"

using namespace search_engine;

static void BM_IndexDocument(benchmark::State& state) {
    SearchEngine engine;
    
    for (auto _ : state) {
        // TODO: Benchmark document indexing
        // Document doc(state.range(0), "sample document content");
        // engine.indexDocument(doc);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_IndexDocument)->Range(1, 1<<10);

static void BM_BatchIndexing(benchmark::State& state) {
    // TODO: Benchmark batch indexing throughput
    // Measure documents/second
}

BENCHMARK(BM_BatchIndexing)->Arg(100)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();
