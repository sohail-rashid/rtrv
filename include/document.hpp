#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace rtrv_search_engine {

/**
 * Represents a searchable document with metadata
 */
struct Document {
    uint32_t id;                                             // Unique document ID (supports 4B docs)
    std::unordered_map<std::string, std::string> fields;     // Field-based storage
    size_t term_count;                                       // Cached for BM25
    
    Document() = default;
    Document(uint32_t id, const std::unordered_map<std::string, std::string>& fields);

    std::string getField(const std::string& field_name) const;
    std::string getAllText() const;
};

} 
