#include "ranker.hpp"
#include <cmath>
#include <algorithm>
#include <cctype>

namespace search_engine {

// TF-IDF Ranker

TfIdfRanker::TfIdfRanker() = default;

TfIdfRanker::~TfIdfRanker() = default;

double TfIdfRanker::score(const Query& query, 
                          const Document& doc,
                          const IndexStats& stats) {
    if (stats.total_docs == 0) {
        return 0.0;
    }
    
    double score = 0.0;
    
    // Count term frequencies in document
    std::unordered_map<std::string, uint32_t> term_freq;
    // For now, we'll need to tokenize the document content
    // This is a simplified version - in practice, this would be pre-computed
    
    for (const auto& query_term : query.terms) {
        // Get term frequency in document (simplified - should use tokenizer)
        uint32_t tf = 0;
        size_t pos = 0;
        std::string lower_content = doc.content;
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

// BM25 Ranker

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
        std::string lower_content = doc.content;
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
            double doc_length = doc.term_count > 0 ? doc.term_count : doc.content.length();
            double normalized_length = 1.0 - b_ + b_ * (doc_length / stats.avg_doc_length);
            double tf_component = (tf * (k1_ + 1.0)) / (tf + k1_ * normalized_length);
            
            score += idf * tf_component;
        }
    }
    
    return score;
}

} 