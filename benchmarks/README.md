# Search Engine Benchmarks

This directory contains performance benchmarks for the search engine using [Google Benchmark](https://github.com/google/benchmark).

## Overview

The benchmark suite measures various aspects of the search engine's performance:

- **Indexing Performance**: Document ingestion speed and batch indexing throughput
- **Search Performance**: Query latency and throughput with different configurations
- **Memory Usage**: Memory overhead per document and index size vs corpus size
- **Concurrency**: Multi-threaded search performance

## Building and Running

### Build All Benchmarks

```bash
cd build
cmake ..
make
```

### Run Individual Benchmarks

```bash
# Indexing benchmarks
./benchmarks/indexing_benchmark

# Search benchmarks
./benchmarks/search_benchmark

# Memory benchmarks
./benchmarks/memory_benchmark

# Concurrent benchmarks
./benchmarks/concurrent_benchmark
```

### Run All Benchmarks

```bash
cd build/benchmarks
./indexing_benchmark && ./search_benchmark && ./memory_benchmark && ./concurrent_benchmark
```

## Benchmark Files

### 1. indexing_benchmark.cpp

Measures document indexing performance.

**Benchmarks:**
- `BM_IndexDocument`: Single document indexing latency (1, 8, 64, 512, 1024 docs)
- `BM_BatchIndexing`: Batch indexing throughput (100, 1000, 10000 docs)

**Example Output:**
```
BM_IndexDocument/1        3981 ns     3980 ns    183517 items_per_second=251.249k/s
BM_BatchIndexing/1000   136856 ns   136847 ns      5012 items_per_second=7.30744M/s
```

**Metrics:**
- `Time/CPU`: Average time per benchmark iteration
- `items_per_second`: Documents indexed per second

### 2. search_benchmark.cpp

Measures search query performance with different configurations.

**Benchmarks:**
- `BM_Search`: Basic single-term search with varying corpus sizes (100, 1000, 10000 docs)
- `BM_SearchComplexQuery`: Multi-term query performance (1000, 10000 docs)
- `BM_SearchWithTfIdf`: TF-IDF ranking algorithm performance
- `BM_SearchWithBm25`: BM25 ranking algorithm performance
- `BM_SearchResultSize`: Impact of result set size (1, 10, 50, 100 results)

**Example Output:**
```
BM_Search/1000                   3501 ns    3500 ns    199374 items_per_second=285.674k/s
BM_SearchComplexQuery/1000      18064 ns   18063 ns     38335 items_per_second=55.3632k/s
BM_SearchWithTfIdf/1000         10354 ns   10352 ns     66514 items_per_second=96.6029k/s
BM_SearchWithBm25/1000          10360 ns   10359 ns     68325 items_per_second=96.5321k/s
BM_SearchResultSize/10           5899 ns    5899 ns    120393 items_per_second=169.526k/s
```

**Metrics:**
- `items_per_second`: Queries processed per second
- `result_count`: Number of results returned per query

**Insights:**
- Single-term queries are ~5x faster than multi-term queries
- TF-IDF and BM25 have similar performance characteristics
- Result set size has minimal impact on query latency

### 3. memory_benchmark.cpp

Measures memory consumption and efficiency.

**Benchmarks:**
- `BM_MemoryPerDocument`: Memory overhead per indexed document (100, 1000, 10000 docs)
- `BM_IndexSize`: Inverted index size compared to corpus size

**Example Output:**
```
BM_MemoryPerDocument/100    171466 ns  bytes_per_doc=163.84 total_memory_kb=16
BM_IndexSize                174607 ns  corpus_size_kb=9.79688 index_size_kb=48 compression_ratio=4.9
```

**Metrics:**
- `bytes_per_doc`: Average memory used per document
- `total_memory_kb`: Total memory consumed by the index
- `compression_ratio`: Index size / corpus size (lower is better)

**Note:** Memory measurements use platform-specific APIs (mach on macOS, /proc on Linux) and are most accurate in Release builds.

### 4. concurrent_benchmark.cpp

Measures multi-threaded performance.

**Benchmarks:**
- `BM_ConcurrentSearches`: Parallel search query throughput (1, 2, 4, 8, 16 threads)
- `BM_ConcurrentUpdates`: Concurrent indexing operations (2, 4 threads)

**Example Output:**
```
BM_ConcurrentSearches/1      13223 ns    6503 ns   102528 items_per_second=153.765k/s
BM_ConcurrentSearches/4      31612 ns   27960 ns    24281 items_per_second=143.061k/s
BM_ConcurrentSearches/16    115731 ns  113978 ns     6116 items_per_second=140.378k/s
```

**Metrics:**
- `Time`: Wall clock time (affected by thread contention)
- `CPU`: Total CPU time across all threads
- `items_per_second`: Aggregate throughput

**Insights:**
- Search operations can be performed concurrently (read-only)
- Update operations require synchronization (SearchEngine is not thread-safe for writes)
- Optimal thread count depends on workload and CPU cores

## Data Files

Benchmarks use sample data from `data/wikipedia_sample.txt`. The file format is:
```
id|document content
1|Computer science is the study of computation...
2|Artificial intelligence is intelligence demonstrated by machines...
```

The benchmark automatically searches for the data file in multiple locations:
- `data/wikipedia_sample.txt` (workspace root)
- `../data/wikipedia_sample.txt` (one level up)
- `../../data/wikipedia_sample.txt` (two levels up)

## Interpreting Results

### Understanding Timing

- **ns (nanoseconds)**: 1 second = 1,000,000,000 nanoseconds
- **µs (microseconds)**: 1 second = 1,000,000 microseconds
- **ms (milliseconds)**: 1 second = 1,000 milliseconds

### Common Patterns

1. **Logarithmic Scaling**: Search performance typically doesn't degrade linearly with corpus size due to inverted index efficiency.

2. **CPU Affinity**: Multi-threaded benchmarks show diminishing returns beyond physical core count.

3. **Debug vs Release**: DEBUG builds (with `-g -O0`) can be 10-100x slower than Release builds (`-O3`).

### Optimization Tips

- Build in Release mode for accurate performance measurements:
  ```bash
  cmake -DCMAKE_BUILD_TYPE=Release ..
  ```

- Use `--benchmark_filter` to run specific benchmarks:
  ```bash
  ./search_benchmark --benchmark_filter=BM_Search/1000
  ```

- Export results to JSON for analysis:
  ```bash
  ./search_benchmark --benchmark_out=results.json --benchmark_out_format=json
  ```

## Performance Baselines

Typical performance on modern hardware (Release build):

| Operation | Latency | Throughput |
|-----------|---------|------------|
| Single document index | ~1-5 µs | 200k-1M docs/sec |
| Simple search query | ~1-10 µs | 100k-1M queries/sec |
| Complex search query | ~5-50 µs | 20k-200k queries/sec |
| Memory per document | 16-512 bytes | - |

**Note:** Actual performance varies based on:
- Document size and complexity
- Query complexity
- Hardware specifications (CPU, memory, cache)
- Build configuration (Debug vs Release)
- Operating system and compiler optimizations

## Troubleshooting

### Benchmark Errors

**"No Wikipedia sample data found"**
- Ensure `data/wikipedia_sample.txt` exists in the workspace root
- Check file permissions and path

**Segmentation faults in concurrent benchmarks**
- The SearchEngine is not thread-safe for concurrent writes
- Ensure only read operations (searches) are performed concurrently
- Use separate engine instances for concurrent indexing

### Performance Issues

**Slow benchmarks**
- Verify you're running Release builds
- Check for background processes consuming CPU
- Disable CPU frequency scaling for consistent results

**Inconsistent results**
- Run benchmarks multiple times and average results
- Close unnecessary applications
- Use `--benchmark_repetitions=N` for statistical accuracy

## Contributing

When adding new benchmarks:

1. Follow the existing naming convention: `BM_FeatureName`
2. Use `benchmark::DoNotOptimize()` to prevent compiler optimization
3. Set appropriate metrics with `state.counters[]`
4. Include error checking for data loading
5. Document the benchmark purpose and metrics in this README

## Additional Resources

- [Google Benchmark Documentation](https://github.com/google/benchmark)
- [Benchmark User Guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
- [Performance Profiling Best Practices](https://easyperf.net/blog/)
