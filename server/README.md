# Rtrv Search Engine Servers

This directory contains REST API and interactive CLI server implementations for the Rtrv search engine, along with a glassmorphism-styled web UI.

## Server Implementations

### 1. Drogon REST Server (`rest_server_drogon.cpp`)

High-performance async HTTP server using the Drogon framework. Serves both the REST API and the web UI as static files.

```bash
./rest_server_drogon [port]   # default port: 8080
```

**Features:**
- ✅ Production-ready async I/O
- ✅ Built-in JSON handling (JsonCpp)
- ✅ Multi-threaded event loop (4 threads)
- ✅ Request/response logging with emoji indicators
- ✅ CORS middleware (all origins, GET/POST/DELETE/OPTIONS)
- ✅ Static file serving for the web UI
- ✅ Auto-resolves UI root from multiple directory candidates
- ⚡ Performance: ~50,000+ requests/second

**Requirements:**
```bash
brew install drogon  # macOS
```

**Use Case:** Production deployments requiring high throughput and low latency.

---

### 2. Interactive CLI Server (`rest_server_interactive.cpp`)

Command-line interface for manual search engine interaction.

```bash
./interactive_server
```

**Features:**
- ⚡ Interactive REPL (Read-Eval-Print Loop)
- 📝 Command registry with aliases
- 💡 Fuzzy command suggestions for typos
- 🎨 Beautiful UI with box-drawing characters
- ⌨️ Tab-friendly command completion hints

**Available Commands:**
| Command | Aliases | Description |
|---------|---------|-------------|
| `index <id> <content>` | - | Index a document |
| `search <query>` | `find` | Search for documents |
| `delete <id>` | `del`, `rm` | Delete a document |
| `stats` | - | Show index statistics |
| `save <file>` | - | Save snapshot to file |
| `load <file>` | - | Load snapshot from file |
| `clear` | `cls` | Clear the screen |
| `help` | `?` | Show help menu |
| `quit` | `q`, `exit` | Exit the server |

**Use Case:** Development, debugging, and manual testing.

---

## REST API Endpoints

### Search
```http
GET /search?q=<query>
```

**Parameters:**
| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `q` | Yes | — | Search query string |
| `algorithm` | No | `bm25` | Ranking algorithm: `bm25` or `tfidf` |
| `max_results` | No | `10` | Maximum number of results |
| `use_top_k_heap` | No | `true` | Use Top-K heap (O(N log K)) vs full sort (O(N log N)) |
| `highlight` | No | `false` | Enable snippet generation / highlighting |
| `snippet_length` | No | — | Max length per snippet (chars) |
| `num_snippets` | No | — | Number of snippets to return per result |
| `fuzzy` | No | `false` | Enable fuzzy matching (edit-distance expansion) |
| `max_edit_distance` | No | — | Max edit distance for fuzzy matching |
| `cache` | No | `true` | Enable/disable query cache for this request |

**Example:**
```bash
curl "http://localhost:8080/search?q=machine+learning&algorithm=bm25&max_results=5&highlight=true&fuzzy=true"
```

**Response:**
```json
{
  "results": [
    {
      "score": 8.234567,
      "document": {
        "id": 2,
        "content": "Machine learning is a branch of artificial intelligence..."
      },
      "snippets": [
        "...branch of <b>artificial intelligence</b> that focuses on..."
      ],
      "expanded_terms": {
        "learning": "learnings"
      }
    }
  ],
  "total_results": 5
}
```

> **Note:** `snippets` is only present when `highlight=true`. `expanded_terms` is only present when `fuzzy=true` and expansions were used.

---

### List Documents
```http
GET /documents?offset=<n>&limit=<n>
```

Browse indexed documents with pagination. Used by the web UI for empty searches.

**Parameters:**
| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `offset` | No | `0` | Number of documents to skip |
| `limit` | No | `10` | Number of documents to return (max 1000) |

**Example:**
```bash
curl "http://localhost:8080/documents?offset=0&limit=5"
```

**Response:**
```json
{
  "results": [
    {
      "score": 0.0,
      "document": {
        "id": 1,
        "content": "Document content here..."
      }
    }
  ],
  "total_results": 5,
  "total_documents": 50
}
```

---

### Statistics
```http
GET /stats
```

**Response:**
```json
{
  "total_documents": 50,
  "total_terms": 1234,
  "avg_doc_length": 45.6
}
```

---

### Cache Statistics
```http
GET /cache/stats
```

**Response:**
```json
{
  "hit_count": 120,
  "miss_count": 45,
  "eviction_count": 5,
  "current_size": 30,
  "max_size": 100,
  "hit_rate": 0.727
}
```

### Clear Cache
```http
DELETE /cache
```

**Response:**
```json
{
  "success": true
}
```

---

### Index Document
```http
POST /index
Content-Type: application/json

{
  "id": 123,
  "content": "Document text here"
}
```

**Response:**
```json
{
  "success": true,
  "doc_id": 123
}
```

### Delete Document
```http
DELETE /delete/<id>
```

**Response:**
```json
{
  "success": true,
  "doc_id": 123
}
```

---

### Save Snapshot
```http
POST /save
Content-Type: application/json

{
  "filename": "snapshot.bin"
}
```

**Response:**
```json
{
  "success": true,
  "filename": "snapshot.bin"
}
```

### Load Snapshot
```http
POST /load
Content-Type: application/json

{
  "filename": "snapshot.bin"
}
```

**Response:**
```json
{
  "success": true,
  "filename": "snapshot.bin"
}
```

---

### Skip Pointer Management

#### Rebuild All Skip Pointers
```http
POST /skip/rebuild
```

#### Rebuild Skip Pointers for a Specific Term
```http
POST /skip/rebuild/<term>
```

#### Get Skip Pointer Stats
```http
GET /skip/stats?term=<term>
```

**Response:**
```json
{
  "term": "machine",
  "postings_count": 15,
  "skip_pointers_count": 3,
  "skip_interval": 5,
  "needs_rebuild": false
}
```

---

## Endpoint Summary

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

---

## Web UI

A glassmorphism-styled single-page dashboard located in `ui/`. Served directly by the Drogon server at the root URL.

**Files:**
- `index.html` — Dashboard layout with sidebar and content area
- `style.css` — Glassmorphism theme with mesh-gradient background
- `app.js` — Client-side search, index management, and view switching
### Running the Web UI

**Quick Launch (Automated Script):**

From the project root:
```bash
./launch_webui.sh
```

The script will:
- ✅ Start the Drogon REST server
- ✅ Check for port conflicts
- ✅ Open your browser automatically
- ✅ Handle graceful shutdown with Ctrl+C

**Manual Setup:**

1. **Start the server:**
   ```bash
   cd build/server
   ./rest_server_drogon 8080
   ```
   
   The server will automatically load `wikipedia_sample.json` and display:
   ```
   ✅ Loaded 50 documents from ../data/wikipedia_sample.json
   === Rtrv REST Server (Drogon) ===
   Server will listen on http://localhost:8080
   ```

2. **Open in browser:**
   ```
   http://localhost:8080
   ```

### Features

**Search View:**
- 🔍 Real-time search with instant results
- ⚙️ Algorithm selection (BM25 or TF-IDF)
- 📊 Configurable max results slider
- 🔤 Fuzzy matching and snippet highlighting
- 📄 Empty search browses all documents via `/documents` endpoint

**Index View:**
- 📈 Live index statistics (document count, term count, avg doc length)
- 💾 Cache monitoring (hit rate, hits, misses, evictions, size)
- ➕ Add documents (ID + content form)
- 🗑️ Delete documents by ID
- 🧹 Clear query cache
- 🔄 Auto-refresh stats every 15 seconds

**Design:**
- Glassmorphism UI with translucent glass panels and backdrop blur
- Mesh-gradient background with violet accent color scheme
- SVG logo (violet gradient rounded square with magnifying glass)
- Inter font family via Google Fonts
- Sidebar view switcher (Search / Index pill tabs)
- Responsive layout with mobile breakpoints

### Configuration

Edit `ui/app.js` to change settings:

```javascript
const CONFIG = {
  apiBase: window.location.origin,
  requestTimeoutMs: 4000,
  enableFuzzy: true,
  enableHighlight: true,
  defaultMaxResults: 10,
  statsRefreshMs: 15000
};
```

**Available Settings:**
| Setting | Default | Description |
|---------|---------|-------------|
| `apiBase` | `window.location.origin` | REST API server URL |
| `requestTimeoutMs` | `4000` | Request timeout (ms) |
| `enableFuzzy` | `true` | Enable fuzzy matching |
| `enableHighlight` | `true` | Enable snippet highlighting |
| `defaultMaxResults` | `10` | Default max search results |
| `statsRefreshMs` | `15000` | Index view stats auto-refresh interval (ms) |

### Architecture

```
Browser (port 8080)               Drogon Server (port 8080)
┌───────────────────────┐         ┌───────────────────────────┐
│  index.html           │         │  rest_server_drogon.cpp   │
│  style.css            │         │  (C++ backend, 4 threads) │
│  app.js               │         │                           │
│                       │  HTTP   │  Endpoints:               │
│  Search View ─────────│────────▶│  /search, /documents      │
│  Index View  ─────────│────────▶│  /stats, /cache/stats     │
│                       │◀────────│  /index, /delete, /cache  │
│  (glassmorphism UI)   │  JSON   │  /save, /load, /skip/*    │
└───────────────────────┘         └───────────────────────────┘
                                           │
                                    SearchEngine
                                   + InvertedIndex
                                   + QueryCache
                                   + SkipPointers
```

---

## Building

### Prerequisites

**Required (all servers):**
- CMake 3.15+
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- Threads library

**Optional (Drogon server):**
```bash
# macOS
brew install drogon

# Ubuntu/Debian
sudo apt install libdrogon-dev libjsoncpp-dev

# Fedora/RHEL
sudo dnf install drogon-devel jsoncpp-devel
```

### Build All Servers

From the project root:

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

Executables will be in `build/server/`:
- `rest_server_drogon` (if Drogon is installed)
- `interactive_server` (always built)

### Build Specific Servers

```bash
cd build
make interactive_server     # CLI only
make rest_server_drogon     # Drogon server only (requires Drogon)
```

### Rebuild After Changes

```bash
cd build
make -j$(nproc)

# Or force a clean rebuild
rm -rf * && cmake .. && make -j$(nproc)
```

### Build Configuration Options

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..        # Debug build with symbols
cmake -DCMAKE_BUILD_TYPE=Release ..      # Release build with optimizations
cmake -DCMAKE_CXX_COMPILER=clang++ ..   # Specify compiler
```

### Verify Build

```bash
cd build/server
ls -lh rest_server_drogon interactive_server

# Quick smoke test
./rest_server_drogon 8080 &
sleep 1
curl http://localhost:8080/stats
curl "http://localhost:8080/documents?limit=2"
pkill rest_server_drogon
```

---

## Quick Start

### 1. Start the Server
```bash
cd build/server
./rest_server_drogon 8080
```

### 2. Test with curl
```bash
# Search with fuzzy + highlighting
curl "http://localhost:8080/search?q=machine+learning&fuzzy=true&highlight=true"

# Browse documents
curl "http://localhost:8080/documents?offset=0&limit=5"

# Get stats
curl "http://localhost:8080/stats"

# Get cache stats
curl "http://localhost:8080/cache/stats"

# Index a document
curl -X POST http://localhost:8080/index \
  -H "Content-Type: application/json" \
  -d '{"id": 100, "content": "New document content"}'

# Delete a document
curl -X DELETE http://localhost:8080/delete/100

# Clear cache
curl -X DELETE http://localhost:8080/cache
```

### 3. Use the Web UI
```bash
./launch_webui.sh
# Or open http://localhost:8080 after starting the server
```

---

## Logging Format

All servers use consistent logging with emoji indicators:

**Incoming Request:**
```
📥 [127.0.0.1] GET /search?q=test
```

**Response:**
```
✅ [127.0.0.1] GET /search → 200
⚠️ [127.0.0.1] POST /index → 400
❌ [127.0.0.1] GET /stats → 500
```

---

## Sample Data

The server automatically loads sample data from `data/wikipedia_sample.json` (JSONL format) on startup:
- 50 computer science articles
- Topics: AI, ML, algorithms, systems, web technologies
- JSONL format: One JSON object per line with `id`, `title`, `content`, `category` fields

The server tries multiple paths automatically:
- `../data/wikipedia_sample.json` (running from `build/server/`)
- `../../data/wikipedia_sample.json` (running from `build/`)
- `data/wikipedia_sample.json` (running from project root)

To use your own data:
1. Create a JSONL file with documents (one JSON object per line)
2. Place in `data/` directory
3. Update file path in server source code or pass via command line
4. Rebuild: `cd build && make -j$(nproc)`

**Example JSONL format:**
```json
{"id": 1, "title": "Machine Learning", "content": "Machine learning is...", "category": "AI"}
{"id": 2, "title": "Algorithms", "content": "An algorithm is...", "category": "CS"}
```

---

## Troubleshooting

### Port Already in Use
```bash
lsof -i :8080              # Check what's using the port
./rest_server_drogon 8081   # Use a different port
```

### CORS Errors in Web UI
CORS is enabled by default for all origins. If issues persist:
1. Check browser console for specific error
2. Verify `apiBase` in `app.js` matches server URL
3. Try accessing API directly with curl first

### Server Not Found During Build
```bash
brew install drogon  # macOS — installs Drogon + JsonCpp
```

### Can't Load Sample Data
Run the server from the correct directory:
```bash
cd build/server
./rest_server_drogon 8080   # Will find ../data/wikipedia_sample.json
```

### Web UI Not Loading
The server auto-resolves the UI directory by checking multiple paths. If the UI doesn't load, ensure `server/ui/index.html` exists relative to your working directory.

---

## License

See project root LICENSE file for details.
