# Rtrv — Blazing Fast Enterprise In-Memory Search Engine

**A high-performance, production-ready search engine implementation in modern C++17** featuring full-text search with advanced ranking algorithms, SIMD-accelerated tokenization, skip pointers for fast conjunctive queries, fuzzy search with Damerau-Levenshtein distance, LRU query caching, snippet extraction with highlighting, Top-K heap optimization, comprehensive testing, and a modern REST API with glassmorphism web interface.

## Table of Contents
1. [Project Overview](#project-overview)
2. [Architecture & Design](#architecture--design)
3. [Core Components](#core-components)
4. [Build System & Dependencies](#build-system--dependencies)
5. [Testing Strategy](#testing-strategy)
6. [Benchmarking & Performance](#benchmarking--performance)
7. [REST API](#rest-api)
8. [Getting Started](#getting-started)

---

## 1. Project Overview

### Purpose
This project implements a scalable, in-memory search engine designed to demonstrate proficiency in:
- **Systems Programming**: Low-level C++ with focus on performance and memory efficiency
- **Data Structures**: Inverted indices, hash maps, n-gram indices, LRU caches, and efficient text processing
- **Algorithms**: TF-IDF, BM25, and ML-based ranking; Damerau-Levenshtein fuzzy matching; SIMD tokenization
- **Software Engineering**: Clean architecture, comprehensive testing, benchmarking
- **API Design**: RESTful interfaces with full CRUD, caching, and web UI

### Key Features
- ✅ **Full-Text Search**: Index and query documents with sub-millisecond latency
- ✅ **Advanced Ranking Algorithms**: 
  - TF-IDF, BM25, and ML-Ranker with configurable parameters
  - Plugin architecture via `Ranker` base class and `RankerRegistry`
  - Top-K heap optimization for efficient result retrieval
- ✅ **High-Performance Tokenization**:
  - SIMD-accelerated processing (AVX2, SSE4.2, ARM NEON with scalar fallback)
  - 2-4x faster than standard tokenization
  - Position tracking for phrase queries and highlighting
  - Custom stopword filtering and stemming (Porter, Simple)
- ✅ **Fuzzy Search**:
  - Damerau-Levenshtein distance with early termination
  - Bigram n-gram index for fast candidate filtering
  - Automatic edit distance selection based on term length
  - Prefix matching fallback
- ✅ **Snippet Extraction & Highlighting**:
  - Sliding-window best-snippet selection
  - Configurable snippet count, length, and highlight tags
  - Word-boundary snapping with ellipsis indicators
- ✅ **Query Caching**:
  - LRU eviction with configurable TTL (default: 60s)
  - Thread-safe with atomic hit/miss/eviction counters
  - Per-request cache bypass option
- ✅ **Advanced Query Parser**: 
  - AST-based query parsing with boolean operators (AND, OR, NOT)
  - Phrase queries, proximity queries (`"term1 term2"~N`), field-specific search
  - Parenthesized expressions and operator precedence
- ✅ **Optimized Inverted Index**:
  - Skip pointers for fast conjunctive query processing
  - Position tracking for phrase and proximity queries
  - Thread-safe with `std::shared_mutex`
- ✅ **Persistence**: Save and load index snapshots in binary format with magic-number validation
- ✅ **RESTful API**: 
  - Drogon-based async HTTP server (4 threads, CORS enabled)
  - Interactive CLI server with REPL
  - Full CRUD: search, index, delete, browse, cache management, skip pointer control
- ✅ **Modern Web Interface**:
  - Glassmorphism UI with backdrop blur and mesh-gradient background
  - Search View with real-time results, fuzzy matching, and snippet highlighting
  - Index View with live stats, cache monitoring, document management
  - Sidebar view switcher, responsive layout, Inter font
- ✅ **Comprehensive Testing**: 
  - Unit tests covering all 11 components (GoogleTest)
  - Integration tests with real-world scenarios
- ✅ **Extensive Benchmarking**: 
  - 6 benchmark suites with detailed metrics (Google Benchmark)
  - Tokenizer SIMD performance comparisons
  - Top-K heap vs full sorting analysis
  - Memory profiling and concurrency tests
  - Automated benchmark runner with JSON reports
- ✅ **Production Ready**: Thread-safe operations, error handling, comprehensive logging

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
│              (REST API, CLI, Web UI, Embedded)               │
└─────────────────┬───────────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────────┐
│                     SearchEngine Facade                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Indexing    │  │   Searching  │  │ Persistence  │      │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
└─────────┼──────────────────┼──────────────────┼─────────────┘
          │                  │                  │
┌─────────▼──────────────────▼──────────────────▼─────────────┐
│                      Core Components                         │
│  ┌──────────┐  ┌───────────┐  ┌────────────┐  ┌──────────┐ │
│  │Tokenizer │  │ Inverted  │  │  Ranker    │  │  Query   │ │
│  │ +SIMD    │  │  Index    │  │  Registry  │  │  Parser  │ │
│  │ +Stem    │  │ +Skip Ptr │  │  +Plugins  │  │  +AST    │ │
│  └──────────┘  └───────────┘  └────────────┘  └──────────┘ │
│                                                              │
│  ┌──────────┐  ┌───────────┐  ┌────────┐  ┌─────────────┐  │
│  │  Fuzzy   │  │ Snippet   │  │ Top-K  │  │   Query     │  │
│  │  Search  │  │ Extractor │  │  Heap  │  │   Cache     │  │
│  └──────────┘  └───────────┘  └────────┘  └─────────────┘  │
│                                                              │
│  ┌──────────┐  ┌───────────┐  ┌──────────────┐             │
│  │Document  │  │ Document  │  │ Persistence  │             │
│  │          │  │  Loader   │  │  (Binary)    │             │
│  └──────────┘  └───────────┘  └──────────────┘             │
└──────────────────────────────────────────────────────────────┘
```

### Design Principles

1. **Modularity**: Each component has a single, well-defined responsibility
2. **Plugin Architecture**: Rankers are extensible via abstract `Ranker` base class + `RankerRegistry`
3. **RAII**: Automatic resource management through `std::unique_ptr` smart pointers
4. **Thread Safety**: `std::shared_mutex` on `InvertedIndex`, `SearchEngine`, and `QueryCache`
5. **Zero-Copy Where Possible**: Minimize data copying for performance
6. **Clear Interfaces**: Simple, intuitive APIs for ease of use

### Data Flow

#### Indexing Flow
```
Document → Tokenizer → Terms + Positions → InvertedIndex → Statistics Update
                                                  ↓
                                          Store in documents_ map
                                                  ↓
                                    Incrementally update FuzzySearch n-gram index
```

#### Search Flow
```
Query String → QueryCache (check hit)
                   ↓ (miss)
              QueryParser → Terms
                   ↓
         FuzzySearch (if enabled) → Expanded Terms
                   ↓
         InvertedIndex Lookup → Candidate Doc IDs
                   ↓
         Ranker (TF-IDF / BM25 / ML / Custom) 
                   ↓
         Top-K Heap or Full Sort → Ranked Results
                   ↓
         SnippetExtractor (if enabled) → Highlighted Snippets
                   ↓
         QueryCache (store) → Return Results
```

---

## 3. Core Components

### 3.1 Document (`document.hpp/cpp`)

**Purpose**: Represents a searchable document with field-based storage.

**Structure**:
```cpp
struct Document {
    uint32_t id;                                                   // Unique ID (4B docs)
    std::unordered_map<std::string, std::string> fields;           // Field-based storage
    size_t term_count;                                             // Cached for BM25
};
```

**Key Methods**:
```cpp
Document(uint32_t id, const std::unordered_map<std::string, std::string>& fields);
std::string getField(const std::string& field_name) const;   // Get a specific field
std::string getAllText() const;                                // Concatenate all fields
```

**Features**:
- Field-based storage (title, content, category, etc.) instead of a single `content` string
- `getAllText()` concatenates all field values for full-text indexing
- `getField()` for field-specific access (used by ML-Ranker for title boosting)
- Efficient copy/move semantics

### 3.2 Tokenizer (`tokenizer.hpp/cpp`)

**Purpose**: Converts raw text into normalized, searchable terms with SIMD acceleration.

**Enums**:
```cpp
enum class StemmerType { NONE, PORTER, SIMPLE };
```

**Token struct** (for position-aware tokenization):
```cpp
struct Token {
    std::string text;
    uint32_t position;       // Word position in document
    uint32_t start_offset;   // Character offset start (highlighting)
    uint32_t end_offset;     // Character offset end (highlighting)
};
```

**Features**:
- **Lowercasing**: Case-insensitive matching with SIMD optimization
- **Punctuation Removal**: Clean tokens using vectorized operations
- **Stopword Filtering**: Remove common words with hash-based lookup (loaded from `data/stopwords.txt`)
- **Stemming**: Porter stemmer and simple suffix-stripping stemmer
- **SIMD Acceleration**:
  - AVX2 support (256-bit vectors) — 2-3x faster
  - SSE4.2 support (128-bit vectors)
  - ARM NEON support (128-bit vectors, used on Apple Silicon)
  - Automatic compile-time detection with scalar fallback
- **Position Tracking**: Track term positions for phrase queries and highlighting

**SIMD-Accelerated Operations**:
| Function | AVX2 (32B/iter) | SSE4.2 (16B/iter) | ARM NEON (16B/iter) | Scalar |
|----------|------------------|--------------------|---------------------|--------|
| `normalizeSIMD` (lowercase) | ✅ | ✅ | ✅ | ✅ |
| `classifyCharactersSIMD` (char types) | ✅ | ✅ | ✅ | ✅ |
| `equalsSIMD` (string compare) | ✅ | ✅ | ✅ | ✅ |

**Usage**:
```cpp
Tokenizer tokenizer;
tokenizer.enableSIMD(true);
tokenizer.setStemmer(StemmerType::PORTER);

// Basic tokenization
auto terms = tokenizer.tokenize("The Quick Brown Fox!");
// Returns: ["quick", "brown", "fox"]

// Position-aware tokenization (for phrase queries and highlighting)
auto tokens = tokenizer.tokenizeWithPositions("quick brown fox");
// Returns: [{text:"quick", pos:0, start:0, end:5}, ...]
```

**Configuration Methods**:
```cpp
void setLowercase(bool enabled);
void setStopWords(const std::unordered_set<std::string>& stops);
void setRemoveStopwords(bool enabled);
void setStemmer(StemmerType type);
void enableSIMD(bool enabled);
static bool detectSIMDSupport();
```

### 3.3 Inverted Index (`inverted_index.hpp/cpp`)

**Purpose**: Core data structure for fast term-to-document lookups with skip pointer optimization.

**Structure**:
```cpp
struct Posting {
    uint64_t doc_id;
    uint32_t term_frequency;
    std::vector<uint32_t> positions;   // Term positions for phrase queries
};

struct SkipPointer {
    size_t position;     // Position in posting list
    uint64_t doc_id;     // Document ID at this position
};

class PostingList {
    std::vector<Posting> postings;
    mutable std::vector<SkipPointer> skip_pointers;
    mutable bool skips_dirty_;
};
```

**Operations**:
- `addTerm(term, doc_id, position)`: O(1) amortized — adds a posting with position tracking
- `removeDocument(doc_id)`: O(T) where T = number of terms
- `getPostings(term)`: O(M) where M = number of matching documents
- `getDocumentFrequency(term)`: O(1) term lookup
- `rebuildSkipPointers()`: Rebuild all or per-term skip pointers

**Skip Pointer Features**:
- Configurable skip interval (default: √N where N = posting list length)
- `buildSkipPointers()` / `findSkipTarget()` for accelerated list intersection
- `markSkipsDirty()` / `needsSkipRebuild()` for lazy rebuild tracking
- Minimal memory overhead (~1-2% increase)

**Free Function**:
```cpp
std::vector<uint64_t> intersectWithSkips(const PostingList& list1, const PostingList& list2);
```

**Thread Safety**: All operations are guarded by `mutable std::shared_mutex`.

### 3.4 Ranker (`ranker.hpp/cpp`)

**Purpose**: Scores and ranks search results using information retrieval algorithms with an extensible plugin architecture.

#### Plugin Architecture

All rankers inherit from the abstract `Ranker` base class:

```cpp
class Ranker {
public:
    virtual ~Ranker() = default;
    virtual double score(const Query& query, const Document& doc, 
                         const IndexStats& stats) = 0;
    virtual std::string getName() const = 0;
    virtual std::vector<double> scoreBatch(const Query& query,
                                           const std::vector<Document>& docs,
                                           const IndexStats& stats);
};
```

`RankerRegistry` manages registered rankers:
```cpp
class RankerRegistry {
    void registerRanker(std::unique_ptr<Ranker> ranker);
    Ranker* getRanker(const std::string& name);
    bool setDefaultRanker(const std::string& name);
    std::vector<std::string> listRankers() const;
    bool hasRanker(const std::string& name) const;
};
```

#### Built-In Rankers

**1. TF-IDF** (`"TF-IDF"`)
```
score(d, q) = Σ( log(1 + tf(t,d)) × log(N / df(t)) )
```

**2. BM25** (`"BM25"`) — **Default**
```
score(d, q) = Σ( idf(t) × (tf(t,d) × (k1 + 1)) / (tf(t,d) + k1 × (1 - b + b × |d|/avgdl)) )
where:
  idf(t) = log((N - df(t) + 0.5) / (df(t) + 0.5) + 1)
  k1 = 1.5 (default, configurable)
  b = 0.75 (default, configurable)
```

**3. ML-Ranker** (`"ML-Ranker"`)

A linear weighted combination of 5 features:
| Feature | Weight | Description |
|---------|--------|-------------|
| BM25 score | 0.40 | Standard BM25 relevance |
| TF-IDF score | 0.20 | TF-IDF relevance |
| Query term coverage | 0.20 | Fraction of query terms in document |
| Document length ratio | 0.05 | Normalized document length |
| Title match bonus | 0.15 | Bonus if query terms appear in `title` field |

**Custom Ranker Example**:
```cpp
class TitleBoostRanker : public Ranker {
public:
    double score(const Query& query, const Document& doc,
                 const IndexStats& stats) override {
        // Custom scoring logic
    }
    std::string getName() const override { return "TitleBoost"; }
};

engine.registerCustomRanker(std::make_unique<TitleBoostRanker>());
engine.setDefaultRanker("TitleBoost");
```

### 3.5 Query Parser (`query_parser.hpp/cpp`)

**Purpose**: Parses search queries into Abstract Syntax Tree (AST) representations.

**AST Node Types**:
```cpp
enum Type { TERM, PHRASE, FIELD, AND, OR, NOT, PROXIMITY };
```

| Node Class | Members |
|------------|---------|
| `TermNode` | `std::string term` |
| `PhraseNode` | `std::vector<std::string> terms`, `int max_distance` (0 = exact) |
| `FieldNode` | `std::string field_name`, `std::unique_ptr<QueryNode> query` |
| `AndNode` | `std::vector<std::unique_ptr<QueryNode>> children` |
| `OrNode` | `std::vector<std::unique_ptr<QueryNode>> children` |
| `NotNode` | `std::unique_ptr<QueryNode> child` |

**Query Examples**:
```cpp
// Simple multi-term (implicit AND)
"computer science" → AND(Term("computer"), Term("science"))

// Boolean operators
"cat AND dog" → AND(Term("cat"), Term("dog"))
"cat OR dog"  → OR(Term("cat"), Term("dog"))
"cat NOT dog" → AND(Term("cat"), NOT(Term("dog")))

// Phrase query
"\"machine learning\"" → Phrase(["machine", "learning"], distance=0)

// Proximity query
"\"machine learning\"~5" → Phrase(["machine", "learning"], distance=5)

// Field-specific
"title:python content:tutorial" 
  → AND(Field("title", "python"), Field("content", "tutorial"))

// Complex expression
"(neural OR deep) AND learning NOT shallow" 
  → AND(OR(Term("neural"), Term("deep")), 
        Term("learning"), 
        NOT(Term("shallow")))
```

**Implementation**: Recursive descent parser with lexer. Operator precedence: NOT > AND > OR.

### 3.6 Fuzzy Search (`fuzzy_search.hpp/cpp`)

**Purpose**: Approximate string matching for typo-tolerant search using n-gram indexing and Damerau-Levenshtein distance.

**Algorithm**:
1. **N-gram Index**: Bigram index (n=2) with `^`/`$` padding for candidate filtering
2. **Edit Distance**: Full Damerau-Levenshtein (insertions, deletions, substitutions, transpositions) with early termination
3. **Auto Distance**: Term length ≤2 → 0, 3–4 → 1, ≥5 → 2

**Key Methods**:
```cpp
void buildNgramIndex(const std::unordered_set<std::string>& vocabulary);
void addTerm(const std::string& term);       // Incremental update
void removeTerm(const std::string& term);     // Incremental removal
std::vector<FuzzyMatch> findMatches(const std::string& term, 
                                     uint32_t max_edit_distance = 0,
                                     size_t max_candidates = 10) const;
static uint32_t damerauLevenshteinDistance(const std::string& s1, 
                                           const std::string& s2,
                                           uint32_t max_distance = 255);
```

**Integration with Search**: When `SearchOptions::fuzzy_enabled = true`, the search engine:
1. Checks if each query term has exact index hits
2. If not, tries prefix matching first, then fuzzy matching
3. Applies a score penalty (−10% per expanded term, floor 0.5×)
4. Attaches expansion map (`original → corrected`) to each `SearchResult`

### 3.7 Snippet Extractor (`snippet_extractor.hpp/cpp`)

**Purpose**: Generates context-aware text snippets with highlighted query terms.

**Options**:
```cpp
struct SnippetOptions {
    size_t max_snippet_length = 150;
    size_t num_snippets = 3;
    std::string highlight_open = "<mark>";
    std::string highlight_close = "</mark>";
};
```

**Key Methods**:
```cpp
std::vector<std::string> generateSnippets(const std::string& text,
                                           const std::vector<std::string>& query_terms,
                                           const SnippetOptions& options = {}) const;
std::string highlightTerms(const std::string& text,
                            const std::vector<std::string>& query_terms,
                            const std::string& open_tag = "<mark>",
                            const std::string& close_tag = "</mark>") const;
```

**Algorithm**: Sliding-window approach that scores windows by query term match density, selects non-overlapping best windows, snaps to word boundaries, and adds `"..."` ellipsis indicators.

### 3.8 Query Cache (`query_cache.hpp/cpp`)

**Purpose**: LRU cache with TTL for memoizing search results.

**Configuration**:
```cpp
QueryCache(size_t max_entries = 1024, 
           std::chrono::milliseconds ttl = std::chrono::seconds(60));
```

**Cache Key**: Normalized query string + hashed `SearchOptions`.

**Key Methods**:
```cpp
bool get(const QueryCacheKey& key, std::vector<SearchResult>* out_results);
void put(const QueryCacheKey& key, const std::vector<SearchResult>& results);
void clear();
void setMaxEntries(size_t max_entries);
void setTtl(std::chrono::milliseconds ttl);
CacheStatistics getStats() const;
```

**Statistics**:
```cpp
struct CacheStatistics {
    size_t hit_count, miss_count, eviction_count;
    size_t current_size, max_size;
    double hit_rate;
};
```

**Thread Safety**: `std::shared_mutex` with atomic counters for stats.

### 3.9 Top-K Heap (`top_k_heap.hpp`)

**Purpose**: Memory-efficient data structure for retrieving only the top-K highest-scoring results without full sorting.

**Template Class**: `BoundedPriorityQueue<T>` — min-heap bounded to K elements.

```cpp
template<typename T, typename Compare = std::greater<T>>
class BoundedPriorityQueue {
    explicit BoundedPriorityQueue(size_t k);
    void push(T item);                  // O(log K)
    std::vector<T> getSorted();         // O(K log K) — destructive, descending
    std::vector<T> peek() const;        // O(K log K) — non-destructive, descending
    double minScore() const;            // O(1)
    size_t size() const;                // O(1)
    bool isFull() const;
    bool empty() const;
    size_t capacity() const;
    void clear();
};
```

**Specialized Type for Search**:
```cpp
struct ScoredDocument {
    uint64_t doc_id;
    double score;
    bool operator>(const ScoredDocument& other) const;  // Higher score = greater
};
```

**Performance Comparison** (retrieving top 10 from 10,000 results):
```
Full Sort:     O(n log n) = O(10,000 × 13.3) ≈ 133,000 operations
Top-K Heap:    O(n log k) = O(10,000 × 3.3)  ≈ 33,000 operations
Speedup:       ~4x faster
```

### 3.10 Document Loader (`document_loader.hpp/cpp`)

**Purpose**: Load documents from various file formats.

**Supported Formats**:
- **JSONL** (JSON Lines): One JSON object per line with arbitrary fields
- **CSV**: With configurable column name mapping

**Key Methods**:
```cpp
std::vector<Document> loadJSONL(const std::string& filepath);
std::vector<Document> loadCSV(const std::string& filepath, 
                               const std::vector<std::string>& column_names = {});
Document createDocument(const std::unordered_map<std::string, std::string>& fields);
```

Auto-assigns incremental document IDs starting from 1. Uses `nlohmann/json` for JSON parsing.

### 3.11 Search Engine (`search_engine.hpp/cpp`)

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
std::vector<SearchResult> search(const std::string& query,
                                 const std::string& ranker_name,
                                 size_t max_results = 10);

// Browsing
std::vector<std::pair<uint64_t, Document>> getDocuments(size_t offset = 0, 
                                                         size_t limit = 10) const;

// Statistics
IndexStatistics getStats() const;
CacheStatistics getCacheStats() const;

// Cache Management
void clearCache();
void setCacheConfig(size_t max_entries, std::chrono::milliseconds ttl);

// Ranker Management
void registerCustomRanker(std::unique_ptr<Ranker> ranker);
void setDefaultRanker(const std::string& ranker_name);
std::string getDefaultRanker() const;
std::vector<std::string> listAvailableRankers() const;
bool hasRanker(const std::string& name) const;
Ranker* getRanker(const std::string& name);

// Tokenizer Shortcuts
void enableSIMD(bool enabled);
void setStemmer(StemmerType type);
void setRemoveStopwords(bool enabled);
void setTokenizer(std::unique_ptr<Tokenizer> tokenizer);

// Direct Component Access
InvertedIndex* getIndex();
const SnippetExtractor& getSnippetExtractor() const;
FuzzySearch& getFuzzySearch();

// Persistence
bool saveSnapshot(const std::string& filepath);
bool loadSnapshot(const std::string& filepath);
```

**SearchOptions**:
```cpp
struct SearchOptions {
    std::string ranker_name = "";        // Use default if empty
    size_t max_results = 10;
    bool explain_scores = false;
    bool use_top_k_heap = true;          // O(n log k) vs O(n log n)
    bool generate_snippets = false;
    SnippetOptions snippet_options;
    bool fuzzy_enabled = false;
    uint32_t max_edit_distance = 0;      // 0 = auto
    bool use_cache = true;
};
```

**SearchResult**:
```cpp
struct SearchResult {
    Document document;
    double score;
    std::string explanation;
    std::vector<std::string> snippets;
    std::unordered_map<std::string, std::string> expanded_terms;  // original → corrected
};
```

**IndexStatistics**:
```cpp
struct IndexStatistics {
    size_t total_documents;
    size_t total_terms;
    double avg_doc_length;
};
```

### 3.12 Persistence (`persistence.hpp/cpp`)

**Purpose**: Serialize/deserialize index snapshots in binary format.

**Binary Format**:
```
[SnapshotHeader]              Magic: 0x53454152 ("SEAR"), Version: 1
[next_doc_id]                 uint64_t
[Documents...]                For each: id, term_count, field count, field key-value pairs
[num_index_terms]             uint64_t
[Term + PostingList...]       For each term: string, posting count, postings with positions
```

**Usage**:
```cpp
// Static methods — friend of SearchEngine
Persistence::save(engine, "index.bin");
Persistence::load(engine, "index.bin");

// Via SearchEngine facade
engine.saveSnapshot("index.bin");
engine.loadSnapshot("index.bin");
```

**Notes**:
- Atomic writes (write to temp, then rename) — not currently implemented, writes directly
- Version compatibility checks via magic number and version field
- Does **not** persist: query cache, fuzzy search index, ranker configuration, tokenizer settings
- On load, clears existing state and reconstructs the inverted index with positions

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
- **nlohmann/json**: v3.11.3 (fetched automatically via CMake FetchContent) — used by `DocumentLoader`

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

#### Optional Server Dependencies
- **Drogon**: High-performance async HTTP framework (required for REST server)
  ```bash
  brew install drogon  # macOS
  ```

### Compiler Flags

```
-Wall -Wextra -Wpedantic -O3
```

### Build Instructions

#### Quick Start
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
ctest --output-on-failure
```

#### Build Configurations

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..        # Debug build with symbols
cmake -DCMAKE_BUILD_TYPE=Release ..      # Release build with optimizations
cmake -DCMAKE_CXX_COMPILER=clang++ ..   # Specify compiler
```

### Project Structure

```
rtrv/
├── CMakeLists.txt                  # Main build configuration
├── TECHNICAL_DESIGN.md             # This file
├── README.md                       # Project overview
├── roadMap.md                      # Development roadmap
├── build_and_run_tests.sh          # Test runner script
│
├── include/                        # Public headers (13 files)
│   ├── document.hpp                # Document model (field-based)
│   ├── document_loader.hpp         # JSONL/CSV document loading
│   ├── fuzzy_search.hpp            # Fuzzy search with n-gram index
│   ├── inverted_index.hpp          # Core inverted index + skip pointers
│   ├── persistence.hpp             # Binary snapshot save/load
│   ├── query_cache.hpp             # LRU cache with TTL
│   ├── query_parser.hpp            # AST-based query parser
│   ├── ranker.hpp                  # Ranker plugin architecture
│   ├── search_engine.hpp           # Main facade
│   ├── search_types.hpp            # Shared types (SearchOptions, SearchResult, etc.)
│   ├── snippet_extractor.hpp       # Snippet generation + highlighting
│   ├── tokenizer.hpp               # SIMD-accelerated tokenizer
│   └── top_k_heap.hpp              # Bounded priority queue
│
├── src/                            # Implementation files (11 files)
│   ├── document.cpp
│   ├── document_loader.cpp
│   ├── fuzzy_search.cpp
│   ├── inverted_index.cpp
│   ├── persistence.cpp
│   ├── query_cache.cpp
│   ├── query_parser.cpp
│   ├── ranker.cpp
│   ├── search_engine.cpp
│   ├── snippet_extractor.cpp
│   └── tokenizer.cpp
│
├── tests/                          # Unit and integration tests (11 test files)
│   ├── CMakeLists.txt
│   ├── document_loader_test.cpp
│   ├── fuzzy_search_test.cpp
│   ├── integration_test.cpp
│   ├── inverted_index_test.cpp
│   ├── query_cache_test.cpp
│   ├── query_parser_test.cpp
│   ├── ranker_test.cpp
│   ├── search_engine_test.cpp
│   ├── snippet_extractor_test.cpp
│   ├── tokenizer_test.cpp
│   └── top_k_heap_test.cpp
│
├── benchmarks/                     # Performance benchmarks (6 suites)
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── BENCHMARK_GUIDE.md
│   ├── EXAMPLE_RESULTS.md
│   ├── concurrent_benchmark.cpp
│   ├── indexing_benchmark.cpp
│   ├── memory_benchmark.cpp
│   ├── search_benchmark.cpp
│   ├── tokenizer_simd_benchmark.cpp
│   ├── topk_benchmark.cpp
│   ├── run_benchmarks.sh
│   ├── run_tokenizer_benchmark.sh
│   └── compare_benchmarks.py
│
├── launch_webui.sh                 # Web UI launcher script
│
├── server/                         # API servers + Web UI
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── rest_server_drogon.cpp      # Drogon async REST server
│   ├── rest_server_interactive.cpp # Interactive CLI REPL
│   └── ui/                         # Glassmorphism Web UI
│       ├── index.html
│       ├── style.css
│       └── app.js
│
├── examples/                       # Usage examples
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── simple_search.cpp
│   ├── batch_indexing.cpp
│   └── skip_pointer_demo.cpp
│
├── data/                           # Sample data and resources
│   ├── stopwords.txt               # English stopwords list
│   └── wikipedia_sample.json       # 50 sample documents (JSONL)
│
├── local_docs/                     # Design documentation
│   ├── search-engine-design-complete.md
│   └── skip-pointer-implementation.md
│
├── external/                       # Third-party dependencies
│   ├── README.md
│   └── googletest/                 # Git submodule
│
└── build/                          # Build artifacts (gitignored)
```

---

## 5. Testing Strategy

### Test Framework
- **Google Test**: Industry-standard C++ testing framework
- **Coverage**: Unit tests for all 11 core components + integration tests

### Test Organization

Each component has a dedicated test file:

1. **`tokenizer_test.cpp`** — Lowercasing, punctuation removal, stopword filtering, SIMD tokenization, position tracking, stemming, edge cases

2. **`inverted_index_test.cpp`** — Document addition/removal, term/document frequency tracking, skip pointer functionality, search with and without skip pointers

3. **`ranker_test.cpp`** — TF-IDF score calculation, BM25 score calculation, ML-Ranker, plugin architecture validation, custom ranker registration, ranking order verification

4. **`query_parser_test.cpp`** — AST-based parsing, boolean operators (AND, OR, NOT), phrase and proximity queries, parenthesized expressions, operator precedence, field-specific queries

5. **`search_engine_test.cpp`** — End-to-end indexing, search result accuracy, Top-K heap functionality, skip pointer integration, update/delete operations, statistics tracking, plugin ranker integration

6. **`top_k_heap_test.cpp`** — Heap insertion/ordering, Top-K extraction correctness, edge cases (k=0, k>n, duplicates)

7. **`fuzzy_search_test.cpp`** — Damerau-Levenshtein distance, n-gram index building, fuzzy match ranking, auto edit distance, incremental add/remove

8. **`snippet_extractor_test.cpp`** — Snippet generation, term highlighting, word boundary snapping, configurable options

9. **`query_cache_test.cpp`** — Cache hit/miss, TTL expiration, LRU eviction, thread safety, statistics

10. **`document_loader_test.cpp`** — JSONL loading, CSV loading, field mapping, error handling

11. **`integration_test.cpp`** — Full workflow: index → search → rank → return, multiple documents and queries, different ranking algorithms, persistence (save/load)

### Running Tests

```bash
cd build
make -j$(nproc)

# Run all tests
ctest --output-on-failure

# Run specific test
./tests/search_engine_test

# Filter specific tests
./tests/search_engine_test --gtest_filter=*SearchWithMultipleTerms*

# Verbose output
./tests/search_engine_test --gtest_verbose
```

### Test Data
- Small in-memory corpus for fast unit tests
- `data/wikipedia_sample.json`: 50 Wikipedia CS articles for integration tests

---

## 6. Benchmarking & Performance

### Benchmark Suite

Comprehensive performance testing using Google Benchmark framework.

#### Available Benchmarks (6 suites)

1. **`indexing_benchmark`** — Single document indexing latency, batch indexing throughput, scaling with document count

2. **`search_benchmark`** — Query latency (simple and complex), TF-IDF vs BM25 comparison, result set size impact, skip pointer optimization

3. **`memory_benchmark`** — Memory per document (small/medium/large), index size vs corpus size, skip pointer memory overhead

4. **`concurrent_benchmark`** — Parallel search throughput, multi-threaded performance (1–16 threads), lock contention analysis

5. **`tokenizer_simd_benchmark`** — Standard vs SIMD tokenization (AVX2/SSE4.2/ARM NEON), speedup analysis

6. **`topk_benchmark`** — Full sort vs Top-K heap, various K values and result set sizes, memory efficiency

### Typical Performance (Release Build)

| Operation | Latency | Throughput |
|-----------|---------|------------|
| Index single document | 3-5 µs | 200k-300k docs/sec |
| Simple search query | 3-4 µs | 250k-300k queries/sec |
| Complex search (3+ terms) | 15-20 µs | 50k-60k queries/sec |
| Search with skip pointers | 8-12 µs | 80k-120k queries/sec |
| BM25 ranking | ~10 µs | ~100k queries/sec |
| Top-K heap (k=10, n=10K) | 5-8 µs | 120k-200k queries/sec |
| SIMD tokenization (AVX2) | 2-3 µs/KB | 300-500 MB/s |

**Memory Usage**:
- Base overhead: ~16-64 bytes per document
- Skip pointers overhead: ~1-2% of index size
- Top-K heap: O(k) vs O(n) for full results

### Running Benchmarks

```bash
cd build

# Individual benchmarks
./benchmarks/indexing_benchmark
./benchmarks/search_benchmark
./benchmarks/topk_benchmark

# All benchmarks with automated runner
cd ../benchmarks && ./run_benchmarks.sh

# Export results
./benchmarks/search_benchmark --benchmark_out=results.json --benchmark_out_format=json

# Filter specific benchmarks
./benchmarks/search_benchmark --benchmark_filter=BM_Search/1000

# Compare two runs
cd ../benchmarks && python3 compare_benchmarks.py results1.json results2.json
```

### Implemented Optimizations

- **Hash-based inverted index** (O(1) lookups)
- **Skip pointers** for 2-5x faster conjunctive queries
- **SIMD tokenization** (AVX2, SSE4.2, ARM NEON) for 2-4x throughput
- **Top-K heap** (`BoundedPriorityQueue`) for 2-10x faster result retrieval when k ≪ n
- **LRU query cache** with TTL for repeated query acceleration
- **N-gram fuzzy index** for fast approximate matching candidates
- **Cached document statistics** for BM25
- **Plugin ranker architecture** for extensibility without rebuilds
- **Minimal memory allocations** in hot paths
- **Efficient string handling** (views, moves)
- **AST-based query parsing** with term extraction optimization

---

## 7. REST API

### Server Implementations

#### 1. Drogon REST Server (`rest_server_drogon.cpp`)
- High-performance async HTTP framework, 4-thread event loop
- Serves both REST API and static web UI files
- Auto-resolves UI root directory, auto-loads `wikipedia_sample.json`
- Full CORS support (all origins, GET/POST/DELETE/OPTIONS)
- Request/response logging with emoji indicators

#### 2. Interactive CLI Server (`rest_server_interactive.cpp`)
- Command-line REPL with command aliases
- Fuzzy command suggestions for typos
- Useful for development, debugging, and manual testing

### Endpoint Summary

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/` | Serve web UI |
| `GET` | `/search?q=...` | Full-text search with ranking |
| `GET` | `/documents?offset=&limit=` | Browse documents (paginated) |
| `GET` | `/stats` | Index statistics |
| `GET` | `/cache/stats` | Query cache statistics |
| `DELETE` | `/cache` | Clear query cache |
| `POST` | `/index` | Add a document |
| `DELETE` | `/delete/{id}` | Remove a document |
| `POST` | `/save` | Save index snapshot |
| `POST` | `/load` | Load index snapshot |
| `POST` | `/skip/rebuild` | Rebuild all skip pointers |
| `POST` | `/skip/rebuild/{term}` | Rebuild skip pointers for one term |
| `GET` | `/skip/stats?term=` | Skip pointer statistics |

### Search Endpoint

```http
GET /search?q=<query>
```

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `q` | Yes | — | Search query string |
| `algorithm` | No | `bm25` | Ranking algorithm: `bm25` or `tfidf` |
| `max_results` | No | `10` | Maximum results |
| `use_top_k_heap` | No | `true` | Top-K heap optimization |
| `highlight` | No | `false` | Enable snippet generation |
| `snippet_length` | No | `150` | Max snippet length (chars) |
| `num_snippets` | No | `3` | Number of snippets per result |
| `fuzzy` | No | `false` | Enable fuzzy matching |
| `max_edit_distance` | No | auto | Max edit distance for fuzzy |
| `cache` | No | `true` | Enable query cache |

**Response**:
```json
{
  "results": [
    {
      "score": 8.234,
      "document": { "id": 2, "content": "Machine learning is..." },
      "snippets": ["...branch of <mark>artificial intelligence</mark>..."],
      "expanded_terms": { "learning": "learnings" }
    }
  ],
  "total_results": 5
}
```

### Documents Endpoint

```http
GET /documents?offset=0&limit=10
```

Paginated document browsing (max limit: 1000). Returns results in the same shape as `/search` with `score: 0.0`.

### Running the Server

```bash
cd build/server
./rest_server_drogon 8080
# Opens: http://localhost:8080
```

### Web UI

A glassmorphism-styled single-page dashboard served at the root URL.

**Search View**: Real-time search, algorithm selection, fuzzy matching, snippet highlighting, empty search browses all documents.

**Index View**: Live index statistics, cache monitoring (hit rate, hits, misses, evictions), add/delete documents, clear cache, auto-refresh every 15s.

**Design**: Glassmorphism with backdrop blur, mesh-gradient background, violet accent color scheme, Inter font, sidebar view switcher, responsive layout.

For full REST API documentation, see [`server/README.md`](server/README.md).

---

## 8. Getting Started

### Quick Start Example

```cpp
#include "search_engine.hpp"
using namespace rtrv_search_engine;

int main() {
    SearchEngine engine;
    
    // Index documents using field-based storage
    Document doc1;
    doc1.fields["title"] = "Machine Learning";
    doc1.fields["content"] = "Machine learning is a branch of artificial intelligence";
    engine.indexDocument(doc1);
    
    Document doc2;
    doc2.fields["content"] = "A lazy cat sleeps in the sun";
    engine.indexDocument(doc2);
    
    // Search with default BM25 ranking
    auto results = engine.search("machine learning");
    for (const auto& result : results) {
        std::cout << "Score: " << result.score 
                  << " - " << result.document.getAllText() << "\n";
    }
    
    return 0;
}
```

### Advanced Usage

```cpp
#include "search_engine.hpp"
using namespace rtrv_search_engine;

int main() {
    SearchEngine engine;
    
    // Configure tokenizer
    engine.enableSIMD(true);
    engine.setStemmer(StemmerType::PORTER);
    engine.setRemoveStopwords(true);
    
    // Configure cache
    engine.setCacheConfig(2048, std::chrono::seconds(120));
    
    // Index documents
    Document doc;
    doc.fields["title"] = "Machine Learning";
    doc.fields["content"] = "Machine learning and artificial intelligence";
    doc.fields["category"] = "AI";
    engine.indexDocument(doc);
    
    // Search with full options
    SearchOptions options;
    options.ranker_name = "BM25";
    options.max_results = 20;
    options.use_top_k_heap = true;
    options.generate_snippets = true;
    options.snippet_options.num_snippets = 2;
    options.snippet_options.max_snippet_length = 200;
    options.fuzzy_enabled = true;
    options.max_edit_distance = 2;
    options.use_cache = true;
    
    auto results = engine.search("(machine OR deep) AND learning", options);
    
    for (const auto& result : results) {
        std::cout << "Doc " << result.document.id 
                  << ": " << result.score << "\n";
        for (const auto& snippet : result.snippets) {
            std::cout << "  " << snippet << "\n";
        }
        for (const auto& [original, corrected] : result.expanded_terms) {
            std::cout << "  Fuzzy: " << original << " → " << corrected << "\n";
        }
    }
    
    // Register custom ranker
    engine.registerCustomRanker(std::make_unique<CustomMLRanker>());
    engine.setDefaultRanker("ML-Ranker");
    
    // Browse documents  
    auto docs = engine.getDocuments(0, 10);
    
    // Persistence
    engine.saveSnapshot("index.bin");
    
    SearchEngine engine2;
    engine2.loadSnapshot("index.bin");
    
    // Statistics
    auto stats = engine2.getStats();
    std::cout << "Documents: " << stats.total_documents << "\n";
    std::cout << "Terms: " << stats.total_terms << "\n";
    
    auto cache_stats = engine2.getCacheStats();
    std::cout << "Cache hit rate: " << cache_stats.hit_rate << "\n";
    
    return 0;
}
```

### Loading Documents from Files

```cpp
#include "document_loader.hpp"
#include "search_engine.hpp"
using namespace rtrv_search_engine;

int main() {
    DocumentLoader loader;
    
    // Load JSONL (one JSON object per line)
    auto docs = loader.loadJSONL("data/wikipedia_sample.json");
    
    // Load CSV with column names
    auto csv_docs = loader.loadCSV("data.csv", {"title", "content", "category"});
    
    SearchEngine engine;
    engine.indexDocuments(docs);
    
    return 0;
}
```

### Example Programs

See the `examples/` directory:

1. **[`simple_search.cpp`](examples/simple_search.cpp)**: Basic indexing and search
2. **[`batch_indexing.cpp`](examples/batch_indexing.cpp)**: Loading and indexing large corpora
3. **[`skip_pointer_demo.cpp`](examples/skip_pointer_demo.cpp)**: Skip pointer performance demonstration

```bash
cd build
make -j$(nproc)
./examples/simple_search
./examples/batch_indexing ../data/wikipedia_sample.json
./examples/skip_pointer_demo
```

---

## 9. Performance Highlights

| Feature | Performance | vs Baseline |
|---------|-------------|-------------|
| **SIMD Tokenization (AVX2)** | 300-500 MB/s | 2-3x vs scalar |
| **SIMD Tokenization (ARM NEON)** | ~200-400 MB/s | ~2x vs scalar |
| **Top-K Heap (k=10, n=10K)** | 5-8 µs | 4-8x vs full sort |
| **Skip Pointers (AND query)** | 8-12 µs | 2-5x vs linear scan |
| **BM25 Ranking** | ~10 µs/query | ~100K queries/sec |
| **Index Throughput** | 200-300K docs/sec | Sub-microsecond latency |
| **LRU Cache Hit** | <1 µs | Near-instant for repeated queries |

---

## 10. Future Roadmap

### Planned Features

- [ ] **Query Features**:
  - Wildcard prefix queries (`term*`)
  - Regular expression search
  - Faceted search on metadata fields
  
- [ ] **Performance**:
  - AVX-512 SIMD support
  - GPU acceleration for large-scale ranking
  - Parallel indexing with thread pool
  - Bloom filters for negative queries
  
- [ ] **Storage**:
  - Compressed index format (delta encoding, variable-length integers)
  - Memory-mapped files for large indices
  - Incremental index updates
  - Distributed index sharding
  
- [ ] **API**:
  - WebSocket streaming results
  - Batch operations API
  - Index replication

---

## 11. References & Resources

### Information Retrieval
- [Introduction to Information Retrieval](https://nlp.stanford.edu/IR-book/) — Manning, Raghavan, Schütze
- [BM25 Paper](https://en.wikipedia.org/wiki/Okapi_BM25) — Robertson, Zaragoza
- [Skip Pointers in Information Retrieval](https://nlp.stanford.edu/IR-book/html/htmledition/faster-postings-list-intersection-via-skip-pointers-1.html) — Stanford IR Book

### C++ and Performance
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [SIMD Programming — Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)

### Libraries
- [Google Test](https://github.com/google/googletest) — v1.12.1
- [Google Benchmark](https://github.com/google/benchmark) — v1.8.3
- [Drogon](https://github.com/drogonframework/drogon) — High-performance C++ HTTP framework
- [nlohmann/json](https://github.com/nlohmann/json) — v3.11.3, JSON for Modern C++

### Project Documentation
- [Server README](server/README.md) — REST API and server documentation
- [Benchmark Guide](benchmarks/BENCHMARK_GUIDE.md) — Comprehensive benchmarking documentation
- [Example Results](benchmarks/EXAMPLE_RESULTS.md) — Sample benchmark outputs
- [Examples README](examples/README.md) — Usage examples and tutorials

---

**Rtrv** — A blazing fast, enterprise-grade in-memory search engine demonstrating advanced C++ systems programming, high-performance algorithms, and production-ready software engineering practices.
