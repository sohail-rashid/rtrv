#pragma once

#include "document.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace search_engine {

/**
 * Statistics needed for ranking
 */
struct IndexStats {
    size_t total_docs;                                       // Total number of documents
    double avg_doc_length;                                   // Average document length
    std::unordered_map<std::string, size_t> doc_frequency;  // Document frequency per term
};

/**
 * Query representation
 */
struct Query {
    std::vector<std::string> terms;
};

/**
 * Base class for ranking algorithms
 */
class Ranker {
public:
    virtual ~Ranker() = default;
    
    /**
     * Score a document for a query
     */
    virtual double score(const Query& query, 
                        const Document& doc,
                        const IndexStats& stats) = 0;
};

/**
 * TF-IDF ranking algorithm
 */
class TfIdfRanker : public Ranker {
public:
    TfIdfRanker();
    ~TfIdfRanker() override;
    
    double score(const Query& query, 
                const Document& doc,
                const IndexStats& stats) override;
};

/**
 * BM25 ranking algorithm
 */
class Bm25Ranker : public Ranker {
public:
    Bm25Ranker(double k1 = 1.5, double b = 0.75);
    ~Bm25Ranker() override;
    
    double score(const Query& query, 
                const Document& doc,
                const IndexStats& stats) override;
    
private:
    double k1_;  // Term saturation parameter
    double b_;   // Length normalization parameter
};

}
