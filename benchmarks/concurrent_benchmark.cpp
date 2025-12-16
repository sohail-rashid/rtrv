#include <benchmark/benchmark.h>
#include "search_engine.hpp"
#include <thread>
#include <vector>

using namespace search_engine;

static void BM_ConcurrentSearches(benchmark::State& state) {
    SearchEngine engine;
    
    // TODO: Index documents
    
    for (auto _ : state) {
        // TODO: Benchmark concurrent search throughput
        // Launch multiple threads performing searches
        // Measure queries per second
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

BENCHMARK(BM_ConcurrentSearches)
    ->Arg(1)   // Single thread
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16);

static void BM_ConcurrentUpdates(benchmark::State& state) {
    // TODO: Benchmark concurrent indexing and searching
}

BENCHMARK(BM_ConcurrentUpdates)->Arg(4);

BENCHMARK_MAIN();
