#include "inverted_index.hpp"
#include <algorithm>

namespace search_engine {

Posting::Posting(uint64_t doc_id, uint32_t term_frequency)
    : doc_id(doc_id), term_frequency(term_frequency) {
}

InvertedIndex::InvertedIndex() = default;

InvertedIndex::~InvertedIndex() = default;

void InvertedIndex::addTerm(const std::string& term, uint64_t doc_id, uint32_t position) {
    std::unique_lock lock(mutex_);
    
    auto& postings = index_[term];
    
    // // Find if document already exists in posting list
    // auto it = postings.end();
    // for (auto it = postingList.begin(); it != postingList.end(); ++it) {
    //     if (it->doc_id == docId) {
    //         existingPosting = it;
    //         break;
    //     }
    // }
    // Find if document already exists in posting list [using lambda]
    auto it = std::find_if(postings.begin(), postings.end(),
                           [doc_id](const Posting& p) { return p.doc_id == doc_id; });
    
    if (it != postings.end()) {
        // Document already exists, increment frequency and add position
        it->term_frequency++;
        if (position > 0) {
            it->positions.push_back(position);
        }
    } else {
        // New document, create posting
        Posting posting(doc_id, 1);
        if (position > 0) {
            posting.positions.push_back(position);
        }
        postings.push_back(posting);
    }
}

std::vector<Posting> InvertedIndex::getPostings(const std::string& term) const {
    std::shared_lock lock(mutex_);
    
    auto it = index_.find(term);
    if (it != index_.end()) {
        return it->second;
    }
    
    return std::vector<Posting>();
}

void InvertedIndex::removeDocument(uint64_t doc_id) {
    std::unique_lock lock(mutex_);
    
    // Iterate through all terms and remove postings for this document
    for (auto& [term, postings] : index_) {
        postings.erase(
            std::remove_if(postings.begin(), postings.end(),
                          [doc_id](const Posting& p) { return p.doc_id == doc_id; }),
            postings.end()
        );
    }
    
    // Optional: Remove terms with empty posting lists
    for (auto it = index_.begin(); it != index_.end(); ) {
        if (it->second.empty()) {
            it = index_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t InvertedIndex::getDocumentFrequency(const std::string& term) const {
    std::shared_lock lock(mutex_);
    
    auto it = index_.find(term);
    if (it != index_.end()) {
        return it->second.size();
    }
    
    return 0;
}

size_t InvertedIndex::getTermCount() const {
    std::shared_lock lock(mutex_);
    return index_.size();
}

void InvertedIndex::clear() {
    std::unique_lock lock(mutex_);
    index_.clear();
}

} 
