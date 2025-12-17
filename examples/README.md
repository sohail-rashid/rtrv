# Search Engine Examples

This directory contains example programs demonstrating how to use the search engine library.

## Examples

### 1. Simple Search (`simple_search.cpp`)

A basic example showing fundamental search engine operations:

```bash
./simple_search
```

**Features:**
- Index a few sample documents
- Perform simple search queries
- Display ranked results with scores

**Use Case:** Quick introduction to the search engine API for new users.

---

### 2. Batch Indexing (`batch_indexing.cpp`)

Bulk indexing from a file with performance metrics:

```bash
./batch_indexing <corpus_file>
```

**Example:**
```bash
./batch_indexing ../data/wikipedia_sample.txt
```

**Features:**
- Load and index documents from pipe-delimited files (`ID|content` format)
- Real-time progress indicator (updates every 100 documents)
- Performance metrics:
  - Total indexing time
  - Throughput (documents/second)
- Index statistics:
  - Total documents indexed
  - Total unique terms
  - Average document length
- Automatic snapshot saving to `index_snapshot.bin`

**Input File Format:**
```
1|First document content here
2|Second document content here
3|Third document content here
```

**Use Case:** Production-scale indexing workflows and performance benchmarking.

---

## Building

These examples are automatically built with the main project:

```bash
cd build
cmake ..
cmake --build .
```

Executables will be available in `build/examples/`:
- `simple_search`
- `batch_indexing`

## Sample Data

Use the provided Wikipedia sample dataset:
```bash
cd build/examples
./batch_indexing ../data/wikipedia_sample.txt
```

This will index 50 computer science articles covering topics like AI, machine learning, algorithms, and more.

## Performance Tips

- **Batch Indexing**: Use `batch_indexing` for large datasets (1000+ documents)
- **Snapshots**: Save snapshots after indexing to avoid re-indexing on restart
- **Progress Tracking**: Monitor the throughput metric to identify bottlenecks

## Next Steps

After running these examples, try:
1. **REST API Servers** - Start a server in `server/` directory
2. **Web UI** - Use the browser interface in `examples/web_ui/`
3. **Custom Applications** - Build your own search applications using these as templates
