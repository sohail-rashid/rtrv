# Rtrv Search Engine — Feature Roadmap

> **Last Updated:** February 17, 2026
>
> This document defines 10 features to be implemented in the Rtrv search engine. Each feature includes a description, motivation, detailed requirements, affected components, acceptance criteria, and estimated effort. Features are ordered by priority (P0 = highest).

---

## Table of Contents

1. [Feature 1: Search Result Highlighting & Snippet Generation](#feature-1-search-result-highlighting--snippet-generation)
2. [Feature 2: Fuzzy Search & Typo Tolerance](#feature-2-fuzzy-search--typo-tolerance)
3. [Feature 3: Query Result Caching (LRU Cache)](#feature-3-query-result-caching-lru-cache)
4. [Feature 4: Pagination & Cursor-Based Navigation](#feature-4-pagination--cursor-based-navigation)
5. [Feature 5: Document Field Boosting](#feature-5-document-field-boosting)
6. [Feature 6: Auto-Complete & Type-Ahead Suggestions](#feature-6-auto-complete--type-ahead-suggestions)
7. [Feature 7: Synonym Expansion & Query Rewriting](#feature-7-synonym-expansion--query-rewriting)
8. [Feature 8: Faceted Search & Aggregations](#feature-8-faceted-search--aggregations)
9. [Feature 9: Wildcard & Regex Search](#feature-9-wildcard--regex-search)
10. [Feature 10: Index Compression (Variable Byte Encoding)](#feature-10-index-compression-variable-byte-encoding)

---

## Feature 1: Search Result Highlighting & Snippet Generation

**Priority:** P0 — Critical
**Estimated Effort:** Medium (3–5 days)

### Description

Generate context-aware text snippets from matched documents with query term highlighting. When a user searches for `"machine learning"`, each result should include a short excerpt (snippet) of the document content surrounding the matched terms, with matched terms wrapped in highlight markers (e.g., `<em>machine</em> <em>learning</em>`).

### Motivation

The engine already tracks token positions and character offsets via the `Token` struct (`start_offset`, `end_offset`) in the tokenizer, but this data is never surfaced to the user. Results currently return the full document content, making it hard for users to see *why* a document matched. Every production search engine (Elasticsearch, Solr, Algolia) provides highlighting — it is a table-stakes feature.

### Detailed Requirements

1. **Snippet Extractor Class** (`include/snippet_extractor.hpp`, `src/snippet_extractor.cpp`)
   - Accept a document's raw text, a list of matched terms, and configuration options.
   - Scan the document to find windows of text that contain the highest density of query terms.
   - Return one or more snippets (configurable, default: 3 snippets per document).
   - Each snippet should be a maximum of N characters (configurable, default: 150 chars).
   - Snippets should break on word boundaries, not mid-word.
   - Prepend/append ellipsis (`...`) when the snippet does not start at the beginning or end at the end of the document.

2. **Highlight Markers**
   - Wrap matched terms with configurable open/close tags (default: `<em>` / `</em>`).
   - Support plain-text markers as well (e.g., `**term**` for Markdown).
   - Highlighting must be case-insensitive while preserving the original case of the document text.

3. **Integration with SearchResult**
   - Add a `std::vector<std::string> snippets` field to the `SearchResult` struct.
   - Add a `bool generate_snippets` flag to `SearchOptions` (default: `false` for backward compatibility).
   - When enabled, populate snippets during the search flow, after scoring.

4. **REST API Changes**
   - Add `highlight=true` query parameter to `/search` endpoint.
   - Add `snippet_length` and `num_snippets` optional parameters.
   - Return snippets in the JSON response under `"snippets": [...]` per result.

5. **Performance Constraints**
   - Snippet generation must add no more than 10% overhead to query latency.
   - Use the existing position data from the tokenizer — do not re-tokenize the document.

### Affected Components

| Component | Change Type |
|---|---|
| `SearchResult` struct | Add `snippets` field |
| `SearchOptions` struct | Add `generate_snippets`, `snippet_length`, `num_snippets` |
| New: `SnippetExtractor` class | Create |
| `SearchEngine::search()` | Call snippet extractor post-scoring |
| `rest_server_drogon.cpp` | Parse highlight params, include snippets in response |
| `ui/` | Render highlighted snippets in results |

### Acceptance Criteria

- [ ] Snippets correctly surround matched terms with configurable markers.
- [ ] Multi-term queries highlight all matched terms in each snippet.
- [ ] Snippets break on word boundaries and respect max length.
- [ ] Performance benchmark shows ≤10% latency increase with highlighting enabled.
- [ ] REST API returns snippets when `highlight=true`.
- [ ] Unit tests cover: single term, multi-term, phrase, edge cases (term at start/end, very short documents).
- [ ] Web UI renders highlighted snippets with `<em>` styling.

---

## Feature 2: Fuzzy Search & Typo Tolerance

**Priority:** P0 — Critical
**Estimated Effort:** Large (5–8 days)

### Description

Allow the search engine to return relevant results even when the user's query contains typographical errors. A search for `"machne lerning"` should still return documents about "machine learning" by matching terms within an edit distance threshold.

### Motivation

Users frequently misspell search queries. Without fuzzy matching, misspelled queries return zero results, creating a poor user experience. This is a core feature of modern search engines (Elasticsearch's `fuzziness` parameter, Lucene's `FuzzyQuery`).

### Detailed Requirements

1. **Edit Distance Calculator** (`include/fuzzy_search.hpp`, `src/fuzzy_search.cpp`)
   - Implement Damerau-Levenshtein distance (supports transposition, insertion, deletion, substitution).
   - Optimize with a dynamic programming approach bounded by max edit distance (early termination).
   - Support configurable max edit distance per term (default: 2 for terms ≥5 chars, 1 for terms 3–4 chars, 0 for terms ≤2 chars).

2. **N-Gram Index for Candidate Generation**
   - Build a secondary index mapping character n-grams (bigrams or trigrams) to terms in the vocabulary.
   - On a fuzzy query, decompose the misspelled term into n-grams, use the n-gram index to fetch candidate terms, then filter candidates by edit distance.
   - This avoids an O(V) scan of the vocabulary for every query term (V = vocabulary size).

3. **Integration with Query Processing**
   - Add a `bool fuzzy_enabled` flag to `SearchOptions` (default: `false`).
   - Add a `uint8_t max_edit_distance` field to `SearchOptions` (default: `2`).
   - When fuzzy is enabled and a query term has zero exact matches in the inverted index, fall back to fuzzy matching.
   - Expand the query with fuzzy-matched terms, applying a scoring penalty proportional to edit distance.

4. **REST API Changes**
   - Add `fuzzy=true` and `max_edit_distance=N` query parameters to `/search`.
   - Return the corrected/expanded terms in the response as `"expanded_terms": {"machne": "machine"}`.

5. **Performance Constraints**
   - N-gram candidate generation must complete in under 1ms for vocabularies up to 500K terms.
   - Fuzzy matching should add no more than 50% overhead to query latency.

### Affected Components

| Component | Change Type |
|---|---|
| New: `FuzzySearch` class | Create (edit distance + n-gram index) |
| `SearchOptions` struct | Add `fuzzy_enabled`, `max_edit_distance` |
| `SearchEngine::search()` | Integrate fuzzy term expansion |
| `InvertedIndex` | Expose vocabulary iterator for n-gram index build |
| `rest_server_drogon.cpp` | Parse fuzzy params, return expanded terms |

### Acceptance Criteria

- [ ] `"machne"` matches `"machine"` with edit distance 1.
- [ ] `"lerning"` matches `"learning"` with edit distance 1.
- [ ] Transpositions are handled (`"teh"` → `"the"`).
- [ ] N-gram index is built on first fuzzy query or at engine startup.
- [ ] Edit distance threshold auto-scales based on term length.
- [ ] Fuzzy results are scored with a penalty relative to edit distance.
- [ ] Unit tests cover: substitution, deletion, insertion, transposition, max distance boundary.
- [ ] Benchmark shows fuzzy overhead ≤50% on 100K document corpus.

---

## Feature 3: Query Result Caching (LRU Cache)

**Priority:** P0 — Critical
**Estimated Effort:** Medium (2–4 days)

### Description

Implement a thread-safe Least Recently Used (LRU) cache that stores recent search results keyed by the normalized query string + search options. Identical repeated queries return cached results in O(1) without re-executing the search pipeline.

### Motivation

In production workloads, a significant portion of queries are repeated (Zipf distribution). Caching eliminates redundant computation for hot queries, directly improving P50/P99 latency and query throughput (QPS). The design document mentions a target of >2000 QPS — caching is the most cost-effective way to achieve this.

### Detailed Requirements

1. **LRU Cache Implementation** (`include/query_cache.hpp`, `src/query_cache.cpp`)
   - Implement using `std::unordered_map` + doubly-linked list for O(1) get/put.
   - Cache key: normalized query string + hash of `SearchOptions` (ranker, max_results, etc.).
   - Cache value: `std::vector<SearchResult>` + timestamp.
   - Configurable maximum cache size (default: 1024 entries).
   - Configurable TTL (time-to-live) per entry (default: 60 seconds).
   - Thread-safe using `std::shared_mutex` (read-shared, write-exclusive).

2. **Cache Invalidation**
   - Invalidate the entire cache on any index mutation (document add/update/delete).
   - Provide a `clearCache()` method on `SearchEngine`.
   - Support selective invalidation by query prefix (optional, stretch goal).

3. **Cache Statistics**
   - Track and expose: hit count, miss count, hit rate, eviction count, current size.
   - Add cache stats to the `IndexStatistics` struct or a new `CacheStatistics` struct.

4. **Integration with SearchEngine**
   - Check cache before executing the search pipeline.
   - Store results in cache after successful search execution.
   - Add `bool use_cache` flag to `SearchOptions` (default: `true`).

5. **REST API Changes**
   - Add `cache=false` parameter to bypass cache for a specific query.
   - Add `GET /cache/stats` endpoint returning hit/miss/eviction counts.
   - Add `DELETE /cache` endpoint to manually flush the cache.

### Affected Components

| Component | Change Type |
|---|---|
| New: `QueryCache` class | Create |
| `SearchEngine` | Add cache member, check/populate on search |
| `SearchEngine::indexDocument/update/delete` | Invalidate cache |
| `SearchOptions` struct | Add `use_cache` |
| `IndexStatistics` or new `CacheStatistics` | Add cache metrics |
| `rest_server_drogon.cpp` | Add `/cache/stats`, `DELETE /cache`, cache param |

### Acceptance Criteria

- [ ] Second identical query returns in <0.01ms (cache hit).
- [ ] Cache respects max size and evicts LRU entries.
- [ ] Cache is invalidated on document mutations.
- [ ] TTL expiration correctly evicts stale entries.
- [ ] Thread-safety validated under concurrent load (benchmark with multiple threads).
- [ ] Cache stats are accurate and exposed via REST API.
- [ ] Unit tests cover: hit, miss, eviction, TTL expiry, invalidation, thread safety.

---

## Feature 4: Pagination & Cursor-Based Navigation

**Priority:** P1 — High
**Estimated Effort:** Small (1–2 days)

### Description

Add pagination support to search results, allowing clients to request results in pages (e.g., results 11–20) using offset-based or cursor-based navigation. Currently, the engine only supports a `max_results` parameter with no way to access results beyond the first page.

### Motivation

Any real-world search UI needs pagination. Returning all results at once is impractical for large result sets. The current API forces clients to request a large `max_results` and slice client-side, which is wasteful. Cursor-based pagination also enables efficient "load more" patterns in the web UI.

### Detailed Requirements

1. **Offset-Based Pagination**
   - Add `size_t offset` field to `SearchOptions` (default: 0).
   - `offset=20, max_results=10` returns results ranked 21–30.
   - The engine should score all candidates but only materialize results in the requested window.

2. **Cursor-Based Pagination (Search After)**
   - Add `std::optional<double> search_after_score` and `std::optional<uint64_t> search_after_id` to `SearchOptions`.
   - The cursor is the `(score, doc_id)` tuple of the last result on the previous page.
   - Results are returned starting after the cursor position.
   - More efficient than offset for deep pagination (no need to re-score skipped results).

3. **Pagination Metadata in Response**
   - Add a `PaginationInfo` struct: `{ total_hits, offset, page_size, has_next_page }`.
   - `total_hits` is the total count of matching documents (computed once per query, cached).
   - Return `PaginationInfo` alongside results.

4. **REST API Changes**
   - Add `offset` and `page_size` parameters to `/search`.
   - Add `search_after_score` and `search_after_id` for cursor mode.
   - Include `pagination` object in JSON response.

5. **Web UI Changes**
   - Add "Next" / "Previous" buttons or infinite scroll to the search results page.
   - Display "Showing X–Y of Z results".

### Affected Components

| Component | Change Type |
|---|---|
| `SearchOptions` struct | Add `offset`, `search_after_score`, `search_after_id` |
| New: `PaginationInfo` struct | Create |
| `SearchEngine::search()` | Implement offset slicing and cursor logic |
| `rest_server_drogon.cpp` | Parse pagination params, return pagination metadata |
| `ui/` | Add pagination controls |

### Acceptance Criteria

- [ ] `offset=0, page_size=10` returns the first 10 results.
- [ ] `offset=10, page_size=10` returns results 11–20 with correct scores.
- [ ] Cursor-based pagination returns consistent results when index is stable.
- [ ] `total_hits` is accurate and counts all matching documents.
- [ ] `has_next_page` is correctly set.
- [ ] REST API returns proper pagination metadata.
- [ ] Web UI displays working pagination controls.
- [ ] Unit tests cover: first page, middle page, last page, empty page, cursor navigation.

---

## Feature 5: Document Field Boosting

**Priority:** P1 — High
**Estimated Effort:** Medium (3–4 days)

### Description

Allow per-field weight configuration so that matches in certain document fields (e.g., `title`) contribute more to the relevance score than matches in other fields (e.g., `body`). The current engine indexes all fields as a single concatenated text blob via `getAllText()`, losing field-level granularity.

### Motivation

In most document corpora, a match in the title is far more relevant than a match buried in the body text. Without field boosting, a document mentioning "Python" once in its title and a document mentioning "Python" 50 times in its body will be ranked with the body-heavy document higher — which is usually wrong. Elasticsearch, Solr, and Lucene all support field boosting as a core feature.

### Detailed Requirements

1. **Field-Aware Indexing**
   - Modify `SearchEngine::indexDocument()` to index each field separately, prefixing terms with the field name in the inverted index (e.g., `title:machine`, `body:machine`) or using a field-tagged posting list.
   - Maintain a separate term frequency per field per document.
   - Store field lengths separately for accurate BM25 normalization per field.

2. **Boost Configuration**
   - Add a `std::unordered_map<std::string, double> field_boosts` to `SearchOptions` (e.g., `{"title": 3.0, "body": 1.0}`).
   - Default boost for all fields: `1.0`.
   - Support boost configuration at the engine level (persistent defaults) and per-query overrides.

3. **Field-Aware Scoring**
   - Modify the ranker interface to accept field-level term frequencies and boosts.
   - Final score = sum of (field_score × field_boost) across all fields.
   - Update both `TfIdfRanker` and `Bm25Ranker` to support field-aware scoring.

4. **Query-Time Field Specification**
   - The existing `FieldNode` in the query parser already supports `title:machine` syntax.
   - Ensure field-specific queries only search the specified field's postings.
   - Unqualified terms search across all fields with configured boosts.

5. **REST API Changes**
   - Add `field_boosts` JSON parameter to `/search` (e.g., `{"title": 3.0, "body": 1.0}`).
   - Support `boost_title` / `boost_body` shorthand query params.

### Affected Components

| Component | Change Type |
|---|---|
| `InvertedIndex` | Support field-prefixed terms or field-tagged postings |
| `SearchEngine::indexDocument()` | Index fields separately |
| `Ranker` interface | Add field-aware scoring method |
| `TfIdfRanker`, `Bm25Ranker` | Implement field-aware scoring |
| `SearchOptions` struct | Add `field_boosts` |
| `SearchEngine::search()` | Pass boosts to ranker |
| `rest_server_drogon.cpp` | Parse boost parameters |

### Acceptance Criteria

- [ ] A document matching a query term in the `title` field with boost 3.0 scores higher than a document matching only in `body` with boost 1.0.
- [ ] Default behavior (no boosts) produces identical results to current implementation (backward compatible).
- [ ] BM25 and TF-IDF rankers both support field boosting.
- [ ] Field-specific queries (`title:machine`) only match the title field.
- [ ] REST API accepts and applies field boost parameters.
- [ ] Unit tests cover: single field boost, multi-field boost, default boost, field-specific query with boost.
- [ ] Benchmark shows field-aware indexing adds ≤20% overhead to indexing throughput.

---

## Feature 6: Auto-Complete & Type-Ahead Suggestions

**Priority:** P1 — High
**Estimated Effort:** Large (5–7 days)

### Description

Provide real-time query suggestions as the user types, including prefix completions from the vocabulary and popular query completions. A user typing `"mach"` should instantly see suggestions like `"machine"`, `"machine learning"`, `"machu picchu"`.

### Motivation

Auto-complete is one of the most visible and impactful UX features in search engines. It reduces query formulation time, helps users discover content, and reduces misspellings. The current engine has no support for prefix matching — this is a significant gap for the web UI.

### Detailed Requirements

1. **Trie / Compressed Trie Data Structure** (`include/autocomplete.hpp`, `src/autocomplete.cpp`)
   - Implement a compressed trie (Patricia trie) mapping term prefixes to term completions.
   - Each trie node stores the document frequency of the term (for ranking suggestions).
   - Support insertion, prefix lookup, and deletion.
   - Memory-efficient: use path compression to avoid one-node-per-character overhead.

2. **Suggestion Ranking**
   - Rank suggestions by a weighted combination of:
     - Document frequency (how many documents contain this term).
     - Query popularity (how often this term was searched — track in a simple counter map).
     - Prefix match length (longer prefix matches score higher).
   - Return top-K suggestions (configurable, default: 10).

3. **Multi-Word Suggestions**
   - Support phrase-level suggestions by maintaining a secondary trie of indexed multi-word sequences (bigrams, trigrams extracted during indexing).
   - User types `"machine l"` → suggests `"machine learning"`.

4. **Integration with SearchEngine**
   - Build the trie during indexing (incrementally, as documents are added).
   - Rebuild or update trie on document deletion/update.
   - Add `std::vector<std::string> suggest(const std::string& prefix, size_t max_suggestions = 10)` method to `SearchEngine`.

5. **REST API Changes**
   - Add `GET /suggest?q=<prefix>&max=<n>` endpoint.
   - Return `{"suggestions": ["machine", "machine learning", ...]}`.
   - Target latency: <5ms.

6. **Web UI Changes**
   - Add a dropdown suggestion panel below the search box.
   - Trigger on every keystroke with debouncing (200ms).
   - Keyboard navigation (up/down arrows, enter to select).

### Affected Components

| Component | Change Type |
|---|---|
| New: `AutoComplete` class (trie) | Create |
| `SearchEngine` | Add trie member, `suggest()` method |
| `SearchEngine::indexDocument()` | Feed terms into trie |
| `SearchEngine::deleteDocument()` | Remove terms from trie |
| `rest_server_drogon.cpp` | Add `/suggest` endpoint |
| `ui/` | Add suggestion dropdown, debounced API calls |

### Acceptance Criteria

- [ ] Typing `"mach"` returns `"machine"` as a top suggestion.
- [ ] Suggestions are ranked by document frequency.
- [ ] Multi-word prefixes return phrase completions.
- [ ] Trie is updated incrementally on index mutations.
- [ ] `/suggest` endpoint responds in <5ms for vocabularies up to 500K terms.
- [ ] Web UI displays suggestion dropdown with keyboard navigation.
- [ ] Unit tests cover: single char prefix, full word, no matches, special characters, multi-word.
- [ ] Memory benchmark shows trie overhead is <50% of vocabulary size.

---

## Feature 7: Synonym Expansion & Query Rewriting

**Priority:** P2 — Medium
**Estimated Effort:** Medium (3–5 days)

### Description

Expand user queries with synonyms and related terms at query time to improve recall. A search for `"automobile"` should also match documents containing `"car"`, `"vehicle"`, and `"motorcar"`. Support both manual synonym dictionaries and automatic expansion.

### Motivation

Users often search with different vocabulary than what appears in the documents. Without synonym expansion, relevant documents are missed. This is especially important for domain-specific corpora where technical jargon and common language coexist (e.g., `"myocardial infarction"` vs. `"heart attack"`).

### Detailed Requirements

1. **Synonym Dictionary** (`include/synonym_dictionary.hpp`, `src/synonym_dictionary.cpp`)
   - Load synonym mappings from a configurable file (JSON or text format).
   - Format: `{"automobile": ["car", "vehicle", "motorcar"], "fast": ["quick", "rapid", "speedy"]}`.
   - Support two synonym types:
     - **Symmetric synonyms**: A ↔ B (car matches automobile and vice versa).
     - **Directed mappings**: A → B (search for "auto" expands to "automobile" but not the reverse).
   - Case-insensitive matching.

2. **Query Expansion Engine** (`include/query_expander.hpp`, `src/query_expander.cpp`)
   - Given a list of query terms, expand each term using the synonym dictionary.
   - Expanded terms are combined with the original term using OR semantics.
   - Apply a configurable boost penalty to synonym matches (default: 0.8× the original term's score).
   - Limit expansion to a maximum number of synonyms per term (default: 5) to prevent query explosion.

3. **Integration with Search Pipeline**
   - Add a `bool expand_synonyms` flag to `SearchOptions` (default: `false`).
   - The query expander runs after query parsing and before inverted index lookup.
   - Original query terms retain full scoring weight; expanded terms get penalized weight.
   - Return expanded terms in the response for transparency.

4. **REST API Changes**
   - Add `synonyms=true` parameter to `/search`.
   - Return `"query_expansion": {"automobile": ["car", "vehicle"]}` in the response.
   - Add `POST /synonyms` endpoint to load/update the synonym dictionary at runtime.
   - Add `GET /synonyms/<term>` to inspect synonyms for a specific term.

5. **Built-in Default Synonyms**
   - Ship a small default synonym file covering common English synonyms (~500 entries).
   - Loadable from `data/synonyms.json`.

### Affected Components

| Component | Change Type |
|---|---|
| New: `SynonymDictionary` class | Create |
| New: `QueryExpander` class | Create |
| `SearchOptions` struct | Add `expand_synonyms` |
| `SearchEngine` | Add expander member, integrate into search flow |
| `SearchEngine::search()` | Expand query terms before scoring |
| `rest_server_drogon.cpp` | Parse synonym params, add management endpoints |
| New: `data/synonyms.json` | Create default dictionary |

### Acceptance Criteria

- [ ] Searching `"automobile"` with synonyms enabled matches documents containing `"car"`.
- [ ] Synonym matches are scored lower than exact matches (configurable penalty).
- [ ] Symmetric and directed synonym types both work correctly.
- [ ] Query expansion is limited to prevent combinatorial explosion.
- [ ] Synonym dictionary can be loaded from file and updated at runtime.
- [ ] REST API returns expansion information in the response.
- [ ] Unit tests cover: symmetric, directed, no synonyms found, circular references, max expansion limit.

---

## Feature 8: Faceted Search & Aggregations

**Priority:** P2 — Medium
**Estimated Effort:** Large (5–7 days)

### Description

Allow grouping and counting search results by document field values (facets), enabling users to filter and drill down into result sets. For example, searching for `"programming"` could return facets like `{category: {tutorials: 45, articles: 30, videos: 12}}`.

### Motivation

Faceted search is essential for e-commerce, knowledge bases, and any structured document collection. It enables users to refine broad queries without reformulating them. The engine already has field-based document storage (`Document::fields`), but no aggregation capabilities. This feature transforms the engine from a basic search tool to an analytics-capable system.

### Detailed Requirements

1. **Facet Configuration**
   - Add a `std::vector<std::string> facet_fields` to `SearchOptions` specifying which fields to aggregate (e.g., `["category", "author", "year"]`).
   - For each facet field, count the number of matching documents per unique value.
   - Return the top N values per facet (configurable, default: 10).

2. **Facet Index** (`include/facet_index.hpp`, `src/facet_index.cpp`)
   - Maintain a forward index mapping `(field_name, field_value)` → `set<doc_id>`.
   - Build incrementally during indexing.
   - Support efficient intersection with search result document IDs.

3. **Facet Result Struct**
   - Define a `FacetResult` struct: `{ field_name, values: [{value, count}] }`.
   - Return as part of the search response.

4. **Facet Filtering**
   - Add `std::unordered_map<std::string, std::string> facet_filters` to `SearchOptions`.
   - When a filter is set (e.g., `{"category": "tutorials"}`), restrict results to documents matching the filter.
   - Filters are applied *before* ranking to reduce the candidate set.

5. **REST API Changes**
   - Add `facets=category,author` parameter to `/search`.
   - Add `filter_category=tutorials` style parameters for facet filtering.
   - Return `"facets": {"category": [{"value": "tutorials", "count": 45}, ...]}` in JSON.

6. **Web UI Changes**
   - Add a sidebar panel showing facets with checkboxes.
   - Clicking a facet value refines the search results.
   - Show counts next to each facet value.

### Affected Components

| Component | Change Type |
|---|---|
| New: `FacetIndex` class | Create |
| New: `FacetResult` struct | Create |
| `SearchOptions` struct | Add `facet_fields`, `facet_filters` |
| `SearchEngine` | Add facet index member, compute facets during search |
| `SearchEngine::indexDocument()` | Populate facet index |
| `SearchEngine::deleteDocument()` | Remove from facet index |
| `rest_server_drogon.cpp` | Parse facet params, return facet results |
| `ui/` | Add facet sidebar panel |

### Acceptance Criteria

- [ ] Faceting on `"category"` returns correct counts per unique value.
- [ ] Facet counts reflect only the documents matching the current query.
- [ ] Applying a facet filter restricts results to the selected value.
- [ ] Multiple simultaneous facets (e.g., `category` + `author`) work correctly.
- [ ] Facet index is updated on document add/update/delete.
- [ ] REST API returns proper facet JSON structure.
- [ ] Web UI displays interactive facet sidebar.
- [ ] Unit tests cover: single facet, multi-facet, facet with filter, empty facets, missing field.
- [ ] Performance: facet computation adds ≤15% overhead for up to 100K documents.

---

## Feature 9: Wildcard & Regex Search

**Priority:** P2 — Medium
**Estimated Effort:** Medium (4–5 days)

### Description

Support wildcard patterns (e.g., `mach*`, `?earning`) and regular expression queries (e.g., `/learn(ing|ed)/`) against the term vocabulary, enabling powerful pattern-based retrieval for power users.

### Motivation

Wildcard and regex search are important for advanced users, log analysis, and scenarios where the exact term is unknown. The query parser currently supports terms, phrases, boolean operators, field queries, and proximity — but no pattern matching. This fills a significant gap for the "code search" and "log analysis" use cases listed in the README.

### Detailed Requirements

1. **Wildcard Query Support**
   - Support `*` (zero or more characters) and `?` (exactly one character).
   - Example: `mach*` matches `machine`, `machines`, `machining`, etc.
   - Example: `?earning` matches `learning`, `yearning`, `earning`.
   - Implementation: convert wildcard pattern to a regex or use the trie (from Feature 6) for prefix wildcards.

2. **Regex Query Support**
   - Support a subset of POSIX regex: `.`, `*`, `+`, `?`, `[...]`, `(a|b)`, `^`, `$`.
   - Queries enclosed in `/pattern/` are treated as regex.
   - Use `std::regex` or a lightweight regex engine for matching.

3. **Query Parser Integration**
   - Add `WILDCARD` and `REGEX` node types to the query AST.
   - Detect wildcard patterns by presence of `*` or `?` in unquoted terms.
   - Detect regex patterns by `/...../` delimiters.

4. **Term Expansion**
   - At query time, scan the vocabulary for matching terms and expand the query.
   - Apply a max expansion limit (default: 100 terms) to prevent runaway queries.
   - If expansion exceeds the limit, return an error or truncate with a warning.

5. **REST API Changes**
   - Wildcard and regex patterns work naturally in the `q` parameter.
   - Add `"expanded_terms": ["machine", "machines", ...]` to the response when expansion occurs.
   - Add `max_wildcard_expansion` parameter (default: 100).

6. **Performance Guardrails**
   - Leading wildcard patterns (`*ing`) require a full vocabulary scan — warn or limit these.
   - Regex queries have a configurable timeout (default: 100ms) to prevent catastrophic backtracking.

### Affected Components

| Component | Change Type |
|---|---|
| `QueryNode` | Add `WILDCARD` and `REGEX` types |
| New: `WildcardNode`, `RegexNode` classes | Create |
| `QueryParser` | Detect and parse wildcard/regex patterns |
| `SearchEngine::search()` | Expand wildcard/regex terms before index lookup |
| `InvertedIndex` | Expose vocabulary iteration for pattern matching |
| `rest_server_drogon.cpp` | Return expanded terms, expansion limit param |

### Acceptance Criteria

- [ ] `mach*` matches all terms starting with `mach`.
- [ ] `?earning` matches `learning`, `earning`, etc.
- [ ] `/learn(ing|ed)/` matches `learning` and `learned`.
- [ ] Max expansion limit is respected and returns a warning when hit.
- [ ] Leading wildcards work but are flagged as expensive.
- [ ] Regex timeout prevents catastrophic backtracking.
- [ ] Query parser correctly distinguishes wildcard/regex from normal terms.
- [ ] Unit tests cover: trailing wildcard, leading wildcard, single char wildcard, regex alternation, regex character class, expansion limit.

---

## Feature 10: Index Compression (Variable Byte Encoding)

**Priority:** P3 — Low (Performance Optimization)
**Estimated Effort:** Large (5–7 days)

### Description

Compress posting lists using Variable Byte (VByte) encoding and delta encoding to reduce memory consumption and improve cache locality. Currently, posting lists store raw `uint64_t` doc IDs and `uint32_t` frequencies, consuming significant memory at scale.

### Motivation

For large corpora (1M+ documents), the inverted index becomes the dominant memory consumer. A posting list for a common term with 100K postings uses ~800KB of raw doc IDs alone. With delta + VByte encoding, this can be reduced to ~100–200KB (4–8× compression). Improved cache locality from smaller data also accelerates posting list intersection. This is a standard technique used in Lucene, Google's search infrastructure, and academic IR systems.

### Detailed Requirements

1. **Variable Byte Codec** (`include/compression.hpp`, `src/compression.cpp`)
   - Implement VByte encoding: each integer is encoded using 1–9 bytes, with the high bit of each byte indicating continuation.
   - Implement VByte decoding with SIMD acceleration (SIMD-BP128 style batch decoding for 128 integers at a time where supported).
   - Support both encoding and decoding of `uint64_t` values.

2. **Delta Encoding**
   - Posting lists store sorted document IDs. Instead of storing absolute IDs, store deltas (gaps) between consecutive IDs.
   - Delta values are smaller and compress better with VByte.
   - Example: `[3, 7, 15, 20]` → deltas `[3, 4, 8, 5]` → VByte encoded.

3. **Compressed Posting List**
   - Add a `CompressedPostingList` class that stores the encoded byte stream.
   - Provide an iterator interface that lazily decodes postings on access.
   - Support random access via skip pointers (skip pointers store the byte offset into the compressed stream + the absolute doc ID).

4. **Integration with InvertedIndex**
   - Add a `bool compression_enabled` configuration option to `InvertedIndex`.
   - When enabled, posting lists are compressed after bulk indexing or when the list exceeds a size threshold.
   - Decompression happens transparently during query processing.

5. **Persistence Integration**
   - Save compressed posting lists directly to disk (avoid decompress-then-reserialize).
   - Update the snapshot format version to v2 with a compression flag in the header.

6. **Benchmarking**
   - Add a dedicated compression benchmark measuring:
     - Compression ratio (compressed size / raw size).
     - Encoding throughput (MB/s).
     - Decoding throughput (MB/s).
     - Query latency impact (compressed vs. uncompressed).

### Affected Components

| Component | Change Type |
|---|---|
| New: `Compression` codec class | Create (VByte encode/decode) |
| New: `CompressedPostingList` class | Create |
| `InvertedIndex` | Add compression option, integrate compressed lists |
| `PostingList` | Add `compress()` / decompress methods |
| `Persistence` | Support compressed snapshot format (v2) |
| `SnapshotHeader` | Add compression flag, bump version |
| New: `compression_benchmark.cpp` | Create benchmarks |

### Acceptance Criteria

- [ ] VByte codec correctly round-trips all uint64 values (including edge cases: 0, 1, max).
- [ ] Delta encoding produces correct deltas and reconstructs original IDs.
- [ ] Compressed posting lists produce identical query results to uncompressed.
- [ ] Compression ratio is ≥3× for typical document ID distributions.
- [ ] Decoding throughput is ≥500MB/s on a modern CPU.
- [ ] Skip pointers work correctly with compressed lists (byte-offset based).
- [ ] Compressed snapshots load correctly and are smaller than uncompressed.
- [ ] Unit tests cover: small lists, large lists, single element, sequential IDs, sparse IDs.
- [ ] Memory benchmark shows measurable reduction in index memory footprint.

---

## Implementation Priority Summary

| Priority | Feature | Effort | Key Benefit |
|---|---|---|---|
| **P0** | 1. Highlighting & Snippets | Medium | Immediate UX improvement |
| **P0** | 2. Fuzzy Search | Large | Typo tolerance, user satisfaction |
| **P0** | 3. Query Result Caching | Medium | Latency reduction, QPS boost |
| **P1** | 4. Pagination | Small | API completeness, web UI usability |
| **P1** | 5. Field Boosting | Medium | Relevance quality, ranking accuracy |
| **P1** | 6. Auto-Complete | Large | UX, content discovery |
| **P2** | 7. Synonym Expansion | Medium | Recall improvement |
| **P2** | 8. Faceted Search | Large | Analytics capability, enterprise readiness |
| **P2** | 9. Wildcard & Regex | Medium | Power user support, log analysis |
| **P3** | 10. Index Compression | Large | Memory efficiency, cache performance |

### Suggested Implementation Order

```
Phase 1 (Weeks 1–2):  Feature 3 (Caching) → Feature 4 (Pagination) → Feature 1 (Highlighting)
Phase 2 (Weeks 3–4):  Feature 5 (Field Boosting) → Feature 2 (Fuzzy Search)
Phase 3 (Weeks 5–6):  Feature 6 (Auto-Complete) → Feature 7 (Synonyms)
Phase 4 (Weeks 7–8):  Feature 8 (Faceted Search) → Feature 9 (Wildcard/Regex)
Phase 5 (Week 9+):    Feature 10 (Index Compression)
```

> **Note:** Each feature should include unit tests, integration tests, benchmark updates, and REST API documentation as part of the definition of done.
