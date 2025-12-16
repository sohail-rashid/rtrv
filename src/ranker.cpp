#include "ranker.hpp"
#include <cmath>

namespace search_engine {

// TF-IDF Ranker

TfIdfRanker::TfIdfRanker() = default;

TfIdfRanker::~TfIdfRanker() = default;

double TfIdfRanker::score(const Query& query, 
                          const Document& doc,
                          const IndexStats& stats) {
    // TODO: Implement TF-IDF scoring
    // Formula: TF(term, doc) = log(1 + freq(term, doc))
    //          IDF(term) = log(N / df(term))
    //          Score = sum(TF * IDF for each term)
    double score = 0.0;
    return score;
}

// BM25 Ranker

Bm25Ranker::Bm25Ranker(double k1, double b)
    : k1_(k1), b_(b) {
}

Bm25Ranker::~Bm25Ranker() = default;

double Bm25Ranker::score(const Query& query, 
                         const Document& doc,
                         const IndexStats& stats) {
    // TODO: Implement BM25 scoring
    // Formula: BM25(term, doc) = IDF(term) × (TF × (k1 + 1)) / 
    //          (TF + k1 × (1 - b + b × (|doc| / avgdl)))
    double score = 0.0;
    return score;
}

} // namespace search_engine
