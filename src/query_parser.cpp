#include "query_parser.hpp"
#include <sstream>

namespace search_engine {

QueryNode::QueryNode(QueryType type, const std::string& value)
    : type(type), value(value) {
}

QueryParser::QueryParser() = default;

QueryParser::~QueryParser() = default;

std::unique_ptr<QueryNode> QueryParser::parse(const std::string& query_string) {
    // TODO: Implement query parsing with boolean operators
    // Should handle: AND, OR, NOT, phrases in quotes
    auto root = std::make_unique<QueryNode>(QueryType::TERM, query_string);
    return root;
}

std::vector<std::string> QueryParser::extractTerms(const std::string& query_string) {
    // TODO: Implement simple term extraction
    // For now, just split on whitespace
    std::vector<std::string> terms;
    std::istringstream iss(query_string);
    std::string term;
    while (iss >> term) {
        terms.push_back(term);
    }
    return terms;
}

} // namespace search_engine
