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
    enum RankingAlgorithm { TF_IDF, BM25 };
    
    RankingAlgorithm algorithm = BM25;
    size_t max_results = 10;
    bool explain_scores = false;
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
 * Main search engine API
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
    
    // Statistics
    IndexStatistics getStats() const;
    
    // Persistence
    bool saveSnapshot(const std::string& filepath);
    bool loadSnapshot(const std::string& filepath);
    
    // Configuration
    void setRanker(std::unique_ptr<Ranker> ranker);
    void setTokenizer(std::unique_ptr<Tokenizer> tokenizer);
    
private:
    friend class Persistence;
    
    std::unique_ptr<Tokenizer> tokenizer_;
    std::unique_ptr<InvertedIndex> index_;
    std::unique_ptr<Ranker> ranker_;
    std::unique_ptr<QueryParser> query_parser_;
    std::unordered_map<uint64_t, Document> documents_;
    uint64_t next_doc_id_;
};

} 
