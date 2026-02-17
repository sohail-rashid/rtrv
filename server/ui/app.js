'use strict';

const CONFIG = {
    apiBase: window.location.origin,
    requestTimeoutMs: 4000,
    enableFuzzy: true,
    enableHighlight: true,
    defaultMaxResults: 10
};

const dom = {};

function cacheDom() {
    dom.searchInput = document.getElementById('searchInput');
    dom.searchButton = document.getElementById('searchButton');
    dom.clearButton = document.getElementById('clearButton');
    dom.results = document.getElementById('results');
    dom.stats = document.getElementById('stats');
    dom.advancedToggle = document.getElementById('advancedToggle');
    dom.advancedOptions = document.getElementById('advancedOptions');
    dom.useTopKHeap = document.getElementById('useTopKHeap');
    dom.maxResults = document.getElementById('maxResults');
}

function init() {
    cacheDom();
    attachEventListeners();
    initBackgroundAnimation();
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
            dom.advancedToggle.textContent = isVisible ? '⚙️ Advanced Options' : '⚙️ Hide Advanced';
        });
    }
}

function clearSearch() {
    dom.searchInput.value = '';
    dom.results.innerHTML = '';
    dom.stats.innerHTML = '';
}

async function performSearch() {
    const query = dom.searchInput.value.trim();
    const algorithm = getSelectedAlgorithm();
    const maxResults = parsePositiveInt(dom.maxResults.value, CONFIG.defaultMaxResults);
    const useTopKHeap = dom.useTopKHeap ? dom.useTopKHeap.checked : true;

    if (!query) {
        showToast('Please enter a search query.', 'warning');
        return;
    }

    setSearchLoading(true);
    const startTime = performance.now();

    try {
        const data = await fetchSearchResults(query, algorithm, maxResults, useTopKHeap);

        const endTime = performance.now();
        data.query_time_ms = endTime - startTime;
        data.use_top_k_heap = useTopKHeap;
        data.algorithm = algorithm;
        data.original_query = query;

        const expanded = data.expanded_terms ||
            (data.results && data.results.length > 0 && data.results[0].expanded_terms) || {};

        displayResults(data.results, query, expanded);
        displayStats(data);

        if (expanded && Object.keys(expanded).length > 0) {
            const corrections = Object.entries(expanded)
                .map(([from, to]) => `"${from}" -> "${to}"`)
                .join(', ');
            showToast(`🔤 Applied fuzzy corrections: ${corrections}`, 'info');
        }

        const resultText = data.total_results === 1 ? 'result' : 'results';
        showToast(`✅ Found ${data.total_results} ${resultText} in ${data.query_time_ms.toFixed(2)}ms.`, 'success');
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
    const btnText = dom.searchButton.querySelector('.btn-text');
    const btnIcon = dom.searchButton.querySelector('.btn-icon');
    dom.searchButton.disabled = isLoading;
    btnText.textContent = isLoading ? 'Searching...' : 'Search';
    btnIcon.textContent = isLoading ? '⏳' : '🚀';
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

async function fetchWithTimeout(url, timeoutMs) {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), timeoutMs);
    try {
        return await fetch(url, { signal: controller.signal });
    } finally {
        clearTimeout(timeoutId);
    }
}

function displayResults(results, query, expandedTerms) {
    expandedTerms = expandedTerms || {};

    if (!results || results.length === 0) {
        dom.results.innerHTML = [
            '<div class="no-results">',
            '  <div class="no-results-icon">No results</div>',
            '  <div class="no-results-title">No results found</div>',
            '  <div class="no-results-text">Try different keywords or broaden your query.</div>',
            '</div>'
        ].join('');
        return;
    }

    let html = '<div class="result-list">';
    results.forEach((result, index) => {
        const content = result.snippets && result.snippets.length > 0
            ? result.snippets.join(' ')
            : highlightQuery(result.document.content, query, expandedTerms);

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
                            ${result.score > 5 ? 'Highly Relevant' : result.score > 2 ? 'Relevant' : 'Possibly Relevant'}
                        </div>
                    </div>
                    <div class="result-text">${content}</div>
                    <div class="result-footer">
                        <span class="result-meta">Doc ID: ${result.document.id}</span>
                        <span class="result-meta">Score: ${result.score.toFixed(4)}</span>
                        ${result.document.fields && result.document.fields.title ?
                            `<span class="result-meta">${escapeHtml(result.document.fields.title)}</span>` : ''}
                    </div>
                </div>
            </div>
        `;
    });
    html += '</div>';

    dom.results.innerHTML = html;
}

function highlightQuery(text, query, expandedTerms) {
    if (!query) return escapeHtml(text);
    expandedTerms = expandedTerms || {};

    const originalTerms = query.toLowerCase().split(/\s+/).filter(t => t.length > 2);
    const highlightSet = new Set();

    for (const term of originalTerms) {
        highlightSet.add(term);
        if (expandedTerms[term]) {
            highlightSet.add(expandedTerms[term].toLowerCase());
        }
    }

    for (const corrected of Object.values(expandedTerms)) {
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
    const heapIcon = data.use_top_k_heap ? 'Heap' : 'Sort';
    const algorithmIcon = data.algorithm === 'bm25' ? 'BM25' : 'TF-IDF';
    const speedClass = data.query_time_ms < 10 ? 'super-fast' : data.query_time_ms < 50 ? 'fast' : 'normal';

    dom.stats.innerHTML = `
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
                    <div class="stat-icon">🎯</div>
                </div>
                <div class="stat-info">
                    <div class="stat-value">${algorithmIcon}</div>
                    <div class="stat-label">Ranking</div>
                </div>
            </div>
            <div class="stat-item stat-method">
                <div class="stat-icon-container">
                    <div class="stat-icon">🚀</div>
                </div>
                <div class="stat-info">
                    <div class="stat-value">${heapIcon}</div>
                    <div class="stat-label">Retrieval</div>
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

function initBackgroundAnimation() {
    const nodeCount = 18;
    const lineCount = 14;
    const nodes = [];

    for (let i = 0; i < nodeCount; i++) {
        const node = document.createElement('div');
        node.className = 'network-node';

        const startX = Math.random() * window.innerWidth;
        const startY = Math.random() * window.innerHeight;
        const moveX = (Math.random() - 0.5) * 120;
        const moveY = (Math.random() - 0.5) * 120;
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

    for (let i = 0; i < lineCount; i++) {
        const line = document.createElement('div');
        line.className = 'network-line';

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
}

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
