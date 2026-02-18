#pragma once

#include "document.hpp"
#include "tokenizer.hpp"
#include "inverted_index.hpp"
#include "ranker.hpp"
#include "query_parser.hpp"
#include "snippet_extractor.hpp"
#include "fuzzy_search.hpp"
#include "query_cache.hpp"
#include "search_types.hpp"
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <shared_mutex>

namespace rtrv_search_engine {

/**
 * Main search engine API with plugin architecture for rankers
 */
class SearchEngine {
public:
    SearchEngine();
    ~SearchEngine();
    
    // Indexing operations
    uint64_t indexDocument(const Document& doc);
    void indexDocuments(const std::vector<Document>& docs);
    bool updateDocument(uint64_t doc_id, const Document& doc);
    bool deleteDocument(uint64_t doc_id);
    
    // Search operations
    std::vector<SearchResult> search(const std::string& query,
                                     const SearchOptions& options = {});
    
    // Paginated search — returns results with pagination metadata
    PaginatedSearchResults searchPaginated(const std::string& query,
                                           const SearchOptions& options = {});
    
    // Overload for searching with specific ranker
    std::vector<SearchResult> search(const std::string& query,
                                     const std::string& ranker_name,
                                     size_t max_results = 10);
    
    // Statistics
    IndexStatistics getStats() const;
    CacheStatistics getCacheStats() const;

    // List documents (for browsing)
    std::vector<std::pair<uint64_t, Document>> getDocuments(size_t offset = 0, size_t limit = 10) const;
    void clearCache();
    void setCacheConfig(size_t max_entries, std::chrono::milliseconds ttl);
    
    // Persistence
    bool saveSnapshot(const std::string& filepath);
    bool loadSnapshot(const std::string& filepath);
    
    // Configuration
    void setTokenizer(std::unique_ptr<Tokenizer> tokenizer);
    
    // Ranker management (plugin architecture)
    void registerCustomRanker(std::unique_ptr<Ranker> ranker);
    void setDefaultRanker(const std::string& ranker_name);
    std::string getDefaultRanker() const;
    std::vector<std::string> listAvailableRankers() const;
    bool hasRanker(const std::string& name) const;
    
    // Get ranker for direct parameter tuning
    Ranker* getRanker(const std::string& name);
    
    // Get index for direct access (e.g., skip pointer operations)
    InvertedIndex* getIndex() { return index_.get(); }
    const InvertedIndex* getIndex() const { return index_.get(); }
    
    // Get snippet extractor for direct use
    const SnippetExtractor& getSnippetExtractor() const { return snippet_extractor_; }
    
    // Get fuzzy search for direct use
    FuzzySearch& getFuzzySearch() { return fuzzy_search_; }
    const FuzzySearch& getFuzzySearch() const { return fuzzy_search_; }
    
    // Tokenizer configuration
    void enableSIMD(bool enabled) { if (tokenizer_) tokenizer_->enableSIMD(enabled); }
    void setStemmer(StemmerType type) { if (tokenizer_) tokenizer_->setStemmer(type); }
    void setRemoveStopwords(bool enabled) { if (tokenizer_) tokenizer_->setRemoveStopwords(enabled); }
    
    // Deprecated: Use registerCustomRanker() instead
    void setRanker(std::unique_ptr<Ranker> ranker);
    
private:
    friend class Persistence;
    
    // Internal indexing without locking (caller must hold mutex_)
    uint64_t indexDocumentInternal(const Document& doc);
    
    std::unique_ptr<Tokenizer> tokenizer_;
    std::unique_ptr<InvertedIndex> index_;
    std::unique_ptr<QueryParser> query_parser_;
    std::unique_ptr<RankerRegistry> ranker_registry_;
    SnippetExtractor snippet_extractor_;
    FuzzySearch fuzzy_search_;
    QueryCache query_cache_;
    std::unordered_map<uint64_t, Document> documents_;
    uint64_t next_doc_id_;
    mutable std::shared_mutex mutex_;  // Thread safety for documents_ and next_doc_id_
};

} 
