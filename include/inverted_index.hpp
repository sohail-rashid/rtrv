#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <shared_mutex>

namespace search_engine {

/**
 * Represents a posting entry in the inverted index
 */
struct Posting {
    uint64_t doc_id;                       // Document ID
    uint32_t term_frequency;               // Term frequency in document
    std::vector<uint32_t> positions;       // Optional: term positions for phrase search
    
    Posting() = default;
    Posting(uint64_t doc_id, uint32_t term_frequency);
};

/**
 * Inverted index mapping terms to documents
 */
class InvertedIndex {
public:
    InvertedIndex();
    ~InvertedIndex();
    
    /**
     * Add a term occurrence for a document
     */
    void addTerm(const std::string& term, uint64_t doc_id, uint32_t position = 0);
    
    /**
     * Get posting list for a term
     */
    std::vector<Posting> getPostings(const std::string& term) const;
    
    /**
     * Remove all postings for a document
     */
    void removeDocument(uint64_t doc_id);
    
    /**
     * Get document frequency (number of documents containing term)
     */
    size_t getDocumentFrequency(const std::string& term) const;
    
    /**
     * Get total number of unique terms
     */
    size_t getTermCount() const;
    
    /**
     * Clear the entire index
     */
    void clear();
    
private:
    std::unordered_map<std::string, std::vector<Posting>> index_;
    mutable std::shared_mutex mutex_;  // Thread safety
};

} 
