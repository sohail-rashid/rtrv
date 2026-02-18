'use strict';

const CONFIG = {
    apiBase: window.location.origin,
    requestTimeoutMs: 4000,
    enableFuzzy: true,
    enableHighlight: true,
    defaultMaxResults: 10,
    statsRefreshMs: 15000
};

const dom = {};
let currentView = 'search';
let statsInterval = null;

function cacheDom() {
    // Search view
    dom.searchInput = document.getElementById('searchInput');
    dom.searchButton = document.getElementById('searchButton');
    dom.clearButton = document.getElementById('clearButton');
    dom.results = document.getElementById('results');
    dom.stats = document.getElementById('stats');
    dom.advancedToggle = document.getElementById('advancedToggle');
    dom.advancedOptions = document.getElementById('advancedOptions');
    dom.useTopKHeap = document.getElementById('useTopKHeap');
    dom.maxResults = document.getElementById('maxResults');

    // Views
    dom.searchView = document.getElementById('searchView');
    dom.indexView = document.getElementById('indexView');

    // Nav pills
    dom.navPills = document.querySelectorAll('.nav-pill');

    // Index view
    dom.statTotalDocs = document.getElementById('statTotalDocs');
    dom.statTotalTerms = document.getElementById('statTotalTerms');
    dom.statAvgLen = document.getElementById('statAvgLen');
    dom.statCacheRate = document.getElementById('statCacheRate');
    dom.cacheHits = document.getElementById('cacheHits');
    dom.cacheMisses = document.getElementById('cacheMisses');
    dom.cacheEvictions = document.getElementById('cacheEvictions');
    dom.cacheSize = document.getElementById('cacheSize');
    dom.clearCacheBtn = document.getElementById('clearCacheBtn');
    dom.docIdInput = document.getElementById('docIdInput');
    dom.docContentInput = document.getElementById('docContentInput');
    dom.addDocBtn = document.getElementById('addDocBtn');
    dom.deleteDocIdInput = document.getElementById('deleteDocIdInput');
    dom.deleteDocBtn = document.getElementById('deleteDocBtn');
}

function init() {
    cacheDom();
    attachEventListeners();
}

function attachEventListeners() {
    dom.searchButton.addEventListener('click', performSearch);
    dom.searchInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
            performSearch();
        }
    });
    dom.clearButton.addEventListener('click', clearSearch);

    if (dom.advancedToggle && dom.advancedOptions) {
        dom.advancedToggle.addEventListener('click', () => {
            const isVisible = dom.advancedOptions.style.display !== 'none';
            dom.advancedOptions.style.display = isVisible ? 'none' : 'block';
            dom.advancedToggle.textContent = isVisible ? 'Close Advanced Settings' : 'Advanced Settings';
        });
    }

    // Nav pill tab switching (use data-view attribute)
    dom.navPills.forEach(pill => {
        pill.addEventListener('click', () => {
            const view = pill.getAttribute('data-view') || pill.textContent.trim().toLowerCase();
            switchView(view === 'index' ? 'index' : 'search');
        });
    });

    // Index view actions
    if (dom.addDocBtn) dom.addDocBtn.addEventListener('click', addDocument);
    if (dom.deleteDocBtn) dom.deleteDocBtn.addEventListener('click', deleteDocument);
    if (dom.clearCacheBtn) dom.clearCacheBtn.addEventListener('click', clearCache);
}

function clearSearch() {
    dom.searchInput.value = '';
    dom.results.innerHTML = `
        <div class="empty-state">
            <div class="empty-icon">
                <svg width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>
            </div>
            <h3>Ready to search</h3>
            <p>Enter a query above to search through the index.</p>
        </div>
    `;
    dom.stats.innerHTML = '';
}

async function performSearch() {
    const rawQuery = dom.searchInput.value.trim();
    const algorithm = getSelectedAlgorithm();
    const maxResults = parsePositiveInt(dom.maxResults.value, CONFIG.defaultMaxResults);
    const useTopKHeap = dom.useTopKHeap ? dom.useTopKHeap.checked : true;

    setSearchLoading(true);
    const startTime = performance.now();

    try {
        let data;

        if (!rawQuery) {
            // Empty query => list documents from /documents endpoint
            const url = `${CONFIG.apiBase}/documents?offset=0&limit=${maxResults}`;
            const resp = await fetchWithTimeout(url, CONFIG.requestTimeoutMs);
            data = await resp.json();
        } else {
            data = await fetchSearchResults(rawQuery, algorithm, maxResults, useTopKHeap);
        }

        const endTime = performance.now();
        data.query_time_ms = endTime - startTime;
        data.use_top_k_heap = useTopKHeap;
        data.algorithm = algorithm;
        data.original_query = rawQuery || 'All documents';

        const expanded = data.expanded_terms ||
            (data.results && data.results.length > 0 && data.results[0].expanded_terms) || {};

        displayResults(data.results, rawQuery, expanded);
        displayStats(data);

        if (rawQuery && expanded && Object.keys(expanded).length > 0) {
            const corrections = Object.entries(expanded)
                .map(([from, to]) => `"${from}" -> "${to}"`)
                .join(', ');
            showToast(`Applied fuzzy corrections: ${corrections}`, 'info');
        }

    } catch (error) {
        console.error('Search error:', error);
        dom.results.innerHTML =
            '<div class="error"><strong>Connection Error</strong><br>Unable to connect to search server.</div>';
        showToast('Connection error. Please check the server.', 'error');
    } finally {
        setSearchLoading(false);
    }
}

async function fetchSearchResults(query, algorithm, maxResults, useTopKHeap) {
    const params = new URLSearchParams({
        q: query,
        algorithm: algorithm,
        max_results: String(maxResults),
        use_top_k_heap: String(Boolean(useTopKHeap))
    });

    if (CONFIG.enableHighlight) {
        params.set('highlight', 'true');
    }

    if (CONFIG.enableFuzzy) {
        params.set('fuzzy', 'true');
    }

    const url = `${CONFIG.apiBase}/search?${params.toString()}`;
    const response = await fetchWithTimeout(url, CONFIG.requestTimeoutMs);
    return response.json();
}

function setSearchLoading(isLoading) {
    dom.searchButton.disabled = isLoading;
    dom.searchButton.textContent = isLoading ? 'Searching...' : 'Search';
}

function getSelectedAlgorithm() {
    const selected = document.querySelector('input[name="algorithm"]:checked');
    return selected ? selected.value : 'bm25';
}

function parsePositiveInt(value, fallback) {
    const parsed = parseInt(value, 10);
    if (Number.isNaN(parsed) || parsed < 1) {
        return fallback;
    }
    return parsed;
}

async function fetchWithTimeout(url, timeoutMs, options = {}) {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), timeoutMs);
    try {
        return await fetch(url, { ...options, signal: controller.signal });
    } finally {
        clearTimeout(timeoutId);
    }
}

function displayResults(results, query, expandedTerms) {
    expandedTerms = expandedTerms || {};

    if (!results || results.length === 0) {
        dom.results.innerHTML = `
            <div class="empty-state">
                <div class="empty-icon">
                    <svg width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>
                </div>
                <h3>No results found</h3>
                <p>We couldn't find anything matching "${escapeHtml(query)}". Try different keywords.</p>
            </div>
        `;
        return;
    }

    let html = '';
    results.forEach((result, index) => {
        let content = result.snippets && result.snippets.length > 0
            ? result.snippets.join(' ')
            : result.document.content; // Fallback to content if no snippets
        
        // If we didn't get snippets from backend but have content, highlight it manually
        if ((!result.snippets || result.snippets.length === 0) && content) {
             content = highlightQuery(content, query, expandedTerms);
        }

        const scoreClass = result.score > 5 ? 'score-high' : result.score > 2 ? 'score-med' : 'score-low';
        const relevanceLabel = result.score > 5 ? 'High Relevance' : result.score > 2 ? 'Medium Relevance' : 'Low Relevance';
        const docTitle = (result.document.fields && result.document.fields.title) ? result.document.fields.title : `Document ${result.document.id}`;

        html += `
            <div class="result-card" onclick="console.log('Clicked doc ${result.document.id}')">
                <div class="card-header">
                    <a href="#" class="card-title">${escapeHtml(docTitle)}</a>
                    <span class="relevance-score ${scoreClass}">${relevanceLabel}</span>
                </div>
                <div class="card-meta">
                    <span class="rank-badge">#${index + 1}</span>
                    <span>ID: ${result.document.id}</span>
                    <span>•</span>
                    <span>Score: ${result.score.toFixed(4)}</span>
                </div>
                <div class="card-snippet">${content}</div>
            </div>
        `;
    });

    dom.results.innerHTML = html;
}

function highlightQuery(text, query, expandedTerms) {
    if (!query) return escapeHtml(text);
    expandedTerms = expandedTerms || {};
    
    // Naive highlighting for client-side fallback
    const terms = query.toLowerCase().split(/\s+/).filter(t => t.length > 2);
    
    // Add extended terms
     for (const corrected of Object.values(expandedTerms)) {
        terms.push(corrected.toLowerCase());
    }

    let escapedText = escapeHtml(text);
    
    if (terms.length === 0) return escapedText;

    const pattern = new RegExp(`(${terms.map(t => t.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')).join('|')})`, 'gi');
    return escapedText.replace(pattern, '<mark>$1</mark>');
}

function displayStats(data) {
    const heapText = data.use_top_k_heap ? 'Heap Optimization' : 'Standard Sort';
    const algorithmText = data.algorithm.toUpperCase();

    dom.stats.innerHTML = `
        <div class="stats-header">
             <div class="stats-query-info">
                <span class="query-label">Results for</span>
                <span class="query-text">"${escapeHtml(data.original_query)}"</span>
            </div>
            <div class="stats-container">
                <div class="stat-pill">
                    <span>found</span>
                    <span class="stat-value-bold">${data.total_results}</span>
                    <span>results</span>
                </div>
                <div class="stat-pill">
                    <span>in</span>
                    <span class="stat-value-bold">${data.query_time_ms.toFixed(2)}ms</span>
                </div>
                 <div class="stat-pill">
                    <span>using</span>
                    <span class="stat-value-bold">${algorithmText}</span>
                </div>
            </div>
        </div>
    `;
}

function escapeHtml(text) {
    if (!text) return '';
    return text
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}

/* ── View Switching ─────────────────────────────────── */

function switchView(view) {
    currentView = view;

    // Update nav pills using data-view attribute
    dom.navPills.forEach(pill => {
        const pillView = pill.getAttribute('data-view') || pill.textContent.trim().toLowerCase();
        pill.classList.toggle('active', pillView === view);
    });

    // Toggle view panels
    if (dom.searchView) dom.searchView.classList.toggle('active', view === 'search');
    if (dom.indexView) dom.indexView.classList.toggle('active', view === 'index');

    // When switching to index, load stats
    if (view === 'index') {
        loadIndexStats();
        loadCacheStats();
        // Auto-refresh while on index view
        if (statsInterval) clearInterval(statsInterval);
        statsInterval = setInterval(() => {
            if (currentView === 'index') {
                loadIndexStats();
                loadCacheStats();
            }
        }, CONFIG.statsRefreshMs);
    } else {
        if (statsInterval) { clearInterval(statsInterval); statsInterval = null; }
    }
}

/* ── Index View: Stats ──────────────────────────────── */

async function loadIndexStats() {
    try {
        const resp = await fetchWithTimeout(`${CONFIG.apiBase}/stats`, CONFIG.requestTimeoutMs);
        const data = await resp.json();
        animateValue(dom.statTotalDocs, data.total_documents);
        animateValue(dom.statTotalTerms, data.total_terms);
        dom.statAvgLen.textContent = typeof data.avg_doc_length === 'number'
            ? data.avg_doc_length.toFixed(1)
            : '—';
    } catch (e) {
        console.error('Failed to load index stats', e);
    }
}

async function loadCacheStats() {
    try {
        const resp = await fetchWithTimeout(`${CONFIG.apiBase}/cache/stats`, CONFIG.requestTimeoutMs);
        const data = await resp.json();

        dom.cacheHits.textContent = formatNumber(data.hit_count);
        dom.cacheMisses.textContent = formatNumber(data.miss_count);
        dom.cacheEvictions.textContent = formatNumber(data.eviction_count);
        dom.cacheSize.textContent = `${data.current_size} / ${data.max_size}`;

        const rate = typeof data.hit_rate === 'number' ? (data.hit_rate * 100).toFixed(1) + '%' : '—';
        dom.statCacheRate.textContent = rate;
    } catch (e) {
        console.error('Failed to load cache stats', e);
    }
}

function animateValue(el, target) {
    if (!el) return;
    const current = parseInt(el.textContent, 10);
    if (isNaN(current) || current === target) {
        el.textContent = formatNumber(target);
        return;
    }
    const duration = 400;
    const start = performance.now();
    const from = current;
    function tick(now) {
        const p = Math.min((now - start) / duration, 1);
        const ease = 1 - Math.pow(1 - p, 3);
        el.textContent = formatNumber(Math.round(from + (target - from) * ease));
        if (p < 1) requestAnimationFrame(tick);
    }
    requestAnimationFrame(tick);
}

function formatNumber(n) {
    if (typeof n !== 'number' || isNaN(n)) return '—';
    return n.toLocaleString();
}

/* ── Index View: Actions ────────────────────────────── */

async function addDocument() {
    const id = parseInt(dom.docIdInput.value, 10);
    const content = dom.docContentInput.value.trim();

    if (isNaN(id) || id < 1) { showToast('Enter a valid document ID.', 'warning'); return; }
    if (!content) { showToast('Enter document content.', 'warning'); return; }

    dom.addDocBtn.disabled = true;
    dom.addDocBtn.textContent = 'Indexing...';

    try {
        const resp = await fetchWithTimeout(`${CONFIG.apiBase}/index`, CONFIG.requestTimeoutMs, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id, content })
        });
        const data = await resp.json();

        if (data.success) {
            showToast(`Document ${data.doc_id} indexed successfully.`, 'success');
            dom.docIdInput.value = '';
            dom.docContentInput.value = '';
            loadIndexStats();
        } else {
            showToast(data.error || 'Failed to index document.', 'error');
        }
    } catch (e) {
        showToast('Server error while indexing.', 'error');
    } finally {
        dom.addDocBtn.disabled = false;
        dom.addDocBtn.textContent = 'Add to Index';
    }
}

async function deleteDocument() {
    const id = parseInt(dom.deleteDocIdInput.value, 10);
    if (isNaN(id) || id < 1) { showToast('Enter a valid document ID.', 'warning'); return; }

    dom.deleteDocBtn.disabled = true;
    dom.deleteDocBtn.textContent = 'Deleting...';

    try {
        const resp = await fetchWithTimeout(`${CONFIG.apiBase}/delete/${id}`, CONFIG.requestTimeoutMs, {
            method: 'DELETE'
        });
        const data = await resp.json();

        if (data.success) {
            showToast(`Document ${id} deleted.`, 'success');
            dom.deleteDocIdInput.value = '';
            loadIndexStats();
        } else {
            showToast(`Document ${id} not found.`, 'warning');
        }
    } catch (e) {
        showToast('Server error while deleting.', 'error');
    } finally {
        dom.deleteDocBtn.disabled = false;
        dom.deleteDocBtn.textContent = 'Delete';
    }
}

async function clearCache() {
    dom.clearCacheBtn.disabled = true;
    try {
        const resp = await fetchWithTimeout(`${CONFIG.apiBase}/cache`, CONFIG.requestTimeoutMs, {
            method: 'DELETE'
        });
        const data = await resp.json();
        if (data.success) {
            showToast('Cache cleared.', 'success');
            loadCacheStats();
        }
    } catch (e) {
        showToast('Failed to clear cache.', 'error');
    } finally {
        dom.clearCacheBtn.disabled = false;
    }
}

function showToast(message, type = 'info') {
    // Remove existing toast if any
    const existing = document.querySelector('.toast');
    if (existing) existing.remove();

    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.textContent = message;
    document.body.appendChild(toast);

    // Trigger reflow
    toast.offsetHeight;

    toast.classList.add('show');

    setTimeout(() => {
        toast.classList.remove('show');
        setTimeout(() => {
            if (toast.parentNode) toast.parentNode.removeChild(toast);
        }, 300);
    }, 3000);
}

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
