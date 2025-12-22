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
 * Skip pointer for fast posting list traversal
 */
struct SkipPointer {
    size_t position;      // Index in posting list
    uint64_t doc_id;      // Document ID at this position
    
    SkipPointer() = default;
    SkipPointer(size_t pos, uint64_t id) : position(pos), doc_id(id) {}
};

/**
 * Posting list with skip pointers for fast intersection
 */
class PostingList {
public:
    std::vector<Posting> postings;
    mutable std::vector<SkipPointer> skip_pointers;  // Mutable for lazy initialization
    
    PostingList() = default;
    
    /**
     * Build skip pointers for fast traversal
     * @param skip_interval Number of postings between skip pointers (default: sqrt(size))
     */
    void buildSkipPointers(size_t skip_interval = 0) const;
    
    /**
     * Find optimal starting position using skip pointers
     * @param target_doc_id Target document ID to skip to
     * @return Position in posting list to start scanning from
     */
    size_t findSkipTarget(uint64_t target_doc_id) const;
    
    /**
     * Add a posting to the list (used during indexing)
     */
    void addPosting(const Posting& posting);
    
    /**
     * Mark skip pointers as dirty (need rebuild)
     */
    void markSkipsDirty() { skips_dirty_ = true; }
    
    /**
     * Check if skip pointers need rebuilding
     */
    bool needsSkipRebuild() const { return skips_dirty_; }
    
private:
    mutable bool skips_dirty_ = true;  // Skip pointers need rebuilding (mutable for lazy rebuild)
};

/**
 * Fast AND intersection using skip pointers
 * @param list1 First posting list
 * @param list2 Second posting list
 * @return Document IDs present in both lists
 */
std::vector<uint64_t> intersectWithSkips(
    const PostingList& list1, 
    const PostingList& list2
);

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
     * Get posting list with skip pointers for a term
     */
    PostingList getPostingList(const std::string& term) const;
    
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
    
    /**
     * Rebuild skip pointers for all posting lists
     * Call after bulk indexing operations
     */
    void rebuildSkipPointers();
    
    /**
     * Rebuild skip pointers for a specific term
     */
    void rebuildSkipPointers(const std::string& term);
    
private:
    friend class Persistence;
    
    std::unordered_map<std::string, PostingList> index_;
    mutable std::shared_mutex mutex_;  // Thread safety
};

} 
