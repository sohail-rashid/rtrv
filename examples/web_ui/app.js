// Search Engine Web UI

const API_BASE = 'http://localhost:8080';

document.getElementById('searchButton').addEventListener('click', performSearch);
document.getElementById('searchInput').addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        performSearch();
    }
});

async function performSearch() {
    const query = document.getElementById('searchInput').value;
    const algorithm = document.querySelector('input[name="algorithm"]:checked').value;
    const maxResults = document.getElementById('maxResults').value;
    
    if (!query.trim()) {
        return;
    }
    
    try {
        // TODO: Implement API call when REST server is running
        const url = `${API_BASE}/search?q=${encodeURIComponent(query)}&algorithm=${algorithm}&max_results=${maxResults}`;
        
        const response = await fetch(url);
        const data = await response.json();
        
        displayResults(data.results);
        displayStats(data);
    } catch (error) {
        console.error('Search error:', error);
        document.getElementById('results').innerHTML = 
            '<div class="error">Error: Unable to connect to search server</div>';
    }
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
        <div>Found ${data.total_results} results in ${data.query_time_ms.toFixed(2)}ms</div>
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
        const response = await fetch(`${API_BASE}/stats`);
        const stats = await response.json();
        
        console.log('Index statistics:', stats);
    } catch (error) {
        console.log('Server not running');
    }
}

loadStats();
