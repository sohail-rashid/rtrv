#include "inverted_index.hpp"
#include <algorithm>

namespace search_engine {

Posting::Posting(uint64_t doc_id, uint32_t term_frequency)
    : doc_id(doc_id), term_frequency(term_frequency) {
}

InvertedIndex::InvertedIndex() = default;

InvertedIndex::~InvertedIndex() = default;

void InvertedIndex::addTerm(const std::string& term, uint64_t doc_id, uint32_t position) {
    // TODO: Implement thread-safe term addition with write lock
}

std::vector<Posting> InvertedIndex::getPostings(const std::string& term) const {
    // TODO: Implement thread-safe posting retrieval with read lock
    std::vector<Posting> postings;
    return postings;
}

void InvertedIndex::removeDocument(uint64_t doc_id) {
    // TODO: Implement thread-safe document removal
}

size_t InvertedIndex::getDocumentFrequency(const std::string& term) const {
    // TODO: Implement document frequency calculation
    return 0;
}

size_t InvertedIndex::getTermCount() const {
    // TODO: Implement term count
    std::shared_lock lock(mutex_);
    return index_.size();
}

void InvertedIndex::clear() {
    // TODO: Implement index clearing
    std::unique_lock lock(mutex_);
    index_.clear();
}

} // namespace search_engine
