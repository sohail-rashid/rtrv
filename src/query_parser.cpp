#include "query_parser.hpp"
#include <sstream>
#include <cctype>

namespace search_engine {

QueryNode::QueryNode(QueryType type, const std::string& value)
    : type(type), value(value) {
}

QueryParser::QueryParser() = default;

QueryParser::~QueryParser() = default;

std::unique_ptr<QueryNode> QueryParser::parse(const std::string& query_string) {
    // For now, implement simple parsing without full boolean logic
    // This creates a basic AST that can be extended later
    
    // Check for phrases (quoted strings)
    if (query_string.find('"') != std::string::npos) {
        size_t start = query_string.find('"');
        size_t end = query_string.find('"', start + 1);
        if (end != std::string::npos) {
            std::string phrase = query_string.substr(start + 1, end - start - 1);
            auto node = std::make_unique<QueryNode>(QueryType::PHRASE, phrase);
            return node;
        }
    }
    
    // Check for AND operator
    size_t and_pos = query_string.find(" AND ");
    if (and_pos != std::string::npos) {
        auto node = std::make_unique<QueryNode>(QueryType::AND);
        node->children.push_back(parse(query_string.substr(0, and_pos)));
        node->children.push_back(parse(query_string.substr(and_pos + 5))); // +5 for " AND "
        return node;
    }
    
    // Check for OR operator
    size_t or_pos = query_string.find(" OR ");
    if (or_pos != std::string::npos) {
        auto node = std::make_unique<QueryNode>(QueryType::OR);
        node->children.push_back(parse(query_string.substr(0, or_pos)));
        node->children.push_back(parse(query_string.substr(or_pos + 4))); // +4 for " OR "
        return node;
    }
    
    // Check for NOT operator
    if (query_string.find("NOT ") == 0) {
        auto node = std::make_unique<QueryNode>(QueryType::NOT);
        node->children.push_back(parse(query_string.substr(4))); // +4 for "NOT "
        return node;
    }
    
    // Default: treat as single term
    auto node = std::make_unique<QueryNode>(QueryType::TERM, query_string);
    return node;
}

std::vector<std::string> QueryParser::extractTerms(const std::string& query_string) {
    std::vector<std::string> terms;
    std::string current_term;
    bool in_quotes = false;
    
    for (size_t i = 0; i < query_string.length(); ++i) {
        char c = query_string[i];
        
        // Handle quoted phrases
        if (c == '"') {
            in_quotes = !in_quotes;
            continue;
        }
        
        // Inside quotes, keep everything including spaces
        if (in_quotes) {
            current_term += std::tolower(c);
            continue;
        }
        
        // Outside quotes, split on whitespace and punctuation
        if (std::isspace(c) || std::ispunct(c)) {
            if (!current_term.empty()) {
                // Skip boolean operators
                if (current_term != "and" && current_term != "or" && current_term != "not") {
                    terms.push_back(current_term);
                }
                current_term.clear();
            }
        } else {
            current_term += std::tolower(c);
        }
    }
    
    // Add last term if any
    if (!current_term.empty()) {
        if (current_term != "and" && current_term != "or" && current_term != "not") {
            terms.push_back(current_term);
        }
    }
    
    return terms;
}

} 
