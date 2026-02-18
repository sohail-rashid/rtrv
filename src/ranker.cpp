#include "ranker.hpp"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace rtrv_search_engine {

// ============================================================================
// TF-IDF Ranker Implementation
// ============================================================================

TfIdfRanker::TfIdfRanker() = default;

TfIdfRanker::~TfIdfRanker() = default;

double TfIdfRanker::score(const Query& query, 
                          const Document& doc,
                          const IndexStats& stats) {
    if (stats.total_docs == 0) {
        return 0.0;
    }
    
    double score = 0.0;
    
    for (const auto& query_term : query.terms) {
        // Get term frequency in document (simplified)
        uint32_t tf = 0;
        size_t pos = 0;
        std::string lower_content = doc.getAllText();
        std::string lower_term = query_term;
        
        // Simple case-insensitive search
        std::transform(lower_content.begin(), lower_content.end(), 
                      lower_content.begin(), ::tolower);
        
        while ((pos = lower_content.find(lower_term, pos)) != std::string::npos) {
            tf++;
            pos += lower_term.length();
        }
        
        if (tf > 0) {
            // Get document frequency
            auto df_it = stats.doc_frequency.find(query_term);
            size_t df = (df_it != stats.doc_frequency.end()) ? df_it->second : 1;
            
            // Calculate TF-IDF
            // TF(term, doc) = log(1 + freq(term, doc))
            // IDF(term) = log(N / df(term))
            double tf_component = std::log(1.0 + tf);
            double idf_component = std::log(static_cast<double>(stats.total_docs) / df);
            
            score += tf_component * idf_component;
        }
    }
    
    return score;
}

// ============================================================================
// BM25 Ranker Implementation
// ============================================================================

Bm25Ranker::Bm25Ranker(double k1, double b)
    : k1_(k1), b_(b) {
}

Bm25Ranker::~Bm25Ranker() = default;

double Bm25Ranker::score(const Query& query, 
                         const Document& doc,
                         const IndexStats& stats) {
    if (stats.total_docs == 0 || stats.avg_doc_length == 0) {
        return 0.0;
    }
    
    double score = 0.0;
    
    for (const auto& query_term : query.terms) {
        // Get term frequency in document (simplified)
        uint32_t tf = 0;
        size_t pos = 0;
        std::string lower_content = doc.getAllText();
        std::string lower_term = query_term;
        
        // Simple case-insensitive search
        std::transform(lower_content.begin(), lower_content.end(), 
                      lower_content.begin(), ::tolower);
        
        while ((pos = lower_content.find(lower_term, pos)) != std::string::npos) {
            tf++;
            pos += lower_term.length();
        }
        
        if (tf > 0) {
            // Get document frequency
            auto df_it = stats.doc_frequency.find(query_term);
            size_t df = (df_it != stats.doc_frequency.end()) ? df_it->second : 1;
            
            // Calculate BM25 components
            // IDF(term) = log((N - df + 0.5) / (df + 0.5) + 1)
            double idf = std::log(
                (stats.total_docs - df + 0.5) / (df + 0.5) + 1.0
            );
            
            // BM25 term frequency component with length normalization
            // TF_component = (tf * (k1 + 1)) / (tf + k1 * (1 - b + b * (doc_length / avg_doc_length)))
            double doc_length = doc.term_count > 0 ? doc.term_count : doc.getAllText().length();
            double normalized_length = 1.0 - b_ + b_ * (doc_length / stats.avg_doc_length);
            double tf_component = (tf * (k1_ + 1.0)) / (tf + k1_ * normalized_length);
            
            score += idf * tf_component;
        }
    }
    
    return score;
}

// ============================================================================
// Custom ML Ranker Implementation (Example)
// ============================================================================

CustomMLRanker::CustomMLRanker() = default;

CustomMLRanker::~CustomMLRanker() = default;

std::vector<double> CustomMLRanker::extractFeatures(const Query& query,
                                                     const Document& doc,
                                                     const IndexStats& stats) {
    std::vector<double> features;
    
    // Feature 1: BM25 score
    Bm25Ranker bm25;
    features.push_back(bm25.score(query, doc, stats));
    
    // Feature 2: TF-IDF score
    TfIdfRanker tfidf;
    features.push_back(tfidf.score(query, doc, stats));
    
    // Feature 3: Query term coverage (what fraction of query terms appear in doc)
    int matched_terms = 0;
    for (const auto& term : query.terms) {
        std::string lower_content = doc.getAllText();
        std::transform(lower_content.begin(), lower_content.end(), 
                      lower_content.begin(), ::tolower);
        if (lower_content.find(term) != std::string::npos) {
            matched_terms++;
        }
    }
    double coverage = query.terms.empty() ? 0.0 : 
                      static_cast<double>(matched_terms) / query.terms.size();
    features.push_back(coverage);
    
    // Feature 4: Document length ratio
    double doc_length = doc.term_count > 0 ? doc.term_count : doc.getAllText().length();
    double length_ratio = stats.avg_doc_length > 0 ? 
                          doc_length / stats.avg_doc_length : 1.0;
    features.push_back(length_ratio);
    
    // Feature 5: Title match bonus
    std::string lower_title = doc.getField("title");
    std::transform(lower_title.begin(), lower_title.end(), 
                  lower_title.begin(), ::tolower);
    int title_matches = 0;
    for (const auto& term : query.terms) {
        if (lower_title.find(term) != std::string::npos) {
            title_matches++;
        }
    }
    features.push_back(static_cast<double>(title_matches));
    
    return features;
}

double CustomMLRanker::score(const Query& query, 
                             const Document& doc,
                             const IndexStats& stats) {
    // Extract features
    auto features = extractFeatures(query, doc, stats);
    
    // Simple linear model (in practice, this would be a trained ML model)
    // Weights learned from training data
    std::vector<double> weights = {
        0.4,   // BM25 weight
        0.2,   // TF-IDF weight
        0.2,   // Coverage weight
        0.05,  // Length ratio weight
        0.15   // Title match weight
    };
    
    double score = 0.0;
    for (size_t i = 0; i < features.size() && i < weights.size(); ++i) {
        score += features[i] * weights[i];
    }
    
    return score;
}

// ============================================================================
// Ranker Registry Implementation
// ============================================================================

RankerRegistry::RankerRegistry() 
    : default_ranker_("BM25") {
    // Register built-in rankers
    registerRanker(std::make_unique<TfIdfRanker>());
    registerRanker(std::make_unique<Bm25Ranker>());
    registerRanker(std::make_unique<CustomMLRanker>());
}

RankerRegistry::~RankerRegistry() = default;

void RankerRegistry::registerRanker(std::unique_ptr<Ranker> ranker) {
    if (!ranker) {
        throw std::invalid_argument("Cannot register null ranker");
    }
    
    std::string name = ranker->getName();
    rankers_[name] = std::move(ranker);
}

Ranker* RankerRegistry::getRanker(const std::string& name) {
    auto it = rankers_.find(name);
    if (it != rankers_.end()) {
        return it->second.get();
    }
    
    // Fallback to default if requested ranker not found
    auto default_it = rankers_.find(default_ranker_);
    if (default_it != rankers_.end()) {
        return default_it->second.get();
    }
    
    return nullptr;
}

const Ranker* RankerRegistry::getRanker(const std::string& name) const {
    auto it = rankers_.find(name);
    if (it != rankers_.end()) {
        return it->second.get();
    }
    
    // Fallback to default
    auto default_it = rankers_.find(default_ranker_);
    if (default_it != rankers_.end()) {
        return default_it->second.get();
    }
    
    return nullptr;
}

Ranker* RankerRegistry::getDefaultRanker() {
    return getRanker(default_ranker_);
}

const Ranker* RankerRegistry::getDefaultRanker() const {
    return getRanker(default_ranker_);
}

std::vector<std::string> RankerRegistry::listRankers() const {
    std::vector<std::string> names;
    names.reserve(rankers_.size());
    
    for (const auto& [name, _] : rankers_) {
        names.push_back(name);
    }
    
    // Sort alphabetically for consistent output
    std::sort(names.begin(), names.end());
    
    return names;
}

bool RankerRegistry::setDefaultRanker(const std::string& name) {
    if (rankers_.count(name)) {
        default_ranker_ = name;
        return true;
    }
    return false;
}

bool RankerRegistry::hasRanker(const std::string& name) const {
    return rankers_.count(name) > 0;
}

} 