# C++ In-Memory Search Engine

A production-quality, in-memory search engine built in modern C++ (C++17) demonstrating data structures, algorithms, and software engineering best practices.

## Features

- Full-text search with ranked results
- Multiple ranking algorithms (TF-IDF, BM25)
- Thread-safe concurrent operations
- Query parsing with boolean operators (AND, OR, NOT)
- Phrase search support
- Document updates and deletions
- Persistence (snapshot save/load)
- REST-like API for demos
- Comprehensive benchmarks

## Building

```bash
# Clone repository
git clone <repository-url>
cd searchDB

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Run benchmarks
./benchmarks/search_benchmark

# Run demo server
./server/rest_server
```

## Quick Example

```cpp
#include "search_engine.hpp"

int main() {
    SearchEngine engine;
    
    // Index documents
    engine.indexDocument({1, "The quick brown fox"});
    engine.indexDocument({2, "The lazy dog"});
    engine.indexDocument({3, "A quick dog"});
    
    // Search
    auto results = engine.search("quick dog");
    
    // Print results
    for (const auto& result : results) {
        std::cout << "Doc " << result.document.id 
                  << " (score: " << result.score << "): "
                  << result.document.content << std::endl;
    }
    
    return 0;
}
```

## Architecture

The search engine consists of the following core components:

- **Document Store**: Manages document storage and metadata
- **Tokenizer**: Converts text to searchable terms
- **Inverted Index**: Maps terms to documents with posting lists
- **Ranker**: Scores documents using TF-IDF or BM25
- **Query Parser**: Parses queries with boolean operators
- **Search Engine**: Orchestrates all components

## Performance Targets

- Search latency: <10ms for 100K documents (P99)
- Indexing throughput: >10K documents/second
- Memory overhead: <100 bytes per document
- Thread-safe: Support concurrent queries and updates

## License

MIT License
