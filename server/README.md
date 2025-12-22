# Search Engine Servers

This directory contains multiple REST API server implementations and an interactive CLI server for the search engine, along with a web-based user interface.

## Server Implementations

### 1. Raw Socket REST Server (`rest_server.cpp`)

A lightweight REST API server using raw POSIX sockets with manual HTTP parsing.

```bash
./rest_server [port]
```

**Features:**
- ✅ No external dependencies (pure C++ standard library)
- ✅ Manual HTTP request/response handling
- ✅ Multithreaded (one thread per connection)
- ✅ Request/response logging with timestamps and emojis (📥✅⚠️❌)
- ✅ CORS support for web UI
- ⚡ Performance: ~5,000 requests/second

**Use Case:** Learning tool, embedded systems, or environments without framework dependencies.

---

### 2. Drogon REST Server (`rest_server_drogon.cpp`)

High-performance async HTTP server using the Drogon framework.

```bash
./rest_server_drogon [port]
```

**Features:**
- ✅ Production-ready async I/O
- ✅ Built-in JSON handling (JsonCpp)
- ✅ Multi-threaded event loop (4 threads)
- ✅ Request/response logging
- ✅ CORS middleware
- ⚡ Performance: ~50,000+ requests/second

**Requirements:**
```bash
brew install drogon  # macOS
```

**Use Case:** Production deployments requiring high throughput and low latency.

---

### 3. Crow REST Server (`rest_server_crow.cpp`)

Modern C++ REST server using the Crow micro-framework.

```bash
./rest_server_crow [port]
```

**Features:**
- ✅ Simple, Flask-like API
- ✅ Header-only or installed via package manager
- ✅ Built-in JSON support
- ✅ Multithreaded server
- ✅ Request/response logging with client IP tracking
- ✅ CORS support
- ⚡ Performance: ~20,000 requests/second

**Requirements:**
```bash
brew install crow  # macOS
```

**Use Case:** Rapid prototyping, small-to-medium scale deployments.

---

### 4. Interactive CLI Server (`rest_server_interactive.cpp`)

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

All REST servers expose the same API:

### Search
```http
GET /search?q=<query>&algorithm=<bm25|tfidf>&max_results=<n>
```

**Example:**
```bash
curl "http://localhost:8080/search?q=machine+learning&algorithm=bm25&max_results=10"
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
      }
    }
  ],
  "total_results": 5
}
```

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

### Index Document
```http
POST /index
Content-Type: application/json

{
  "id": 123,
  "content": "Document text here"
}
```

### Delete Document
```http
DELETE /delete/<id>
```

### Save Snapshot
```http
POST /save
Content-Type: application/json

{
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

---

## Web UI

A browser-based interface for the search engine located in `web_ui/`.

### Running the Web UI

The web UI consists of static files (`index.html`, `app.js`, `style.css`) that need to be served through a web server.

**Prerequisites:**
- A running REST API server (any of the implementations above)
- A web server to serve static files (Python, Node.js, PHP, or VS Code Live Server)

**Complete Setup:**

1. **Start a REST API server (in terminal 1):**
   ```bash
   cd build/server
   ./rest_server_drogon 8080
   ```
   
   The server will automatically load `wikipedia_sample.json` and display:
   ```
   ✅ Loaded 50 documents from ../data/wikipedia_sample.json
   === Search Engine REST Server (Drogon) ===
   Server will listen on http://localhost:8080
   ```

2. **Start the web UI server (in terminal 2):**
   
   **Option A: Using Python (recommended):**
   ```bash
   cd server/web_ui
   python3 -m http.server 3000
   ```
   
   **Option B: Using Node.js:**
   ```bash
   cd server/web_ui
   npx http-server -p 3000
   ```
   
   **Option C: Using PHP:**
   ```bash
   cd server/web_ui
   php -S localhost:3000
   ```
   
   **Option D: Using VS Code Live Server:**
   - Install "Live Server" extension in VS Code
   - Open `server/web_ui/index.html`
   - Right-click → "Open with Live Server"

3. **Access in browser:**
   ```
   http://localhost:3000
   ```
   
   Or click the URL shown in the terminal output.

**Quick Start (One-Liner):**
```bash
# Terminal 1: Start REST server (Drogon - high performance)
cd build/server && ./rest_server_drogon 8080

# Terminal 2: Start web UI
cd server/web_ui && python3 -m http.server 3000
```

### Features

- 🔍 Real-time search with instant results
- ⚙️ Algorithm selection (BM25 or TF-IDF)
- 📊 Configurable max results slider
- 📈 Live statistics display
- 🎨 Modern, responsive design
- 🚀 Demo mode for testing without a backend

### Configuration

Edit `web_ui/app.js` to change settings:

```javascript
const API_BASE = 'http://localhost:8080';  // REST API endpoint
const DEMO_MODE = false;                    // Set true for offline testing
```

**Available Settings:**
- `API_BASE`: URL of your REST API server (change if using different port)
- `DEMO_MODE`: Enable to test UI without a backend (uses mock data)
- `DEFAULT_MAX_RESULTS`: Default number of search results (default: 10)
- `DEFAULT_ALGORITHM`: Default algorithm ('bm25' or 'tfidf')

**How app.js Works:**
1. Runs in the browser (client-side JavaScript)
2. Makes HTTP requests to the REST API server
3. Dynamically updates the UI based on responses
4. No server-side execution required for the JavaScript itself

**Architecture:**
```
Browser (port 3000)          REST API Server (port 8080)
┌─────────────────┐         ┌──────────────────────┐
│  index.html     │         │   rest_server        │
│  style.css      │         │   (C++ backend)      │
│  app.js         │────────▶│   /search, /stats    │
│  (JavaScript)   │◀────────│   (JSON responses)   │
└─────────────────┘         └──────────────────────┘
     │                               │
     │                               │
     └── HTTP Server ─────────────── SearchEngine
         (Python/Node/PHP)            + Documents
```

---

## Building

### Prerequisites

**Required (all servers):**
- CMake 3.15+
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- Threads library

**Optional (framework servers):**
```bash
# macOS
brew install drogon      # For rest_server_drogon
brew install crow        # For rest_server_crow

# Ubuntu/Debian
sudo apt install libdrogon-dev libjsoncpp-dev  # For rest_server_drogon
# Crow is header-only, clone from: https://github.com/CrowCpp/Crow

# Fedora/RHEL
sudo dnf install drogon-devel jsoncpp-devel    # For rest_server_drogon
```

### Build All Servers

From the project root:

```bash
# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Build all available servers
make

# Or build with all CPU cores
make -j$(nproc)
```

Executables will be in `build/server/`:
- `rest_server` (always built)
- `rest_server_drogon` (if Drogon is installed)
- `rest_server_crow` (if Crow is installed)
- `interactive_server` (always built)

### Build Specific Servers

```bash
cd build

# Build only the raw socket server
make rest_server

# Build only the interactive CLI
make interactive_server

# Build only Crow server (requires Crow)
make rest_server_crow

# Build only Drogon server (requires Drogon)
make rest_server_drogon
```

### Rebuild After Changes

```bash
cd build

# Rebuild specific server after source changes
make rest_server

# Force clean rebuild
rm -rf *
cmake ..
make
```

### Build Configuration Options

```bash
# Debug build with symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Specify compiler
cmake -DCMAKE_CXX_COMPILER=clang++ ..
make
```

### Verify Build

```bash
cd build/server

# Check if servers were built
ls -lh rest_*

# Test run (should show usage or start server)
./rest_server --help 2>/dev/null || ./rest_server 8080 &
sleep 1
curl http://localhost:8080/stats
pkill rest_server
```

---

## Quick Start

### 1. Start a REST Server
```bash
cd build/server
./rest_server_drogon 8080  # Or use rest_server, rest_server_crow
```

### 2. Test with curl
```bash
# Search
curl "http://localhost:8080/search?q=machine+learning"

# Get stats
curl "http://localhost:8080/stats"

# Index a document
curl -X POST http://localhost:8080/index \
  -H "Content-Type: application/json" \
  -d '{"id": 100, "content": "New document content"}'
```

### 3. Use the Web UI
```bash
cd server/web_ui
python3 -m http.server 3000
# Open http://localhost:3000 in browser
```

---

## Performance Comparison

| Server | Throughput | Latency (p50) | Dependencies | Best For |
|--------|------------|---------------|--------------|----------|
| **Raw Socket** | ~5k req/s | ~2ms | None | Learning, embedded |
| **Crow** | ~20k req/s | ~1ms | Crow, Asio | Prototyping, small scale |
| **Drogon** | ~50k+ req/s | ~0.5ms | Drogon, JsonCpp | Production, high load |

*Benchmarks run on Apple M1 Pro, 10 concurrent connections*

---

## Logging Format

All servers use consistent logging:

**Incoming Request:**
```
📥 [2025-12-16 18:45:23.456] [127.0.0.1] GET /search?q=test
```

**Response:**
```
✅ [2025-12-16 18:45:23.458] [127.0.0.1] GET /search → 200  (Success)
⚠️ [2025-12-16 18:45:24.123] [127.0.0.1] POST /index → 400 (Client Error)
❌ [2025-12-16 18:45:25.789] [127.0.0.1] GET /stats → 500  (Server Error)
```

---

## Sample Data

All REST servers automatically load sample data from `data/wikipedia_sample.json` (JSONL format) on startup:
- 50 computer science articles
- Topics: AI, ML, algorithms, systems, web technologies
- JSONL format: One JSON object per line with `id`, `title`, `content`, `category` fields

Servers try multiple paths automatically:
- `../data/wikipedia_sample.json` (when running from `build/server/`)
- `../../data/wikipedia_sample.json` (when running from `build/`)
- `data/wikipedia_sample.json` (when running from project root)

To use your own data:
1. Create JSONL file with documents (one JSON object per line)
2. Place in `data/` directory
3. Update file path in server source code or pass via command line
4. Rebuild: `cd build && make <server_name>`

**Example JSONL format:**
```json
{"id": 1, "title": "Machine Learning", "content": "Machine learning is...", "category": "AI"}
{"id": 2, "title": "Algorithms", "content": "An algorithm is...", "category": "CS"}
```

---

## Troubleshooting

### Port Already in Use
```bash
# Check what's using the port
lsof -i :8080

# Use a different port
./rest_server 8081
```

### CORS Errors in Web UI
All servers have CORS enabled by default. If issues persist:
1. Check browser console for specific error
2. Verify `API_BASE` in `app.js` matches server URL
3. Try accessing API directly with curl first

### Server Not Found During Build
```bash
# Install missing frameworks
brew install drogon  # For rest_server_drogon
brew install crow    # For rest_server_crow
```

### Can't Load Sample Data
Ensure you run servers from the build directory:
```bash
cd build/server
./rest_server_drogon  # Will load ../data/wikipedia_sample.json
# Or use: ./rest_server, ./rest_server_crow, ./interactive_server
```

---

## Next Steps

1. **Load More Data**: Use `batch_indexing` tool to load custom datasets
2. **Benchmark**: Test performance with your workload
3. **Customize**: Modify algorithms, add endpoints, enhance UI
4. **Deploy**: Use Drogon server with Nginx for production
5. **Scale**: Add load balancing, caching, and replication

## License

See project root LICENSE file for details.
