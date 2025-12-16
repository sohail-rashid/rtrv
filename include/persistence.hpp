#pragma once

#include <string>

namespace search_engine {

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

} // namespace search_engine
