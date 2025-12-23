# Search Engine Web UI

Modern, responsive web interface for the C++ Search Engine with advanced features and performance monitoring.

## Features

### 🎯 **Core Search**
- Real-time search with instant results
- Support for multi-term queries
- Intuitive search interface with keyboard shortcuts (Enter to search)

### ⚡ **Performance Optimization**
- **Top-K Heap Toggle**: Enable/disable bounded priority queue optimization
  - **50-150x faster** for small result sets (K < 1000)
  - Visual indicator shows when heap optimization is active
  - Automatic recommendations based on result count
  
### 🎨 **Ranking Algorithms**
- **BM25** (Recommended): Modern probabilistic ranking algorithm
  - 15-20% faster than TF-IDF
  - Better ranking quality for most use cases
- **TF-IDF**: Classic term frequency-inverse document frequency

### 📊 **Real-time Performance Metrics**
- **Query Time**: Response latency with color-coded indicators
  - Green pulse: < 10ms (Super Fast)
  - Yellow pulse: 10-50ms (Fast)
  - Normal: > 50ms
- **Results Count**: Number of matching documents
- **Algorithm Used**: Active ranking algorithm
- **Optimization Method**: Heap vs Sort indicator

### 🔧 **Advanced Options**
- **Early Termination**: Skip processing when quality threshold is met
- **Query Caching**: Cache frequent queries for faster response
- **Max Results**: Configurable from 1 to 1000

### 💬 **User Feedback**
- Toast notifications for actions and status updates
- Success/error/warning/info messages
- Loading states during search

### 📱 **Responsive Design**
- Mobile-friendly interface
- Adaptive layout for all screen sizes
- Touch-optimized controls

## Quick Start

### Option 1: Using the Launch Script (Recommended)

```bash
cd server/web_ui
chmod +x launch_webui.sh
./launch_webui.sh
```

This will:
1. Start the REST API server (port 8080)
2. Start the web UI server (port 3000)
3. Open your browser automatically

### Option 2: Manual Setup

1. **Start the REST API Server:**
```bash
cd build/server
./rest_server_drogon  # or rest_server_crow, or rest_server
```

2. **Serve the Web UI:**
```bash
cd server/web_ui
python3 -m http.server 3000
```

3. **Open in browser:**
```
http://localhost:3000
```

## Configuration

### API Endpoint
Edit `app.js` to change the API endpoint:

```javascript
const API_BASE = 'http://localhost:8080';
```

### Demo Mode
For testing without a running server:

```javascript
const DEMO_MODE = true;  // Set to true for demo mode
```

## Usage Guide

### Basic Search
1. Type your query in the search box
2. Press Enter or click "Search"
3. Results appear instantly with relevance scores

### Optimizing Performance

**For K < 100 (typical use case):**
- ✅ Enable "Top-K Heap" (default)
- ✅ Use BM25 algorithm
- Expected: < 10ms response time

**For K > 1000 (large result sets):**
- ❌ Disable "Top-K Heap"
- ✅ Use BM25 algorithm
- Expected: 50-100ms response time

### Advanced Features

Click "⚙️ Advanced" to access:
- **Early Termination**: Stops processing early when confident in results
- **Query Cache**: Stores recent queries for instant retrieval

## Performance Comparison

| Configuration | Documents | K | Response Time | Method |
|--------------|-----------|---|---------------|--------|
| Heap + BM25  | 100K      | 10 | ~13 μs | **Fastest** |
| Sort + BM25  | 100K      | 10 | ~968 μs | 74x slower |
| Heap + TF-IDF | 100K     | 10 | ~15 μs | Slightly slower |
| Sort + TF-IDF | 100K     | 10 | ~985 μs | 75x slower |

## API Integration

The web UI calls the following REST endpoint:

```
GET /search?q={query}&algorithm={bm25|tfidf}&max_results={n}&use_top_k_heap={true|false}
```

**Example:**
```bash
curl "http://localhost:8080/search?q=machine+learning&algorithm=bm25&max_results=10&use_top_k_heap=true"
```

**Response:**
```json
{
  "query": "machine learning",
  "results": [
    {
      "document": {
        "id": 42,
        "content": "Machine learning algorithms..."
      },
      "score": 8.245
    }
  ],
  "total_results": 15,
  "query_time_ms": 12.5
}
```

## Browser Compatibility

- ✅ Chrome/Edge 90+
- ✅ Firefox 88+
- ✅ Safari 14+
- ✅ Mobile browsers (iOS Safari, Chrome Mobile)

## Troubleshooting

### "Unable to connect to search server"
1. Verify REST API server is running: `curl http://localhost:8080/stats`
2. Check firewall settings
3. Enable DEMO_MODE in `app.js` for testing

### Slow Performance
1. Enable "Top-K Heap" for K < 1000
2. Use BM25 algorithm (faster than TF-IDF)
3. Enable "Early Termination" in Advanced options
4. Reduce Max Results if not needed

### Results Not Relevant
1. Try BM25 algorithm (better ranking quality)
2. Disable "Early Termination"
3. Check that documents are properly indexed

## Development

### File Structure
```
web_ui/
├── index.html       # Main HTML structure
├── app.js           # JavaScript logic and API calls
├── style.css        # Styling and animations
├── launch_webui.sh  # Launcher script
└── README.md        # This file
```

### Customization

**Change Theme Colors:**
Edit gradient in `style.css`:
```css
body {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}
```

**Modify Search Behavior:**
Edit `performSearch()` function in `app.js`

**Add New Options:**
1. Add control in `index.html`
2. Read value in `performSearch()`
3. Pass to API as query parameter

## Credits

Built with:
- Pure JavaScript (no frameworks)
- Modern CSS with animations
- Responsive design principles
- REST API integration

## License

Part of the C++ Search Engine project.
