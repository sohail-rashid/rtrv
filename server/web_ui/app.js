// Search Engine Web UI

const API_BASE = 'http://localhost:8080';
const DEMO_MODE = false; // Set to false when REST server is available

// Sample data for demo mode
const DEMO_DOCUMENTS = [
    { id: 1, content: "The quick brown fox jumps over the lazy dog" },
    { id: 2, content: "A quick brown dog runs in the park" },
    { id: 3, content: "The lazy cat sleeps all day" },
    { id: 4, content: "Machine learning algorithms process data" },
    { id: 5, content: "Artificial intelligence and machine learning" },
    { id: 6, content: "Natural language processing techniques" },
    { id: 7, content: "Deep learning neural networks" },
    { id: 8, content: "Computer vision image recognition systems" },
    { id: 9, content: "Data science and analytics" },
    { id: 10, content: "Python programming for machine learning" }
];

document.getElementById('searchButton').addEventListener('click', performSearch);
document.getElementById('searchInput').addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        performSearch();
    }
});

async function performSearch() {
    const query = document.getElementById('searchInput').value;
    const algorithm = document.querySelector('input[name="algorithm"]:checked').value;
    const maxResults = parseInt(document.getElementById('maxResults').value);
    
    if (!query.trim()) {
        return;
    }
    
    const startTime = performance.now();
    
    try {
        let data;
        
        if (DEMO_MODE) {
            // Demo mode: simulate search
            data = performDemoSearch(query, algorithm, maxResults);
        } else {
            // Real API call
            const url = `${API_BASE}/search?q=${encodeURIComponent(query)}&algorithm=${algorithm}&max_results=${maxResults}`;
            const response = await fetch(url);
            data = await response.json();
        }
        
        const endTime = performance.now();
        data.query_time_ms = endTime - startTime;
        
        displayResults(data.results);
        displayStats(data);
    } catch (error) {
        console.error('Search error:', error);
        document.getElementById('results').innerHTML = 
            '<div class="error">Error: Unable to connect to search server. Running in demo mode.</div>';
    }
}

function performDemoSearch(query, algorithm, maxResults) {
    // Simple scoring: count matching terms
    const queryTerms = query.toLowerCase().split(/\s+/).filter(t => t.length > 0);
    
    const results = DEMO_DOCUMENTS.map(doc => {
        const docTerms = doc.content.toLowerCase().split(/\s+/);
        let score = 0;
        
        // Count matching terms (exact word match)
        for (const term of queryTerms) {
            for (const docTerm of docTerms) {
                if (docTerm === term) {
                    score += 1.0;
                }
            }
        }
        
        // BM25-style length normalization
        if (algorithm === 'bm25') {
            const avgLen = 7.0;
            const k1 = 1.5;
            const b = 0.75;
            const docLen = docTerms.length;
            score = score / (score + k1 * (1 - b + b * (docLen / avgLen)));
        }
        
        return {
            document: doc,
            score: score
        };
    }).filter(r => r.score > 0)
      .sort((a, b) => b.score - a.score)
      .slice(0, maxResults);
    
    return {
        results: results,
        total_results: results.length
    };
}

function displayResults(results) {
    const resultsDiv = document.getElementById('results');
    
    if (!results || results.length === 0) {
        resultsDiv.innerHTML = '<div class="no-results">No results found</div>';
        return;
    }
    
    let html = '<div class="result-list">';
    results.forEach((result, index) => {
        html += `
            <div class="result-item">
                <div class="result-rank">${index + 1}</div>
                <div class="result-content">
                    <div class="result-score">Score: ${result.score.toFixed(4)}</div>
                    <div class="result-text">${escapeHtml(result.document.content)}</div>
                    <div class="result-meta">Document ID: ${result.document.id}</div>
                </div>
            </div>
        `;
    });
    html += '</div>';
    
    resultsDiv.innerHTML = html;
}

function displayStats(data) {
    const statsDiv = document.getElementById('stats');
    statsDiv.innerHTML = `
        <div class="stats-content">
            <div class="stat-item">
                <div class="stat-icon">📊</div>
                <div class="stat-info">
                    <div class="stat-value">${data.total_results}</div>
                    <div class="stat-label">Results</div>
                </div>
            </div>
            <div class="stat-item">
                <div class="stat-icon">⚡</div>
                <div class="stat-info">
                    <div class="stat-value">${data.query_time_ms.toFixed(2)}ms</div>
                    <div class="stat-label">Query Time</div>
                </div>
            </div>
        </div>
    `;
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// Load index statistics on page load
async function loadStats() {
    try {
        if (DEMO_MODE) {
            const stats = {
                total_documents: DEMO_DOCUMENTS.length,
                total_terms: 45,
                avg_doc_length: 6.5
            };
            displayIndexStats(stats);
        } else {
            const response = await fetch(`${API_BASE}/stats`);
            const stats = await response.json();
            displayIndexStats(stats);
        }
    } catch (error) {
        console.log('Server not running, using demo mode');
    }
}

function displayIndexStats(stats) {
    const infoDiv = document.createElement('div');
    infoDiv.className = 'index-stats';
    infoDiv.innerHTML = `
        <div class="index-stats-content">
            <div class="index-stat">
                <span class="index-icon">📚</span>
                <span class="index-value">${stats.total_documents}</span>
                <span class="index-label">Documents</span>
            </div>
            <div class="index-stat">
                <span class="index-icon">🔤</span>
                <span class="index-value">${stats.total_terms}</span>
                <span class="index-label">Unique Terms</span>
            </div>
            <div class="index-stat">
                <span class="index-icon">📏</span>
                <span class="index-value">${stats.avg_doc_length.toFixed(1)}</span>
                <span class="index-label">Avg Words/Doc</span>
            </div>
            ${DEMO_MODE ? '<div class="demo-badge">🎯 Demo Mode</div>' : ''}
        </div>
    `;
    
    const container = document.querySelector('.container');
    if (container && !document.querySelector('.index-stats')) {
        container.insertBefore(infoDiv, container.firstChild);
    }
}

loadStats();
