# C++ In-Memory Search Engine — Project Design Document

A high-performance, production-ready search engine implementation in modern C++17, featuring full-text search with TF-IDF and BM25 ranking algorithms, comprehensive testing, and REST API support.

## Table of Contents
1. [Project Overview](#project-overview)
2. [Architecture & Design](#architecture--design)
3. [Core Components](#core-components)
4. [Build System & Dependencies](#build-system--dependencies)
5. [Testing Strategy](#testing-strategy)
6. [Benchmarking & Performance](#benchmarking--performance)
7. [Demo API](#demo-api)
8. [Getting Started](#getting-started)

---

## 1. Project Overview

### Purpose
This project implements a scalable, in-memory search engine designed to demonstrate proficiency in:
- **Systems Programming**: Low-level C++ with focus on performance and memory efficiency
- **Data Structures**: Inverted indices, hash maps, and efficient text processing
- **Algorithms**: TF-IDF and BM25 ranking, tokenization, query parsing
- **Software Engineering**: Clean architecture, comprehensive testing, benchmarking
- **API Design**: RESTful interfaces for real-world integration

### Key Features
- ✅ **Full-Text Search**: Index and query documents with sub-millisecond latency
- ✅ **Multiple Ranking Algorithms**: TF-IDF and BM25 with configurable parameters
- ✅ **Query Parser**: Support for multi-term queries with boolean logic (planned)
- ✅ **Persistence**: Save and load index snapshots to/from disk
- ✅ **RESTful API**: HTTP endpoints for integration (Crow and Drogon frameworks)
- ✅ **Comprehensive Testing**: Unit tests, integration tests (Google Test)
- ✅ **Performance Benchmarks**: Detailed benchmarking suite (Google Benchmark)
- ✅ **Production Ready**: Thread-safe operations, error handling, logging

### Use Cases
- Document search for knowledge bases
- Product search in e-commerce
- Code search in IDEs
- Log analysis and filtering
- Real-time text analytics

---

## 2. Architecture & Design

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Client Applications                     │
│              (REST API, CLI, Embedded Usage)                 │
└─────────────────┬───────────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────────┐
│                     SearchEngine API                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Indexing   │  │   Searching  │  │ Persistence  │      │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
└─────────┼──────────────────┼──────────────────┼─────────────┘
          │                  │                  │
┌─────────▼──────────────────▼──────────────────▼─────────────┐
│                      Core Components                         │
│  ┌──────────┐  ┌───────────┐  ┌────────┐  ┌──────────┐    │
│  │Tokenizer │  │  Inverted │  │ Ranker │  │  Query   │    │
│  │          │  │   Index   │  │        │  │  Parser  │    │
│  └──────────┘  └───────────┘  └────────┘  └──────────┘    │
│                                                              │
│  ┌──────────┐  ┌───────────┐                               │
│  │Document  │  │Statistics │                               │
│  └──────────┘  └───────────┘                               │
└──────────────────────────────────────────────────────────────┘
```

### Design Principles

1. **Modularity**: Each component has a single, well-defined responsibility
2. **Dependency Injection**: Components are configurable and testable via dependency injection
3. **RAII**: Automatic resource management through smart pointers
4. **Zero-Copy Where Possible**: Minimize data copying for performance
5. **Clear Interfaces**: Simple, intuitive APIs for ease of use
6. **Performance First**: Optimized data structures and algorithms

### Data Flow

#### Indexing Flow
```
Document → Tokenizer → Terms → InvertedIndex → Statistics Update
                                      ↓
                              Store Document Metadata
```

#### Search Flow
```
Query String → QueryParser → Terms → InvertedIndex Lookup → Candidate Docs
                                                                    ↓
                                              Ranker (TF-IDF/BM25) → Sorted Results
```

---

## 3. Core Components

### 3.1 Document (`document.hpp/cpp`)

**Purpose**: Represents a searchable document with metadata.

**Structure**:
```cpp
struct Document {
    uint64_t id;                                             // Unique ID
    std::string content;                                     // Full text
    std::unordered_map<std::string, std::string> metadata;   // Key-value metadata
    size_t term_count;                                       // Cached for BM25
};
```

**Features**:
- Supports arbitrary metadata (author, date, category, etc.)
- Automatic term count calculation for ranking
- Efficient copy/move semantics

### 3.2 Tokenizer (`tokenizer.hpp/cpp`)

**Purpose**: Converts raw text into normalized, searchable terms.

**Features**:
- **Lowercasing**: Case-insensitive matching
- **Punctuation Removal**: Clean tokens
- **Stopword Filtering**: Remove common words (the, a, an, etc.)
- **Configurable**: Load custom stopword lists

**Algorithm**:
```cpp
tokenize("The Quick Brown Fox!") → ["quick", "brown", "fox"]
```

**Stopwords**: Loaded from `data/stopwords.txt` (common English words)

### 3.3 Inverted Index (`inverted_index.hpp/cpp`)

**Purpose**: Core data structure for fast term-to-document lookups.

**Structure**:
```cpp
std::unordered_map<std::string, PostingList>
// term → [(doc_id, frequency), ...]
```

**Operations**:
- `addDocument()`: O(T) where T = number of unique terms
- `removeDocument()`: O(T)
- `search()`: O(M) where M = number of matching documents
- `getDocumentFrequency()`: O(1) term lookup

**Features**:
- Term frequencies for TF-IDF calculation
- Document frequencies for IDF calculation
- Efficient updates and deletes

### 3.4 Ranker (`ranker.hpp/cpp`)

**Purpose**: Scores and ranks search results using information retrieval algorithms.

#### TF-IDF (Term Frequency-Inverse Document Frequency)

**Formula**:
```
score(d, q) = Σ(tf(t,d) × idf(t))
where:
  tf(t,d) = term frequency in document
  idf(t) = log(N / df(t))
  N = total documents
  df(t) = documents containing term t
```

#### BM25 (Best Matching 25)

**Formula**:
```
score(d, q) = Σ(idf(t) × (tf(t,d) × (k1 + 1)) / (tf(t,d) + k1 × (1 - b + b × |d|/avgdl)))
where:
  k1 = 1.2 (term frequency saturation)
  b = 0.75 (length normalization)
  |d| = document length
  avgdl = average document length
```

**Implementation**:
- Optimized for speed with cached statistics
- Configurable parameters (k1, b)
- Supports score explanation for debugging

### 3.5 Query Parser (`query_parser.hpp/cpp`)

**Purpose**: Parses search queries into structured representations.

**Current Features**:
- Multi-term query splitting
- Term normalization (via Tokenizer)
- Whitespace handling

**Planned Features**:
- Boolean operators (AND, OR, NOT)
- Phrase queries ("exact match")
- Wildcards (prefix*)
- Field-specific search (title:foo content:bar)

### 3.6 Search Engine (`search_engine.hpp/cpp`)

**Purpose**: Main API facade coordinating all components.

**Key Methods**:

```cpp
// Indexing
uint64_t indexDocument(const Document& doc);
void indexDocuments(const std::vector<Document>& docs);
bool updateDocument(uint64_t doc_id, const Document& doc);
bool deleteDocument(uint64_t doc_id);

// Searching
std::vector<SearchResult> search(const std::string& query, 
                                 const SearchOptions& options = {});

// Statistics
IndexStatistics getStats() const;

// Persistence
bool saveSnapshot(const std::string& filepath);
bool loadSnapshot(const std::string& filepath);
```

**Configuration**:
```cpp
SearchOptions options;
options.algorithm = SearchOptions::BM25;  // or TF_IDF
options.max_results = 10;
options.explain_scores = true;  // Include score breakdown
```

### 3.7 Persistence (`persistence.hpp/cpp`)

**Purpose**: Serialize/deserialize index for persistence.

**Features**:
- Binary format for efficiency
- Atomic writes (write to temp, then rename)
- Version compatibility checks
- Compressed storage (planned)

**Usage**:
```cpp
engine.saveSnapshot("index.db");
engine.loadSnapshot("index.db");
```

---

## 4. Build System & Dependencies

### Build Requirements

- **C++ Compiler**: GCC 9+, Clang 10+, or MSVC 2019+ with C++17 support
- **CMake**: 3.15 or higher
- **Operating System**: Linux, macOS, or Windows

### Dependencies

#### Core Dependencies
- **Standard Library**: C++17 STL (no external deps for core engine)
- **Threading**: `pthread` (via CMake `Threads::Threads`)

#### Testing Dependencies
- **Google Test**: Fetched automatically via CMake FetchContent
  ```cmake
  FetchContent_Declare(googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG release-1.12.1)
  ```

#### Benchmarking Dependencies
- **Google Benchmark**: Fetched automatically via CMake FetchContent
  ```cmake
  FetchContent_Declare(benchmark
      GIT_REPOSITORY https://github.com/google/benchmark.git
      GIT_TAG v1.8.3)
  ```

#### Optional API Server Dependencies
- **Crow** (lightweight): Header-only HTTP framework
  ```bash
  brew install crow  # macOS
  ```
- **Drogon** (high-performance): Full-featured async HTTP framework
  ```bash
  brew install drogon  # macOS
  ```

### Build Instructions

#### Quick Start
```bash
# Clone repository
git clone <repository-url>
cd searchDB

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build (all targets)
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Run benchmarks
./benchmarks/indexing_benchmark
./benchmarks/search_benchmark
./benchmarks/memory_benchmark
./benchmarks/concurrent_benchmark
```

#### Build Configurations

**Debug Build** (default):
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

**Release Build** (optimized):
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

**With Specific Servers**:
```bash
# Build with Crow server
cmake -DBUILD_CROW_SERVER=ON ..

# Build with Drogon server
cmake -DBUILD_DROGON_SERVER=ON ..
```

### Project Structure

```
searchDB/
├── CMakeLists.txt              # Main build configuration
├── README.md                   # This file
├── .gitignore                  # Git ignore rules
│
├── include/                    # Public headers
│   ├── document.hpp
│   ├── tokenizer.hpp
│   ├── inverted_index.hpp
│   ├── ranker.hpp
│   ├── query_parser.hpp
│   ├── search_engine.hpp
│   ├── persistence.hpp
│   └── crow.h                  # Crow framework (if used)
│
├── src/                        # Implementation files
│   ├── document.cpp
│   ├── tokenizer.cpp
│   ├── inverted_index.cpp
│   ├── ranker.cpp
│   ├── query_parser.cpp
│   ├── search_engine.cpp
│   └── persistence.cpp
│
├── tests/                      # Unit and integration tests
│   ├── CMakeLists.txt
│   ├── tokenizer_test.cpp
│   ├── inverted_index_test.cpp
│   ├── ranker_test.cpp
│   ├── query_parser_test.cpp
│   ├── search_engine_test.cpp
│   └── integration_test.cpp
│
├── benchmarks/                 # Performance benchmarks
│   ├── CMakeLists.txt
│   ├── README.md               # Benchmark documentation
│   ├── indexing_benchmark.cpp
│   ├── search_benchmark.cpp
│   ├── memory_benchmark.cpp
│   └── concurrent_benchmark.cpp
│
├── server/                     # API server implementations
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── rest_server.cpp         # Simple HTTP server
│   ├── rest_server_crow.cpp    # Crow framework
│   ├── rest_server_drogon.cpp  # Drogon framework
│   ├── rest_server_interactive.cpp  # CLI interface
│   └── web_ui/                 # Web UI (HTML/JS)
│
├── data/                       # Sample data and resources
│   ├── stopwords.txt           # English stopwords list
│   ├── wikipedia_sample.txt    # Sample documents
│   └── test_corpus.txt         # Test data
│
├── examples/                   # Usage examples
│   ├── simple_search.cpp
│   └── batch_indexing.cpp
│
├── external/                   # Third-party dependencies
│   └── googletest/             # Git submodule
│
└── build/                      # Build artifacts (gitignored)
    ├── lib/                    # Compiled libraries
    ├── bin/                    # Executables
    ├── tests/                  # Test executables
    └── benchmarks/             # Benchmark executables
```

---

## 5. Testing Strategy

### Test Framework
- **Google Test**: Industry-standard C++ testing framework
- **Coverage**: Unit tests for all core components + integration tests

### Test Organization

#### Unit Tests
Each component has dedicated test files:

1. **`tokenizer_test.cpp`**
   - Lowercasing
   - Punctuation removal
   - Stopword filtering
   - Edge cases (empty strings, special characters)

2. **`inverted_index_test.cpp`**
   - Document addition/removal
   - Term frequency tracking
   - Document frequency calculation
   - Search operations

3. **`ranker_test.cpp`**
   - TF-IDF score calculation
   - BM25 score calculation
   - Ranking order verification
   - Edge cases (empty results, single term)

4. **`query_parser_test.cpp`**
   - Multi-term parsing
   - Whitespace handling
   - Special character handling

5. **`search_engine_test.cpp`**
   - End-to-end indexing
   - Search result accuracy
   - Update/delete operations
   - Statistics tracking

#### Integration Tests (`integration_test.cpp`)
- Full workflow: index → search → rank → return
- Multiple documents and queries
- Different ranking algorithms
- Persistence (save/load)

### Running Tests

```bash
# Build tests
cd build
make

# Run all tests
ctest --output-on-failure

# Run specific test
./tests/search_engine_test

# Verbose output
./tests/search_engine_test --gtest_verbose

# Filter specific tests
./tests/search_engine_test --gtest_filter=*SearchWithMultipleTerms*
```

### Test Coverage Goals
- **Line Coverage**: >90%
- **Branch Coverage**: >85%
- **Critical Paths**: 100% (indexing, searching, ranking)

### Test Data
- Small corpus for fast tests
- Large corpus for stress tests (benchmarks)
- Wikipedia sample data in `data/wikipedia_sample.txt`

---

## 6. Benchmarking & Performance

### Benchmark Suite

Comprehensive performance testing using Google Benchmark framework.

#### Available Benchmarks

1. **`indexing_benchmark`**
   - Single document indexing latency
   - Batch indexing throughput
   - Scaling with document count

2. **`search_benchmark`**
   - Query latency (simple and complex)
   - TF-IDF vs BM25 performance
   - Result set size impact

3. **`memory_benchmark`**
   - Memory per document
   - Index size vs corpus size
   - Compression ratio

4. **`concurrent_benchmark`**
   - Parallel search throughput
   - Multi-threaded performance
   - Scalability with thread count

### Performance Metrics

#### Typical Performance (Release Build, M1 Mac)

| Operation | Latency | Throughput |
|-----------|---------|------------|
| Index single document | 3-5 µs | 200k-300k docs/sec |
| Simple search query | 3-4 µs | 250k-300k queries/sec |
| Complex search (3+ terms) | 15-20 µs | 50k-60k queries/sec |
| TF-IDF ranking | 10 µs | 100k queries/sec |
| BM25 ranking | 10 µs | 100k queries/sec |

**Memory Usage**:
- Base overhead: ~16-512 bytes per document
- Index compression ratio: 4-5x (index size / corpus size)

#### Scalability

The search engine scales well:
- **Document Count**: Sub-linear search time (logarithmic with inverted index)
- **Query Complexity**: Linear with number of query terms
- **Concurrent Searches**: Near-linear scaling up to CPU core count

### Running Benchmarks

```bash
cd build

# Run all benchmarks
./benchmarks/indexing_benchmark
./benchmarks/search_benchmark
./benchmarks/memory_benchmark
./benchmarks/concurrent_benchmark

# Export results to JSON
./benchmarks/search_benchmark --benchmark_out=results.json \
                              --benchmark_out_format=json

# Filter specific benchmarks
./benchmarks/search_benchmark --benchmark_filter=BM_Search/1000

# Repetitions for statistical accuracy
./benchmarks/search_benchmark --benchmark_repetitions=10
```

### Optimization Notes

**Current Optimizations**:
- Hash-based inverted index (O(1) lookups)
- Cached document statistics for BM25
- Minimal memory allocations in hot paths
- Efficient string handling (views, moves)

**Planned Optimizations**:
- SIMD for tokenization
- Memory pool allocators
- Index compression (delta encoding, variable-length integers)
- Query result caching
- Parallel indexing

For detailed benchmark documentation, see [`benchmarks/README.md`](benchmarks/README.md).

---

## 7. Demo API

### REST API Servers

Three server implementations are provided:

#### 1. Simple REST Server (`rest_server.cpp`)
- Lightweight, minimal dependencies
- Good for learning and demos
- Single-threaded

#### 2. Crow Server (`rest_server_crow.cpp`)
- Header-only, easy to integrate
- Multi-threaded
- Good performance

#### 3. Drogon Server (`rest_server_drogon.cpp`)
- High-performance async framework
- WebSocket support
- Production-ready

### API Endpoints

#### POST `/index` - Index a Document
```bash
curl -X POST http://localhost:8080/index \
  -H "Content-Type: application/json" \
  -d '{
    "id": 1,
    "content": "Computer science is the study of computation and information"
  }'
```

**Response**:
```json
{
  "success": true,
  "doc_id": 1,
  "message": "Document indexed successfully"
}
```

#### GET `/search?q=<query>` - Search Documents
```bash
curl "http://localhost:8080/search?q=computer%20science&max_results=5&algorithm=bm25"
```

**Response**:
```json
{
  "query": "computer science",
  "results": [
    {
      "doc_id": 1,
      "score": 3.14159,
      "content": "Computer science is the study of...",
      "explanation": "BM25: term 'computer' contributes 1.5..."
    }
  ],
  "total": 1,
  "time_ms": 0.342
}
```

#### GET `/stats` - Get Index Statistics
```bash
curl http://localhost:8080/stats
```

**Response**:
```json
{
  "total_documents": 1000,
  "total_terms": 5420,
  "avg_doc_length": 150.5,
  "index_size_mb": 2.4
}
```

#### DELETE `/document/:id` - Delete a Document
```bash
curl -X DELETE http://localhost:8080/document/1
```

**Response**:
```json
{
  "success": true,
  "message": "Document 1 deleted"
}
```

#### POST `/snapshot/save` - Save Index Snapshot
```bash
curl -X POST http://localhost:8080/snapshot/save \
  -H "Content-Type: application/json" \
  -d '{"filepath": "/tmp/index.db"}'
```

#### POST `/snapshot/load` - Load Index Snapshot
```bash
curl -X POST http://localhost:8080/snapshot/load \
  -H "Content-Type: application/json" \
  -d '{"filepath": "/tmp/index.db"}'
```

### Running the Server

```bash
# Build servers
cd build
make

# Run Crow server (recommended)
./server/rest_server_crow --port 8080

# Run Drogon server
./server/rest_server_drogon

# Run interactive CLI
./server/interactive_server
```

### Web UI

A simple web interface is provided in `server/web_ui/`:

```bash
# Serve static files with the REST server
./server/rest_server_crow --port 8080 --web-ui ./server/web_ui

# Open browser
open http://localhost:8080
```

**Features**:
- Document indexing form
- Live search with results
- Index statistics dashboard
- Ranking algorithm selection

---

## 8. Getting Started

### Quick Start Example

```cpp
#include "search_engine.hpp"
using namespace search_engine;

int main() {
    // Create search engine
    SearchEngine engine;
    
    // Index documents
    Document doc1;
    doc1.content = "The quick brown fox jumps over the lazy dog";
    engine.indexDocument(doc1);
    
    Document doc2;
    doc2.content = "A lazy cat sleeps in the sun";
    engine.indexDocument(doc2);
    
    // Search
    auto results = engine.search("lazy");
    
    // Print results
    for (const auto& result : results) {
        std::cout << "Score: " << result.score 
                  << " - " << result.document.content << "\n";
    }
    
    return 0;
}
```

### Advanced Usage

```cpp
// Configure search options
SearchOptions options;
options.algorithm = SearchOptions::BM25;
options.max_results = 20;
options.explain_scores = true;

// Search with options
auto results = engine.search("computer science algorithms", options);

// Check explanations
for (const auto& result : results) {
    std::cout << "Doc " << result.document.id 
              << ": " << result.score << "\n";
    std::cout << result.explanation << "\n\n";
}

// Save index for later
engine.saveSnapshot("index.db");

// Load index
SearchEngine engine2;
engine2.loadSnapshot("index.db");
```

### Example Programs

See the `examples/` directory for complete examples:

1. **`simple_search.cpp`**: Basic usage
2. **`batch_indexing.cpp`**: Indexing large corpora
3. **CLI tool**: `server/rest_server_interactive.cpp`

---

## References & Resources

### Information Retrieval
- [Introduction to Information Retrieval](https://nlp.stanford.edu/IR-book/) - Manning, Raghavan, Schütze
- [Modern Information Retrieval](http://www.mir2ed.org/) - Baeza-Yates, Ribeiro-Neto
- [BM25 Paper](https://en.wikipedia.org/wiki/Okapi_BM25) - Robertson, Zaragoza

### C++ Resources
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Effective Modern C++](https://www.oreilly.com/library/view/effective-modern-c/9781491908419/) - Scott Meyers

### Libraries Used
- [Google Test](https://github.com/google/googletest)
- [Google Benchmark](https://github.com/google/benchmark)
- [Crow](https://github.com/CrowCpp/Crow)
- [Drogon](https://github.com/drogonframework/drogon)

---


