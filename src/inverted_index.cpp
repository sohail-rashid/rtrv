#include "inverted_index.hpp"
#include <algorithm>
#include <cmath>

namespace search_engine {

Posting::Posting(uint64_t doc_id, uint32_t term_frequency)
    : doc_id(doc_id), term_frequency(term_frequency) {
}

// ==================== PostingList Implementation ====================

void PostingList::addPosting(const Posting& posting) {
    postings.push_back(posting);
    skips_dirty_ = true;
}

void PostingList::buildSkipPointers(size_t skip_interval) const {
    skip_pointers.clear();
    
    if (postings.empty()) {
        skips_dirty_ = false;
        return;
    }
    
    // Auto-calculate skip interval if not provided (use sqrt of list size)
    if (skip_interval == 0) {
        skip_interval = std::max(size_t(1), 
                                 static_cast<size_t>(std::sqrt(postings.size())));
    }
    
    // Build skip pointers at regular intervals
    for (size_t i = 0; i < postings.size(); i += skip_interval) {
        skip_pointers.emplace_back(i, postings[i].doc_id);
    }
    
    skips_dirty_ = false;
}

size_t PostingList::findSkipTarget(uint64_t target_doc_id) const {
    if (skip_pointers.empty()) {
        return 0;
    }
    
    // Binary search on skip pointers to find last skip before target
    auto it = std::lower_bound(
        skip_pointers.begin(), 
        skip_pointers.end(),
        target_doc_id,
        [](const SkipPointer& sp, uint64_t target) {
            return sp.doc_id < target;
        }
    );
    
    if (it == skip_pointers.begin()) {
        return 0;
    }
    
    // Return position of last skip pointer before or at target
    return (--it)->position;
}

// ==================== Fast AND Intersection with Skip Pointers ====================

std::vector<uint64_t> intersectWithSkips(
    const PostingList& list1, 
    const PostingList& list2
) {
    std::vector<uint64_t> result;
    size_t i = 0, j = 0;
    
    while (i < list1.postings.size() && j < list2.postings.size()) {
        uint64_t doc_id1 = list1.postings[i].doc_id;
        uint64_t doc_id2 = list2.postings[j].doc_id;
        
        if (doc_id1 == doc_id2) {
            result.push_back(doc_id1);
            ++i;
            ++j;
        } else if (doc_id1 < doc_id2) {
            // Try to skip ahead in list1
            if (!list1.skip_pointers.empty()) {
                size_t skip_pos = list1.findSkipTarget(doc_id2);
                i = std::max(i + 1, skip_pos);
            } else {
                ++i;
            }
        } else {
            // Try to skip ahead in list2
            if (!list2.skip_pointers.empty()) {
                size_t skip_pos = list2.findSkipTarget(doc_id1);
                j = std::max(j + 1, skip_pos);
            } else {
                ++j;
            }
        }
    }
    
    return result;
}

// ==================== InvertedIndex Implementation ====================

InvertedIndex::InvertedIndex() = default;

InvertedIndex::~InvertedIndex() = default;

void InvertedIndex::addTerm(const std::string& term, uint64_t doc_id, uint32_t position) {
    std::unique_lock lock(mutex_);
    
    auto& posting_list = index_[term];
    
    // Find if document already exists in posting list [using lambda]
    auto it = std::find_if(posting_list.postings.begin(), posting_list.postings.end(),
                           [doc_id](const Posting& p) { return p.doc_id == doc_id; });
    
    if (it != posting_list.postings.end()) {
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
        posting_list.addPosting(posting);
    }
    
    // Mark skip pointers as dirty (will rebuild on next query if needed)
    posting_list.markSkipsDirty();
}

std::vector<Posting> InvertedIndex::getPostings(const std::string& term) const {
    std::shared_lock lock(mutex_);
    
    auto it = index_.find(term);
    if (it != index_.end()) {
        return it->second.postings;
    }
    
    return std::vector<Posting>();
}

PostingList InvertedIndex::getPostingList(const std::string& term) const {
    std::shared_lock lock(mutex_);
    
    auto it = index_.find(term);
    if (it != index_.end()) {
        PostingList list = it->second;
        
        // Build skip pointers if needed (on first access after updates)
        if (list.needsSkipRebuild() && !list.postings.empty()) {
            // Make a copy and build skip pointers on the copy
            list.buildSkipPointers();
        }
        
        return list;
    }
    
    return PostingList();
}

void InvertedIndex::removeDocument(uint64_t doc_id) {
    std::unique_lock lock(mutex_);
    
    // Iterate through all terms and remove postings for this document
    for (auto& [term, posting_list] : index_) {
        auto& postings = posting_list.postings;
        postings.erase(
            std::remove_if(postings.begin(), postings.end(),
                          [doc_id](const Posting& p) { return p.doc_id == doc_id; }),
            postings.end()
        );
        
        // Mark skip pointers as dirty if we removed any postings
        if (!postings.empty()) {
            posting_list.markSkipsDirty();
        }
    }
    
    // Remove terms with empty posting lists
    for (auto it = index_.begin(); it != index_.end(); ) {
        if (it->second.postings.empty()) {
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
        return it->second.postings.size();
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

void InvertedIndex::rebuildSkipPointers() {
    std::unique_lock lock(mutex_);
    
    for (auto& [term, posting_list] : index_) {
        if (!posting_list.postings.empty()) {
            posting_list.buildSkipPointers();
        }
    }
}

void InvertedIndex::rebuildSkipPointers(const std::string& term) {
    std::unique_lock lock(mutex_);
    
    auto it = index_.find(term);
    if (it != index_.end() && !it->second.postings.empty()) {
        it->second.buildSkipPointers();
    }
}

std::unordered_set<std::string> InvertedIndex::getVocabulary() const {
    std::shared_lock lock(mutex_);
    
    std::unordered_set<std::string> vocabulary;
    vocabulary.reserve(index_.size());
    for (const auto& [term, _] : index_) {
        vocabulary.insert(term);
    }
    return vocabulary;
}

bool InvertedIndex::hasTerm(const std::string& term) const {
    std::shared_lock lock(mutex_);
    return index_.count(term) > 0;
}

} 
