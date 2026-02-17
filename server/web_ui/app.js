// Search Engine Web UI

const API_BASE = 'http://localhost:8080';
let DEMO_MODE = true; // Start with demo mode, will check server
const FORCE_DEMO = false;

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

// Background animation function - defined early
function createBackgroundAnimation() {
    try {
        const nodeCount = 25;
        const lineCount = 20;
        const nodes = [];
        
        // Create floating network nodes
        for (let i = 0; i < nodeCount; i++) {
            const node = document.createElement('div');
            node.className = 'network-node';
            
            const startX = Math.random() * window.innerWidth;
            const startY = Math.random() * window.innerHeight;
            const moveX = (Math.random() - 0.5) * 150;
            const moveY = (Math.random() - 0.5) * 150;
            const delay = Math.random() * 20;
            const duration = 15 + Math.random() * 10;
            
            node.style.left = startX + 'px';
            node.style.top = startY + 'px';
            node.style.setProperty('--tx', moveX + 'px');
            node.style.setProperty('--ty', moveY + 'px');
            node.style.animationDelay = delay + 's';
            node.style.animationDuration = duration + 's';
            
            nodes.push({ element: node, x: startX, y: startY });
            document.body.appendChild(node);
        }
        
        // Create connection lines between nearby nodes
        for (let i = 0; i < lineCount; i++) {
            const line = document.createElement('div');
            line.className = 'network-line';
            
            // Pick two random nodes to connect
            const node1 = nodes[Math.floor(Math.random() * nodes.length)];
            const node2 = nodes[Math.floor(Math.random() * nodes.length)];
            
            const dx = node2.x - node1.x;
            const dy = node2.y - node1.y;
            const length = Math.sqrt(dx * dx + dy * dy);
            const angle = Math.atan2(dy, dx) * (180 / Math.PI);
            const delay = Math.random() * 4;
            
            line.style.left = node1.x + 'px';
            line.style.top = node1.y + 'px';
            line.style.width = length + 'px';
            line.style.transform = `rotate(${angle}deg)`;
            line.style.animationDelay = delay + 's';
            
            document.body.appendChild(line);
        }
        
        console.log(`✨ Created ${nodeCount} nodes and ${lineCount} connection lines`);
    } catch (error) {
        console.error('❌ Background animation error:', error);
    }
}

// Initialize background animation - ensure it runs after body is ready
if (document.readyState === 'loading') {
    // DOM still loading, wait for it
    document.addEventListener('DOMContentLoaded', function() {
        setTimeout(createBackgroundAnimation, 100);
    });
} else {
    // DOM already loaded, run immediately
    setTimeout(createBackgroundAnimation, 100);
}

// Auto-detect if REST server is available (async, doesn't block rendering)
(async function checkServerAvailability() {
    if (FORCE_DEMO) {
        DEMO_MODE = true;
        console.log('🎯 Forced demo mode (default)');
        return;
    }
    console.log('🔍 Checking REST server availability...');
    try {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 1000);
        
        const response = await fetch(`${API_BASE}/stats`, { signal: controller.signal });
        clearTimeout(timeoutId);
        
        if (response.ok) {
            DEMO_MODE = false;
            console.log('✅ Connected to REST server - DEMO_MODE: false');
        } else {
            DEMO_MODE = true;
            console.log('🎯 Using demo mode - REST server responded with error - DEMO_MODE: true');
        }
    } catch (error) {
        DEMO_MODE = true;
        console.log('🎯 Using demo mode - REST server not available - DEMO_MODE: true', error.message);
    }
})();

// Wait for DOM before attaching event listeners
// Wait for DOM before attaching event listeners
document.addEventListener('DOMContentLoaded', function() {
    // Search handlers
    document.getElementById('searchButton').addEventListener('click', performSearch);
    document.getElementById('searchInput').addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            performSearch();
        }
    });

    // Clear button
    document.getElementById('clearButton').addEventListener('click', () => {
        document.getElementById('searchInput').value = '';
        document.getElementById('results').innerHTML = '';
        document.getElementById('stats').innerHTML = '';
    });

    // Advanced options toggle
    document.getElementById('advancedToggle').addEventListener('click', () => {
        const advancedDiv = document.getElementById('advancedOptions');
        const isVisible = advancedDiv.style.display !== 'none';
        advancedDiv.style.display = isVisible ? 'none' : 'block';
        document.getElementById('advancedToggle').textContent = isVisible ? '⚙️ Advanced Options' : '⚙️ Hide Advanced';
    });

    // Show info tooltip when hovering over Top-K heap checkbox
    document.getElementById('useTopKHeap').addEventListener('change', (e) => {
        if (e.target.checked) {
            showToast('✓ Top-K Heap enabled: 50-150x faster for small result sets', 'success');
        } else {
            showToast('ℹ️ Using full sort: Better for large result sets (K > 1000)', 'info');
        }
    });
    
    console.log('✅ Event listeners attached');
});
async function performSearch() {
    const query = document.getElementById('searchInput').value;
    const algorithm = document.querySelector('input[name="algorithm"]:checked').value;
    const maxResults = parseInt(document.getElementById('maxResults').value);
    const useTopKHeap = document.getElementById('useTopKHeap').checked;
    const fuzzySearch = true; // Always on in UI
    
    if (!query.trim()) {
        showToast('⚠️ Please enter a search query', 'warning');
        return;
    }
    
    // Show loading state
    const searchBtn = document.getElementById('searchButton');
    const btnText = searchBtn.querySelector('.btn-text');
    const btnIcon = searchBtn.querySelector('.btn-icon');
    searchBtn.disabled = true;
    btnText.textContent = 'Searching...';
    btnIcon.textContent = '⏳';
    
    const startTime = performance.now();
    
    try {
        let data;
        
        if (DEMO_MODE) {
            // Demo mode: simulate search
            data = performDemoSearch(query, algorithm, maxResults, useTopKHeap, fuzzySearch);
        } else {
            // Real API call - pass query as-is, server handles parsing
            let url = `${API_BASE}/search?q=${encodeURIComponent(query)}&algorithm=${algorithm}&max_results=${maxResults}&use_top_k_heap=${useTopKHeap}&highlight=true`;
            if (fuzzySearch) {
                url += '&fuzzy=true';
            }
            const response = await fetch(url);
            data = await response.json();
        }
        
        const endTime = performance.now();
        data.query_time_ms = endTime - startTime;
        data.use_top_k_heap = useTopKHeap;
        data.algorithm = algorithm;
        data.original_query = query;
        
        // Collect expanded terms from response
        const expanded = data.expanded_terms || 
            (data.results && data.results.length > 0 && data.results[0].expanded_terms) || {};
        
        displayResults(data.results, query, expanded);
        displayStats(data);
        
        // Show fuzzy expansion info if terms were corrected
        if (expanded && Object.keys(expanded).length > 0) {
            const corrections = Object.entries(expanded).map(([from, to]) => `"${from}" → "${to}"`).join(', ');
            showToast(`🔤 Fuzzy correction: ${corrections}`, 'info');
        }
        
        // Show success message
        const resultText = data.total_results === 1 ? 'result' : 'results';
        showToast(`✓ Found ${data.total_results} ${resultText} in ${data.query_time_ms.toFixed(2)}ms`, 'success');
    } catch (error) {
        console.error('Search error:', error);
        document.getElementById('results').innerHTML = 
            '<div class="error">❌ <strong>Connection Error</strong><br>Unable to connect to search server. Please ensure the server is running on port 8080.</div>';
        showToast('❌ Connection error', 'error');
    } finally {
        searchBtn.disabled = false;
        btnText.textContent = 'Search';
        btnIcon.textContent = '🚀';
    }
}

// Damerau-Levenshtein distance for demo mode fuzzy matching
function damerauLevenshtein(a, b) {
    const la = a.length, lb = b.length;
    if (la === 0) return lb;
    if (lb === 0) return la;
    const d = Array.from({ length: la + 1 }, () => new Array(lb + 1).fill(0));
    for (let i = 0; i <= la; i++) d[i][0] = i;
    for (let j = 0; j <= lb; j++) d[0][j] = j;
    for (let i = 1; i <= la; i++) {
        for (let j = 1; j <= lb; j++) {
            const cost = a[i - 1] === b[j - 1] ? 0 : 1;
            d[i][j] = Math.min(
                d[i - 1][j] + 1,       // deletion
                d[i][j - 1] + 1,       // insertion
                d[i - 1][j - 1] + cost  // substitution
            );
            if (i > 1 && j > 1 && a[i - 1] === b[j - 2] && a[i - 2] === b[j - 1]) {
                d[i][j] = Math.min(d[i][j], d[i - 2][j - 2] + cost); // transposition
            }
        }
    }
    return d[la][lb];
}

function performDemoSearch(query, algorithm, maxResults, useTopKHeap, fuzzySearch) {
    // Simple scoring: count matching terms
    const queryTerms = query.toLowerCase().split(/\s+/).filter(t => t.length > 0);
    const expandedTerms = {};
    
    // In demo mode, always enable fuzzy — no performance concern with small dataset
    const useFuzzy = true;
    
    // Build vocabulary from demo docs for fuzzy + prefix matching
    const vocabulary = new Set();
    DEMO_DOCUMENTS.forEach(doc => {
        doc.content.toLowerCase().split(/\s+/).forEach(t => vocabulary.add(t));
    });

    // Resolve each query term via: exact match → prefix match → fuzzy match
    const resolvedTerms = queryTerms.map(term => {
        // 1. Exact match in vocabulary
        if (vocabulary.has(term)) return term;
        
        // 2. Prefix match — "machin" → "machine"
        let prefixMatch = null;
        let shortestPrefix = Infinity;
        for (const vocabTerm of vocabulary) {
            if (vocabTerm.startsWith(term) && vocabTerm.length < shortestPrefix) {
                prefixMatch = vocabTerm;
                shortestPrefix = vocabTerm.length;
            }
        }
        if (prefixMatch) {
            expandedTerms[term] = prefixMatch;
            return prefixMatch;
        }
        
        // 3. Fuzzy match via edit distance
        let bestMatch = null;
        let bestDist = Infinity;
        const maxDist = term.length <= 2 ? 0 : term.length <= 4 ? 1 : 2;
        for (const vocabTerm of vocabulary) {
            const d = damerauLevenshtein(term, vocabTerm);
            if (d <= maxDist && d < bestDist) {
                bestDist = d;
                bestMatch = vocabTerm;
            }
        }
        if (bestMatch && bestMatch !== term) {
            expandedTerms[term] = bestMatch;
            return bestMatch;
        }
        return term;
    });
    
    const results = DEMO_DOCUMENTS.map(doc => {
        const docTerms = doc.content.toLowerCase().split(/\s+/);
        let score = 0;
        
        // Count matching terms: exact match, prefix match, and fuzzy match
        for (const term of resolvedTerms) {
            for (const docTerm of docTerms) {
                if (docTerm === term) {
                    score += 1.0;           // exact match
                } else if (docTerm.startsWith(term) || term.startsWith(docTerm)) {
                    score += 0.6;           // prefix match
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
        
        // Generate snippets around matching terms
        const snippets = generateDemoSnippets(doc.content, resolvedTerms, 120);

        return {
            document: doc,
            score: score,
            snippets: snippets
        };
    }).filter(r => r.score > 0)
      .sort((a, b) => b.score - a.score)
      .slice(0, maxResults);
    
    return {
        results: results,
        total_results: results.length,
        total_documents: DEMO_DOCUMENTS.length,
        expanded_terms: Object.keys(expandedTerms).length > 0 ? expandedTerms : undefined
    };
}

// Generate highlighted snippets around query term matches (demo mode)
function generateDemoSnippets(text, queryTerms, maxLen) {
    if (!text || !queryTerms.length) return [];
    // If text fits in one snippet, return highlighted full text
    if (text.length <= maxLen) {
        return [highlightTermsInText(text, queryTerms)];
    }
    const lowerText = text.toLowerCase();
    const words = lowerText.split(/\s+/);
    // Find first matching word position
    let bestPos = 0;
    for (const term of queryTerms) {
        const idx = lowerText.indexOf(term);
        if (idx !== -1) { bestPos = idx; break; }
    }
    // Center window around match
    let start = Math.max(0, bestPos - Math.floor(maxLen / 2));
    let end = Math.min(text.length, start + maxLen);
    // Snap to word boundaries
    if (start > 0) {
        const spaceIdx = text.indexOf(' ', start);
        if (spaceIdx !== -1 && spaceIdx < start + 20) start = spaceIdx + 1;
    }
    if (end < text.length) {
        const spaceIdx = text.lastIndexOf(' ', end);
        if (spaceIdx > end - 20) end = spaceIdx;
    }
    let snippet = text.substring(start, end);
    if (start > 0) snippet = '...' + snippet;
    if (end < text.length) snippet = snippet + '...';
    return [highlightTermsInText(snippet, queryTerms)];
}

// Highlight specific terms in text with <mark> tags
function highlightTermsInText(text, terms) {
    if (!terms.length) return escapeHtml(text);
    // Escape HTML first, then highlight
    let safe = escapeHtml(text);
    for (const term of terms) {
        if (term.length < 2) continue;
        const escaped = term.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
        const regex = new RegExp(`\\b(${escaped})`, 'gi');
        safe = safe.replace(regex, '<mark>$1</mark>');
    }
    return safe;
}

function displayResults(results, query, expandedTerms) {
    expandedTerms = expandedTerms || {};
    const resultsDiv = document.getElementById('results');
    
    if (!results || results.length === 0) {
        resultsDiv.innerHTML = `
            <div class="no-results">
                <div class="no-results-icon">🔍</div>
                <div class="no-results-title">No Results Found</div>
                <div class="no-results-text">Try different keywords or use OR operator to broaden your search</div>
            </div>
        `;
        return;
    }
    
    let html = '<div class="result-list">';
    results.forEach((result, index) => {
        // Prefer server/demo snippets, fall back to highlighted content
        let content;
        if (result.snippets && result.snippets.length > 0) {
            content = result.snippets.join(' ');
        } else {
            content = highlightQuery(result.document.content, query, expandedTerms);
        }
        const scorePercent = Math.min(100, (result.score / 10) * 100);
        const scoreClass = result.score > 5 ? 'high-score' : result.score > 2 ? 'medium-score' : 'low-score';
        
        html += `
            <div class="result-item" data-index="${index}">
                <div class="result-rank-container">
                    <div class="result-rank">${index + 1}</div>
                    <div class="result-score-badge ${scoreClass}">
                        <div class="score-circle">
                            <svg class="score-ring" width="50" height="50">
                                <circle cx="25" cy="25" r="20" fill="none" stroke="#e0e0e0" stroke-width="3"/>
                                <circle cx="25" cy="25" r="20" fill="none" stroke="currentColor" stroke-width="3"
                                    stroke-dasharray="${scorePercent * 1.256} ${125.6 - scorePercent * 1.256}"
                                    transform="rotate(-90 25 25)"/>
                            </svg>
                            <div class="score-text">${result.score.toFixed(1)}</div>
                        </div>
                    </div>
                </div>
                <div class="result-content">
                    <div class="result-header">
                        <div class="result-title">Document ${result.document.id}</div>
                        <div class="result-relevance ${scoreClass}">
                            ${result.score > 5 ? '🌟 Highly Relevant' : result.score > 2 ? '✓ Relevant' : '~ Possibly Relevant'}
                        </div>
                    </div>
                    <div class="result-text">${content}</div>
                    <div class="result-footer">
                        <span class="result-meta">📄 Doc ID: ${result.document.id}</span>
                        <span class="result-meta">⭐ Relevance: ${result.score.toFixed(4)}</span>
                        ${result.document.fields && result.document.fields.title ? 
                            `<span class="result-meta">📌 ${escapeHtml(result.document.fields.title)}</span>` : ''}
                    </div>
                </div>
            </div>
        `;
    });
    html += '</div>';
    
    resultsDiv.innerHTML = html;
    
    // Add click animation
    document.querySelectorAll('.result-item').forEach(item => {
        item.addEventListener('click', function() {
            this.style.transform = 'scale(0.98)';
            setTimeout(() => {
                this.style.transform = '';
            }, 100);
        });
    });
}

function highlightQuery(text, query, expandedTerms) {
    if (!query) return escapeHtml(text);
    expandedTerms = expandedTerms || {};
    
    // Collect both original terms and their fuzzy-expanded versions for highlighting
    const originalTerms = query.toLowerCase().split(/\s+/).filter(t => t.length > 2);
    const highlightSet = new Set();
    for (const term of originalTerms) {
        highlightSet.add(term);
        // If this term was fuzzy-expanded, also highlight the corrected version
        if (expandedTerms[term]) {
            highlightSet.add(expandedTerms[term].toLowerCase());
        }
    }
    // Also add any expanded values in case keys don't match original terms exactly
    for (const [orig, corrected] of Object.entries(expandedTerms)) {
        highlightSet.add(corrected.toLowerCase());
    }
    
    let highlighted = escapeHtml(text);
    
    for (const term of highlightSet) {
        const escaped = term.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
        const regex = new RegExp(`(${escaped})`, 'gi');
        highlighted = highlighted.replace(regex, '<mark>$1</mark>');
    }
    
    return highlighted;
}

function displayStats(data) {
    const statsDiv = document.getElementById('stats');
    const heapIcon = data.use_top_k_heap ? '🚀' : '📈';
    const algorithmIcon = data.algorithm === 'bm25' ? '🎯' : '📐';
    const speedClass = data.query_time_ms < 10 ? 'super-fast' : data.query_time_ms < 50 ? 'fast' : 'normal';
    
    statsDiv.innerHTML = `
        <div class="stats-content">
            <div class="stat-item stat-results">
                <div class="stat-icon-container">
                    <div class="stat-icon">📊</div>
                </div>
                <div class="stat-info">
                    <div class="stat-value">${data.total_results}</div>
                    <div class="stat-label">Results Found</div>
                </div>
            </div>
            <div class="stat-item stat-time ${speedClass}">
                <div class="stat-icon-container">
                    <div class="stat-icon">⚡</div>
                </div>
                <div class="stat-info">
                    <div class="stat-value">${data.query_time_ms.toFixed(2)}ms</div>
                    <div class="stat-label">Query Time</div>
                </div>
            </div>
            <div class="stat-item stat-algorithm">
                <div class="stat-icon-container">
                    <div class="stat-icon">${algorithmIcon}</div>
                </div>
                <div class="stat-info">
                    <div class="stat-value">${data.algorithm?.toUpperCase() || 'BM25'}</div>
                    <div class="stat-label">Algorithm</div>
                </div>
            </div>
            <div class="stat-item stat-method">
                <div class="stat-icon-container">
                    <div class="stat-icon">${heapIcon}</div>
                </div>
                <div class="stat-info">
                    <div class="stat-value">${data.use_top_k_heap ? 'Heap' : 'Sort'}</div>
                    <div class="stat-label">Method</div>
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

function showToast(message, type = 'info') {
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.textContent = message;
    document.body.appendChild(toast);
    
    setTimeout(() => {
        toast.classList.add('show');
    }, 10);
    
    setTimeout(() => {
        toast.classList.remove('show');
        setTimeout(() => {
            document.body.removeChild(toast);
        }, 300);
    }, 3000);
}

// Index stats display removed
