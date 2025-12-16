#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace search_engine {

/**
 * Represents a searchable document with metadata
 */
struct Document {
    uint64_t id;                                             // Unique document ID
    std::string content;                                     // Full text content
    std::unordered_map<std::string, std::string> metadata;   // Document metadata
    size_t term_count;                                       // Cached for BM25
    
    Document() = default;
    Document(uint64_t id, const std::string& content);
};

} 
