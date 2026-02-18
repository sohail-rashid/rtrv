#include "search_engine.hpp"
#include "persistence.hpp"
#include "top_k_heap.hpp"
#include "snippet_extractor.hpp"
#include <cctype>

namespace {

std::string normalizeQuery(const std::string& query) {
    std::string normalized;
    normalized.reserve(query.size());

    bool in_space = false;
    for (unsigned char ch : query) {
        if (std::isspace(ch)) {
            if (!normalized.empty() && !in_space) {
                normalized.push_back(' ');
                in_space = true;
            }
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(ch)));
        in_space = false;
    }

    if (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }

    return normalized;
}

size_t hashCombine(size_t seed, size_t value) {
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

size_t hashSearchOptions(const search_engine::SearchOptions& options) {
    size_t seed = 0;
    seed = hashCombine(seed, std::hash<std::string>{}(options.ranker_name));
    seed = hashCombine(seed, static_cast<size_t>(options.algorithm));
    seed = hashCombine(seed, std::hash<size_t>{}(options.max_results));
    seed = hashCombine(seed, std::hash<bool>{}(options.explain_scores));
    seed = hashCombine(seed, std::hash<bool>{}(options.use_top_k_heap));
    seed = hashCombine(seed, std::hash<bool>{}(options.generate_snippets));
    seed = hashCombine(seed, std::hash<size_t>{}(options.snippet_options.max_snippet_length));
    seed = hashCombine(seed, std::hash<size_t>{}(options.snippet_options.num_snippets));
    seed = hashCombine(seed, std::hash<std::string>{}(options.snippet_options.highlight_open));
    seed = hashCombine(seed, std::hash<std::string>{}(options.snippet_options.highlight_close));
    seed = hashCombine(seed, std::hash<bool>{}(options.fuzzy_enabled));
    seed = hashCombine(seed, std::hash<uint32_t>{}(options.max_edit_distance));
    return seed;
}

}

namespace search_engine {

SearchEngine::SearchEngine()
    : tokenizer_(std::make_unique<Tokenizer>()),
      index_(std::make_unique<InvertedIndex>()),
      query_parser_(std::make_unique<QueryParser>()),
      ranker_registry_(std::make_unique<RankerRegistry>()),
      next_doc_id_(1) {
    // Enable SIMD tokenization for better performance
    tokenizer_->enableSIMD(true);
    
    // RankerRegistry automatically registers built-in rankers (BM25, TF-IDF, ML-Ranker)
}

SearchEngine::~SearchEngine() = default;

uint64_t SearchEngine::indexDocument(const Document& doc) {
    std::unique_lock lock(mutex_);
    const auto doc_id = indexDocumentInternal(doc);
    query_cache_.clear();
    return doc_id;
}

uint64_t SearchEngine::indexDocumentInternal(const Document& doc) {
    // Use provided doc ID or generate new one
    uint64_t doc_id = (doc.id > 0) ? doc.id : next_doc_id_++;
    
    // Create mutable copy of document
    Document indexed_doc = doc;
    indexed_doc.id = doc_id;
    
    // Tokenize document content
    auto tokens = tokenizer_->tokenize(doc.getAllText());
    indexed_doc.term_count = tokens.size();
    
    // Add terms to inverted index with positions
    uint32_t position = 0;
    for (const auto& term : tokens) {
        index_->addTerm(term, doc_id, position++);
        // Incrementally update fuzzy n-gram index
        if (fuzzy_search_.isIndexBuilt()) {
            fuzzy_search_.addTerm(term);
        }
    }
    
    // Store document
    documents_[doc_id] = indexed_doc;
    
    return doc_id;
}

void SearchEngine::indexDocuments(const std::vector<Document>& docs) {
    std::unique_lock lock(mutex_);
    for (const auto& doc : docs) {
        indexDocumentInternal(doc);
    }
    query_cache_.clear();
}

bool SearchEngine::updateDocument(uint64_t doc_id, const Document& doc) {
    std::unique_lock lock(mutex_);
    
    // Check if document exists
    if (documents_.find(doc_id) == documents_.end()) {
        return false;
    }
    
    // Delete old document from index
    index_->removeDocument(doc_id);
    
    // Create updated document with same ID
    Document updated_doc = doc;
    updated_doc.id = doc_id;
    
    // Re-index with same ID (using internal method — lock already held)
    indexDocumentInternal(updated_doc);
    
    query_cache_.clear();
    return true;
}

bool SearchEngine::deleteDocument(uint64_t doc_id) {
    std::unique_lock lock(mutex_);
    
    // Check if document exists
    auto it = documents_.find(doc_id);
    if (it == documents_.end()) {
        return false;
    }
    
    // Remove from inverted index
    index_->removeDocument(doc_id);
    
    // Remove from document store
    documents_.erase(it);
    
    query_cache_.clear();
    return true;
}

std::vector<SearchResult> SearchEngine::search(const std::string& query,
                                               const SearchOptions& options) {
    std::shared_lock lock(mutex_);
    
    std::vector<SearchResult> results;
    const bool use_cache = options.use_cache;
    QueryCacheKey cache_key;

    if (use_cache) {
        cache_key.normalized_query = normalizeQuery(query);
        cache_key.options_hash = hashSearchOptions(options);

        if (!cache_key.normalized_query.empty() &&
            query_cache_.get(cache_key, &results)) {
            return results;
        }
    }
    
    // Extract query terms
    auto query_terms = query_parser_->extractTerms(query);
    if (query_terms.empty()) {
        return results;
    }
    
    // Fuzzy search: expand query terms that have zero exact matches
    std::unordered_map<std::string, std::string> fuzzy_expansions;
    if (options.fuzzy_enabled) {
        // Build n-gram index if not already built
        const auto vocabulary = index_->getVocabulary();
        if (!fuzzy_search_.isIndexBuilt()) {
            fuzzy_search_.buildNgramIndex(vocabulary);
        }
        
        std::vector<std::string> expanded_terms;
        for (const auto& term : query_terms) {
            // Check if term has exact matches in the index
            if (index_->getDocumentFrequency(term) > 0) {
                expanded_terms.push_back(term);
                continue;
            }
            
            // Prefix match: "machin" -> "machine"
            std::string prefix_match;
            for (const auto& vocab_term : vocabulary) {
                if (vocab_term.rfind(term, 0) == 0) {
                    if (prefix_match.empty() || vocab_term.size() < prefix_match.size()) {
                        prefix_match = vocab_term;
                    }
                }
            }
            if (!prefix_match.empty()) {
                expanded_terms.push_back(prefix_match);
                fuzzy_expansions[term] = prefix_match;
                continue;
            }
            
            // No exact match — try fuzzy matching
            auto matches = fuzzy_search_.findMatches(
                term, options.max_edit_distance, 5);
            
            if (!matches.empty()) {
                // Use the best (closest) match
                const auto& best = matches[0];
                expanded_terms.push_back(best.matched_term);
                fuzzy_expansions[term] = best.matched_term;
            } else {
                // No fuzzy match found — keep original term
                expanded_terms.push_back(term);
            }
        }
        query_terms = expanded_terms;
    }
    
    // Create Query object
    Query q;
    q.terms = query_terms;
    
    // Prepare index statistics
    IndexStats stats;
    stats.total_docs = documents_.size();
    
    // Calculate average document length
    if (!documents_.empty()) {
        size_t total_length = 0;
        for (const auto& [id, doc] : documents_) {
            total_length += doc.term_count;
        }
        stats.avg_doc_length = static_cast<double>(total_length) / documents_.size();
    } else {
        stats.avg_doc_length = 0.0;
    }
    
    // Get document frequencies for query terms
    for (const auto& term : query_terms) {
        stats.doc_frequency[term] = index_->getDocumentFrequency(term);
    }
    
    // Collect candidate documents from posting lists
    std::unordered_set<uint64_t> candidate_doc_ids;
    for (const auto& term : query_terms) {
        auto postings = index_->getPostings(term);
        for (const auto& posting : postings) {
            candidate_doc_ids.insert(posting.doc_id);
        }
    }
    
    // Select ranker (plugin architecture)
    Ranker* ranker_to_use = nullptr;
    
    if (!options.ranker_name.empty()) {
        // Use specified ranker
        ranker_to_use = ranker_registry_->getRanker(options.ranker_name);
    } else if (options.algorithm == SearchOptions::TF_IDF) {
        // Backward compatibility: use algorithm enum
        ranker_to_use = ranker_registry_->getRanker("TF-IDF");
    } else {
        // Use default ranker
        ranker_to_use = ranker_registry_->getDefaultRanker();
    }
    
    if (!ranker_to_use) {
        // Fallback to BM25 if ranker not found
        ranker_to_use = ranker_registry_->getRanker("BM25");
    }
    
    // Branch: Use Top-K heap or traditional sorting
    if (options.use_top_k_heap) {
        // ============================================================
        // TOP-K HEAP APPROACH: O(N log K) time, O(K) space
        // ============================================================
        BoundedPriorityQueue<ScoredDocument> top_k(options.max_results);
        
        // Score all candidates and maintain top-K
        for (uint64_t doc_id : candidate_doc_ids) {
            auto doc_it = documents_.find(doc_id);
            if (doc_it != documents_.end()) {
                double score = ranker_to_use->score(q, doc_it->second, stats);
                
                if (score > 0.0) {
                    // Only add if better than worst in heap (or heap not full)
                    if (!top_k.isFull() || score > top_k.minScore()) {
                        top_k.push({doc_id, score});
                    }
                }
            }
        }
        
        // Extract sorted results from heap (descending order)
        auto sorted_docs = top_k.getSorted();
        
        // Build final results
        results.reserve(sorted_docs.size());
        for (const auto& scored_doc : sorted_docs) {
            auto doc_it = documents_.find(scored_doc.doc_id);
            if (doc_it != documents_.end()) {
                SearchResult result;
                result.document = doc_it->second;
                result.score = scored_doc.score;
                
                if (options.explain_scores) {
                    result.explanation = "Ranker: " + ranker_to_use->getName() + 
                                       ", Score: " + std::to_string(scored_doc.score) +
                                       ", Method: Top-K Heap (O(N log K))";
                }
                
                results.push_back(result);
            }
        }
        
    } else {
        // ============================================================
        // TRADITIONAL APPROACH: O(N log N) time, O(N) space
        // ============================================================
        // Score all candidate documents
        for (uint64_t doc_id : candidate_doc_ids) {
            auto doc_it = documents_.find(doc_id);
            if (doc_it != documents_.end()) {
                double score = ranker_to_use->score(q, doc_it->second, stats);
                
                if (score > 0.0) {
                    SearchResult result;
                    result.document = doc_it->second;
                    result.score = score;
                    
                    if (options.explain_scores) {
                        result.explanation = "Ranker: " + ranker_to_use->getName() + 
                                           ", Score: " + std::to_string(score) +
                                           ", Method: Full Sort (O(N log N))";
                    }
                    
                    results.push_back(result);
                }
            }
        }
        
        // Sort by score (descending)
        std::sort(results.begin(), results.end(),
                  [](const SearchResult& a, const SearchResult& b) {
                      return a.score > b.score;
                  });
        
        // Return top-K results
        if (results.size() > options.max_results) {
            results.resize(options.max_results);
        }
    }
    
    // Post-process: generate snippets if requested
    if (options.generate_snippets && !results.empty()) {
        for (auto& result : results) {
            std::string doc_text = result.document.getAllText();
            result.snippets = snippet_extractor_.generateSnippets(
                doc_text, query_terms, options.snippet_options);
        }
    }
    
    // Post-process: apply fuzzy scoring penalty and attach expansion info
    if (options.fuzzy_enabled && !fuzzy_expansions.empty()) {
        // Apply a scoring penalty proportional to the number of fuzzy-expanded terms
        double penalty = 1.0 - (0.1 * fuzzy_expansions.size());
        if (penalty < 0.5) penalty = 0.5;
        
        for (auto& result : results) {
            result.score *= penalty;
            result.expanded_terms = fuzzy_expansions;
        }
    }
    
    if (use_cache && !cache_key.normalized_query.empty()) {
        query_cache_.put(cache_key, results);
    }

    return results;
}

IndexStatistics SearchEngine::getStats() const {
    std::shared_lock lock(mutex_);
    
    IndexStatistics stats;
    stats.total_documents = documents_.size();
    stats.total_terms = index_->getTermCount();
    
    // Calculate average document length
    if (!documents_.empty()) {
        size_t total_length = 0;
        for (const auto& [id, doc] : documents_) {
            total_length += doc.term_count;
        }
        stats.avg_doc_length = static_cast<double>(total_length) / documents_.size();
    } else {
        stats.avg_doc_length = 0.0;
    }
    
    return stats;
}

CacheStatistics SearchEngine::getCacheStats() const {
    return query_cache_.getStats();
}

void SearchEngine::clearCache() {
    query_cache_.clear();
}

void SearchEngine::setCacheConfig(size_t max_entries, std::chrono::milliseconds ttl) {
    query_cache_.setMaxEntries(max_entries);
    query_cache_.setTtl(ttl);
}

bool SearchEngine::saveSnapshot(const std::string& filepath) {
    std::shared_lock lock(mutex_);
    return Persistence::save(*this, filepath);
}

bool SearchEngine::loadSnapshot(const std::string& filepath) {
    std::unique_lock lock(mutex_);
    const bool loaded = Persistence::load(*this, filepath);
    if (loaded) {
        query_cache_.clear();
    }
    return loaded;
}

// Search overload with specific ranker name
std::vector<SearchResult> SearchEngine::search(const std::string& query,
                                               const std::string& ranker_name,
                                               size_t max_results) {
    SearchOptions options;
    options.ranker_name = ranker_name;
    options.max_results = max_results;
    return search(query, options);
}

// Ranker management methods (plugin architecture)
void SearchEngine::registerCustomRanker(std::unique_ptr<Ranker> ranker) {
    if (ranker && ranker_registry_) {
        ranker_registry_->registerRanker(std::move(ranker));
    }
}

void SearchEngine::setDefaultRanker(const std::string& ranker_name) {
    if (ranker_registry_) {
        ranker_registry_->setDefaultRanker(ranker_name);
    }
}

std::string SearchEngine::getDefaultRanker() const {
    if (ranker_registry_) {
        return ranker_registry_->getDefaultRankerName();
    }
    return "BM25";
}

std::vector<std::string> SearchEngine::listAvailableRankers() const {
    if (ranker_registry_) {
        return ranker_registry_->listRankers();
    }
    return {};
}

bool SearchEngine::hasRanker(const std::string& name) const {
    if (ranker_registry_) {
        return ranker_registry_->hasRanker(name);
    }
    return false;
}

Ranker* SearchEngine::getRanker(const std::string& name) {
    if (ranker_registry_) {
        return ranker_registry_->getRanker(name);
    }
    return nullptr;
}

// Deprecated: Use registerCustomRanker() instead
void SearchEngine::setRanker(std::unique_ptr<Ranker> ranker) {
    if (ranker && ranker_registry_) {
        ranker_registry_->registerRanker(std::move(ranker));
    }
}

void SearchEngine::setTokenizer(std::unique_ptr<Tokenizer> tokenizer) {
    tokenizer_ = std::move(tokenizer);
}

} 
