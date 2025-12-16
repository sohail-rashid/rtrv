#pragma once

#include <string>
#include <vector>
#include <memory>

namespace search_engine {

/**
 * Query node types
 */
enum class QueryType {
    TERM,
    PHRASE,
    AND,
    OR,
    NOT
};

/**
 * Abstract syntax tree node for queries
 */
struct QueryNode {
    QueryType type;
    std::string value;
    std::vector<std::unique_ptr<QueryNode>> children;
    
    QueryNode(QueryType type, const std::string& value = "");
};

/**
 * Parses user queries into structured format
 */
class QueryParser {
public:
    QueryParser();
    ~QueryParser();
    
    /**
     * Parse query string into AST
     */
    std::unique_ptr<QueryNode> parse(const std::string& query_string);
    
    /**
     * Extract simple terms from query
     */
    std::vector<std::string> extractTerms(const std::string& query_string);
};

} // namespace search_engine
