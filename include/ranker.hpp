#pragma once

#include "document.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

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
 * Abstract base class for all ranking algorithms
 * Implements plugin architecture for hot-swappable rankers
 */
class Ranker {
public:
    virtual ~Ranker() = default;
    
    /**
     * Score a single document for a query
     */
    virtual double score(const Query& query, 
                        const Document& doc,
                        const IndexStats& stats) = 0;
    
    /**
     * Get the name of this ranker
     */
    virtual std::string getName() const = 0;
    
    /**
     * Batch scoring for efficiency (optional optimization)
     * Default implementation calls score() for each document
     */
    virtual std::vector<double> scoreBatch(
        const Query& query,
        const std::vector<Document>& docs,
        const IndexStats& stats
    ) {
        std::vector<double> scores;
        scores.reserve(docs.size());
        for (const auto& doc : docs) {
            scores.push_back(score(query, doc, stats));
        }
        return scores;
    }
};

/**
 * TF-IDF ranking algorithm
 * TF(term, doc) = log(1 + freq(term, doc))
 * IDF(term) = log(N / df(term))
 */
class TfIdfRanker : public Ranker {
public:
    TfIdfRanker();
    ~TfIdfRanker() override;
    
    double score(const Query& query, 
                const Document& doc,
                const IndexStats& stats) override;
    
    std::string getName() const override { return "TF-IDF"; }
};

/**
 * BM25 ranking algorithm (Okapi BM25)
 * Industry-standard probabilistic ranking function
 * IDF(term) = log((N - df + 0.5) / (df + 0.5) + 1)
 * TF_component = (tf * (k1 + 1)) / (tf + k1 * (1 - b + b * (doc_length / avg_doc_length)))
 */
class Bm25Ranker : public Ranker {
public:
    Bm25Ranker(double k1 = 1.5, double b = 0.75);
    ~Bm25Ranker() override;
    
    double score(const Query& query, 
                const Document& doc,
                const IndexStats& stats) override;
    
    std::string getName() const override { return "BM25"; }
    
    /**
     * Adjust BM25 parameters
     * @param k1 Term saturation parameter (default: 1.5)
     * @param b Length normalization parameter (default: 0.75)
     */
    void setParameters(double k1, double b) { k1_ = k1; b_ = b; }
    
    double getK1() const { return k1_; }
    double getB() const { return b_; }
    
private:
    double k1_;  // Term saturation parameter
    double b_;   // Length normalization parameter
};

/**
 * Custom ML-based ranker example
 * Demonstrates how to create custom rankers
 */
class CustomMLRanker : public Ranker {
public:
    CustomMLRanker();
    ~CustomMLRanker() override;
    
    double score(const Query& query, 
                const Document& doc,
                const IndexStats& stats) override;
    
    std::string getName() const override { return "ML-Ranker"; }
    
private:
    // Extract features for ML model
    std::vector<double> extractFeatures(const Query& query,
                                       const Document& doc,
                                       const IndexStats& stats);
};

/**
 * Ranker Registry - manages available ranking algorithms
 * Implements plugin pattern for hot-swappable rankers
 */
class RankerRegistry {
public:
    RankerRegistry();
    ~RankerRegistry();
    
    /**
     * Register a ranker (takes ownership)
     */
    void registerRanker(std::unique_ptr<Ranker> ranker);
    
    /**
     * Get ranker by name
     * Returns nullptr if not found
     */
    Ranker* getRanker(const std::string& name);
    
    /**
     * Get ranker by name (const version)
     */
    const Ranker* getRanker(const std::string& name) const;
    
    /**
     * Get default ranker
     */
    Ranker* getDefaultRanker();
    
    /**
     * Get default ranker (const version)
     */
    const Ranker* getDefaultRanker() const;
    
    /**
     * List all available ranker names
     */
    std::vector<std::string> listRankers() const;
    
    /**
     * Set default ranker by name
     */
    bool setDefaultRanker(const std::string& name);
    
    /**
     * Get current default ranker name
     */
    std::string getDefaultRankerName() const { return default_ranker_; }
    
    /**
     * Check if a ranker exists
     */
    bool hasRanker(const std::string& name) const;
    
private:
    std::unordered_map<std::string, std::unique_ptr<Ranker>> rankers_;
    std::string default_ranker_;
};

}

