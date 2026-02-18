#pragma once

#include "document.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace rtrv_search_engine {

class DocumentLoader {
public:
    // Load documents from JSONL file (JSON Lines format)
    std::vector<Document> loadJSONL(const std::string& filepath);

    // Load documents from CSV file
    std::vector<Document> loadCSV(const std::string& filepath, 
                                  const std::vector<std::string>& column_names = {});

    // Document addition with auto-incrementing ID
    Document createDocument(const std::unordered_map<std::string, std::string>& fields);

private:
    uint32_t next_doc_id_ = 1;  // Auto-incrementing document ID (uint32_t for 4B docs)
};

} // namespace rtrv_search_engine
