#include "document.hpp"

namespace search_engine {

Document::Document(uint64_t id, const std::string& content)
    : id(id), content(content), term_count(0) {
    // term_count will be set during indexing when content is tokenized
    // metadata is default-constructed as empty map
}

} 