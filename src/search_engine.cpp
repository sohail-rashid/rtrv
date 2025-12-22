#include "search_engine.hpp"
#include "persistence.hpp"

namespace search_engine {

SearchEngine::SearchEngine()
    : tokenizer_(std::make_unique<Tokenizer>()),
      index_(std::make_unique<InvertedIndex>()),
      ranker_(std::make_unique<Bm25Ranker>()),
      query_parser_(std::make_unique<QueryParser>()),
      next_doc_id_(1) {
    // Enable SIMD tokenization for better performance
    tokenizer_->enableSIMD(true);
}

SearchEngine::~SearchEngine() = default;

uint64_t SearchEngine::indexDocument(const Document& doc) {
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
    }
    
    // Store document
    documents_[doc_id] = indexed_doc;
    
    return doc_id;
}

void SearchEngine::indexDocuments(const std::vector<Document>& docs) {
    // batch indexing
    for (const auto& doc : docs) {
        indexDocument(doc);
    }
}

bool SearchEngine::updateDocument(uint64_t doc_id, const Document& doc) {
    // Check if document exists
    if (documents_.find(doc_id) == documents_.end()) {
        return false;
    }
    
    // Delete old document from index
    index_->removeDocument(doc_id);
    
    // Create updated document with same ID
    Document updated_doc = doc;
    updated_doc.id = doc_id;
    
    // Re-index with same ID
    indexDocument(updated_doc);
    
    return true;
}

bool SearchEngine::deleteDocument(uint64_t doc_id) {
    // Check if document exists
    auto it = documents_.find(doc_id);
    if (it == documents_.end()) {
        return false;
    }
    
    // Remove from inverted index
    index_->removeDocument(doc_id);
    
    // Remove from document store
    documents_.erase(it);
    
    return true;
}

std::vector<SearchResult> SearchEngine::search(const std::string& query,
                                               const SearchOptions& options) {
    std::vector<SearchResult> results;
    
    // Extract query terms
    auto query_terms = query_parser_->extractTerms(query);
    if (query_terms.empty()) {
        return results;
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
    
    // Select ranker based on options
    Ranker* ranker_to_use = ranker_.get();
    std::unique_ptr<TfIdfRanker> tfidf_temp;
    if (options.algorithm == SearchOptions::TF_IDF) {
        tfidf_temp = std::make_unique<TfIdfRanker>();
        ranker_to_use = tfidf_temp.get();
    }
    
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
                    result.explanation = "Score: " + std::to_string(score);
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
    
    return results;
}

IndexStatistics SearchEngine::getStats() const {
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

bool SearchEngine::saveSnapshot(const std::string& filepath) {
    return Persistence::save(*this, filepath);
}

bool SearchEngine::loadSnapshot(const std::string& filepath) {
    return Persistence::load(*this, filepath);
}

void SearchEngine::setRanker(std::unique_ptr<Ranker> ranker) {
    ranker_ = std::move(ranker);
}

void SearchEngine::setTokenizer(std::unique_ptr<Tokenizer> tokenizer) {
    tokenizer_ = std::move(tokenizer);
}

} 
