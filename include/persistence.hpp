#pragma once

#include <string>

namespace rtrv_search_engine {

class SearchEngine;

/**
 * Snapshot file format header
 */
struct SnapshotHeader {
    uint32_t magic = 0x53454152;  // "SEAR"
    uint32_t version = 1;
    uint64_t num_documents;
    uint64_t num_terms;
};
// Format:
// [Header]                    // SnapshotHeader (magic, version, num_documents, num_terms)
// [next_doc_id]              // uint64_t for ID generation
// [Document1]...[DocumentN]  // Each document: doc_id, content_len, content, term_count, metadata
// [num_index_terms]          // Size of index
// [Term1][PostingList1]...   // Each term: term_len, term, postings_count, then postings


/**
 * Handles persistence of search engine state
 */
class Persistence {
public:
    /**
     * Save search engine state to file
     */
    static bool save(const SearchEngine& engine, const std::string& filepath);
    
    /**
     * Load search engine state from file
     */
    static bool load(SearchEngine& engine, const std::string& filepath);
};

}
