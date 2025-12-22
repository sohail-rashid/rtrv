#include "document.hpp"
#include <sstream>

namespace search_engine {

Document::Document(uint32_t id, const std::unordered_map<std::string, std::string>& fields)
    : id(id), fields(fields), term_count(0) {
}

std::string Document::getField(const std::string& field_name) const {
    auto it = fields.find(field_name);
    if (it != fields.end()) {
        return it->second;
    }
    return "";
}

std::string Document::getAllText() const {
    std::ostringstream oss;
    bool first = true;
    
    for (const auto& [field_name, field_value] : fields) {
        if (!first) {
            oss << " ";
        }
        oss << field_value;
        first = false;
    }
    
    return oss.str();
}

} // namespace search_engine 