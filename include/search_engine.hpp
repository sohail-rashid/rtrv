#pragma once

#include "document.hpp"
#include "tokenizer.hpp"
#include "inverted_index.hpp"
#include "ranker.hpp"
#include "query_parser.hpp"
#include <string>
#include <vector>
#include <memory>

namespace search_engine {

/**
 * Search options
 */
struct SearchOptions {
    std::string ranker_name = "";  // Empty = use default ranker
    size_t max_results = 10;
    bool explain_scores = false;
    bool use_top_k_heap = true;  // Use bounded priority queue for Top-K retrieval
    
    // Deprecated: Use ranker_name instead
    enum RankingAlgorithm { TF_IDF, BM25 };
    RankingAlgorithm algorithm = BM25;  // For backward compatibility
};

/**
 * Search result
 */
struct SearchResult {
    Document document;
    double score;
    std::string explanation;  // Optional score breakdown
};

/**
 * Index statistics
 */
struct IndexStatistics {
    size_t total_documents;
    size_t total_terms;
    double avg_doc_length;
};

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
    
    // Overload for searching with specific ranker
    std::vector<SearchResult> search(const std::string& query,
                                     const std::string& ranker_name,
                                     size_t max_results = 10);
    
    // Statistics
    IndexStatistics getStats() const;
    
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
    
    // Tokenizer configuration
    void enableSIMD(bool enabled) { if (tokenizer_) tokenizer_->enableSIMD(enabled); }
    void setStemmer(StemmerType type) { if (tokenizer_) tokenizer_->setStemmer(type); }
    void setRemoveStopwords(bool enabled) { if (tokenizer_) tokenizer_->setRemoveStopwords(enabled); }
    
    // Deprecated: Use registerCustomRanker() instead
    void setRanker(std::unique_ptr<Ranker> ranker);
    
private:
    friend class Persistence;
    
    std::unique_ptr<Tokenizer> tokenizer_;
    std::unique_ptr<InvertedIndex> index_;
    std::unique_ptr<QueryParser> query_parser_;
    std::unique_ptr<RankerRegistry> ranker_registry_;
    std::unordered_map<uint64_t, Document> documents_;
    uint64_t next_doc_id_;
};

} 
