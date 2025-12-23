# Search Engine Benchmark Suite

Comprehensive performance testing suite using Google Benchmark for production-grade performance analysis.

## Overview

This benchmark suite provides statistical rigor and reproducibility for testing various aspects of the search engine:

- **Search Performance**: Query latency, throughput, and scalability
- **Top-K Retrieval**: Heap vs. traditional sorting performance
- **Indexing**: Document indexing throughput
- **Concurrency**: Multi-threaded search and indexing
- **Memory**: Memory usage and efficiency
- **Tokenization**: SIMD vs. scalar tokenization performance

## Building Benchmarks

```bash
cd build
cmake ..
make -j$(nproc)
```

Individual benchmark executables:
- `benchmarks/search_benchmark` - Search performance tests
- `benchmarks/topk_benchmark` - Top-K heap optimization tests  
- `benchmarks/indexing_benchmark` - Document indexing performance
- `benchmarks/concurrent_benchmark` - Concurrency and thread safety
- `benchmarks/memory_benchmark` - Memory usage analysis
- `benchmarks/tokenizer_simd_benchmark` - Tokenization performance

## Running Benchmarks

### Run All Benchmarks
```bash
cd build/benchmarks
./search_benchmark
./topk_benchmark
./indexing_benchmark
./concurrent_benchmark
./memory_benchmark
```

### Run Specific Benchmarks
```bash
# Run only Top-K heap tests
./topk_benchmark --benchmark_filter="BM_TopK"

# Run with specific iterations
./search_benchmark --benchmark_min_time=2.0s

# Output to JSON for analysis
./search_benchmark --benchmark_out=results.json --benchmark_out_format=json

# Run with specific thread counts
./concurrent_benchmark --benchmark_filter="BM_ConcurrentSearches/8"
```

### Common Options
- `--benchmark_filter=<regex>` - Run only matching benchmarks
- `--benchmark_min_time=<seconds>` - Minimum time per benchmark
- `--benchmark_repetitions=<N>` - Number of repetitions for statistical analysis
- `--benchmark_report_aggregates_only=true` - Show only aggregate statistics
- `--benchmark_out=<file>` - Output results to file
- `--benchmark_out_format=<json|csv|console>` - Output format
- `--benchmark_list_tests=true` - List all available benchmarks

## Benchmark Details

### 1. Top-K Heap Benchmarks (`topk_benchmark`)

Tests the bounded priority queue optimization for Top-K retrieval.

**Key Benchmarks:**
- `BM_TopK_Heap_vs_Sort` - Direct comparison between heap (O(N log K)) and sort (O(N log N))
- `BM_TopK_VaryingK` - Performance with different K values (1 to 1000)
- `BM_TopK_EarlyTermination` - Tests early exit optimization
- `BM_TopK_RankerComparison` - BM25 vs. TF-IDF with Top-K heap
- `BM_TopK_QueryComplexity` - Multi-term query performance
- `BM_TopK_MemoryEfficiency` - Memory usage comparison

**Example Results:**
```
Benchmark                                   Time             CPU
-----------------------------------------------------------------
BM_TopK_Heap_vs_Sort/10/100000/1          79 us         79 us     Heap (8.5x faster)
BM_TopK_Heap_vs_Sort/10/100000/0         670 us        670 us     Sort
BM_TopK_VaryingK/10                       45 us         45 us     K=10
BM_TopK_VaryingK/100                      89 us         89 us     K=100
```

**Key Insights:**
- Heap is 5-15x faster than full sort for K << N
- Optimal for K < 1000 with N > 10,000
- Memory footprint: O(K) vs O(N) for traditional sort
- Early termination provides additional 2-3x speedup in sorted scenarios

### 2. Search Benchmarks (`search_benchmark`)

Core search performance tests with varying document counts and query complexity.

**Key Benchmarks:**
- `BM_Search` - Basic search with 100, 1000, 10000 documents
- `BM_SearchComplexQuery` - Multi-term queries
- `BM_SearchWithTfIdf` - TF-IDF ranking algorithm
- `BM_SearchWithBm25` - BM25 ranking algorithm
- `BM_SearchResultSize` - Varying result set sizes (1, 10, 50, 100)

**Performance Characteristics:**
- Linear scaling with document count for simple queries
- BM25 typically 10-20% faster than TF-IDF
- Query complexity impact: ~50-100µs per additional term

### 3. Indexing Benchmarks (`indexing_benchmark`)

Document indexing throughput and scalability tests.

**Key Metrics:**
- Documents indexed per second
- Memory growth per document
- Index build time for large datasets

### 4. Concurrent Benchmarks (`concurrent_benchmark`)

Multi-threaded performance and scalability.

**Key Benchmarks:**
- `BM_ConcurrentSearches` - Read-only concurrent searches (1, 2, 4, 8, 16 threads)
- `BM_ConcurrentUpdates` - Mixed read/write workloads

**Expected Behavior:**
- Near-linear scaling for read-only workloads
- Graceful degradation under write contention

### 5. Memory Benchmarks (`memory_benchmark`)

Memory usage and efficiency analysis.

**Key Benchmarks:**
- `BM_MemoryPerDocument` - Memory overhead per indexed document
- Memory growth patterns

**Typical Results:**
- Base index overhead: ~100-200 bytes per document
- Inverted index: ~50-100 bytes per unique term
- Depends on document size and vocabulary

### 6. Tokenization Benchmarks (`tokenizer_simd_benchmark`)

SIMD optimization effectiveness for text tokenization.

**Expected Speedup:**
- SIMD: 2-4x faster than scalar implementation
- Varies by text characteristics and CPU architecture

## Performance Targets

### Latency Targets (Production)
| Operation | Target | Notes |
|-----------|--------|-------|
| Simple search (10K docs) | < 1ms | Single term |
| Complex search (10K docs) | < 5ms | 3-5 terms |
| Simple search (100K docs) | < 10ms | Single term |
| Complex search (100K docs) | < 50ms | 3-5 terms |
| Document indexing | > 10K docs/sec | Batch operation |
| Top-K retrieval (K=10, N=100K) | < 100µs | With heap |

### Throughput Targets
| Workload | Target | Configuration |
|----------|--------|---------------|
| Concurrent searches | > 10K QPS | 8 threads, read-only |
| Mixed read/write | > 5K QPS | 80/20 read/write ratio |
| Batch indexing | > 50K docs/sec | Single thread |

## Analyzing Results

### Statistical Significance
Run with repetitions for statistical analysis:
```bash
./search_benchmark --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
```

Output includes:
- Mean time
- Standard deviation
- Min/Max values
- Coefficient of variation

### Comparing Implementations
```bash
# Before optimization
./topk_benchmark --benchmark_out=before.json --benchmark_out_format=json

# After optimization  
./topk_benchmark --benchmark_out=after.json --benchmark_out_format=json

# Compare results
python3 compare_results.py before.json after.json
```

### Profiling
For detailed profiling, use system tools:
```bash
# macOS (Instruments)
instruments -t "Time Profiler" ./search_benchmark

# Linux (perf)
perf record -g ./search_benchmark
perf report

# Valgrind (memory profiling)
valgrind --tool=massif ./memory_benchmark
```

## Continuous Integration

Integrate benchmarks into CI pipeline:

```yaml
# .github/workflows/benchmark.yml
name: Benchmarks
on: [push, pull_request]
jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build
        run: |
          mkdir build && cd build
          cmake -DCMAKE_BUILD_TYPE=Release ..
          make -j$(nproc)
      - name: Run Benchmarks
        run: |
          cd build/benchmarks
          ./search_benchmark --benchmark_out=results.json
      - name: Store Results
        uses: actions/upload-artifact@v2
        with:
          name: benchmark-results
          path: build/benchmarks/results.json
```

## Optimization Guidelines

### Based on Benchmark Results

1. **Use Top-K Heap when:**
   - K < 1000
   - N > 10,000
   - Memory is constrained
   - Result: 5-15x speedup vs. full sort

2. **Choose BM25 for:**
   - Modern ranking requirements
   - Slightly faster than TF-IDF
   - Better relevance in practice

3. **Enable SIMD when:**
   - Large documents (> 1KB)
   - CPU supports AVX2/NEON
   - Result: 2-4x tokenization speedup

4. **Concurrent Searches:**
   - Read-only: Scale to 8-16 threads
   - Mixed workload: Limit to 4-8 threads
   - Use read-write locks for best performance

## Troubleshooting

### High Variance
If benchmarks show high variance:
- Close background applications
- Run with `--benchmark_min_time=2.0s` for more iterations
- Check CPU throttling (`sysctl -n machdep.cpu` on macOS)
- Disable Turbo Boost for consistency

### Unexpected Performance
- Verify build configuration: `CMAKE_BUILD_TYPE=Release`
- Check compiler optimizations: `-O3` enabled
- Profile with system tools to find hotspots
- Compare against baseline results

### Memory Issues
- Use smaller datasets for memory benchmarks
- Monitor with system tools (`top`, `htop`)
- Check for memory leaks with Valgrind
- Adjust test parameters if OOM occurs

## References

- [Google Benchmark Documentation](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
- [Benchmark Design Patterns](https://github.com/google/benchmark/blob/main/docs/perf_counters.md)
- Search Engine Implementation: `../local_docs/search-engine-design-complete.md`

## Contributing

When adding new benchmarks:
1. Follow naming convention: `BM_<Component>_<TestCase>`
2. Document expected results and targets
3. Add to this guide under appropriate section
4. Test with multiple dataset sizes
5. Include statistical analysis with `--benchmark_repetitions=10`

For questions or issues, see project README or open an issue.
