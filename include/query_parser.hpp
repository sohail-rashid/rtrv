#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sstream>

namespace search_engine {

/**
 * Abstract base class for all query nodes
 */
class QueryNode {
public:
    enum class Type {
        TERM,
        PHRASE,
        FIELD,
        AND,
        OR,
        NOT,
        PROXIMITY
    };
    
    virtual ~QueryNode() = default;
    virtual Type getType() const = 0;
    virtual std::string toString() const = 0;
};

/**
 * Terminal node representing a single search term
 */
class TermNode : public QueryNode {
public:
    std::string term;
    
    explicit TermNode(std::string t) : term(std::move(t)) {}
    Type getType() const override { return Type::TERM; }
    std::string toString() const override { return term; }
};

/**
 * Phrase query node (exact or proximity)
 */
class PhraseNode : public QueryNode {
public:
    std::vector<std::string> terms;
    int max_distance = 0;  // 0 = exact phrase, >0 = proximity
    
    PhraseNode(std::vector<std::string> t, int dist = 0)
        : terms(std::move(t)), max_distance(dist) {}
    
    Type getType() const override { return Type::PHRASE; }
    std::string toString() const override;
};

/**
 * Field-specific query node (e.g., title:machine)
 */
class FieldNode : public QueryNode {
public:
    std::string field_name;
    std::unique_ptr<QueryNode> query;
    
    FieldNode(std::string field, std::unique_ptr<QueryNode> q)
        : field_name(std::move(field)), query(std::move(q)) {}
    
    Type getType() const override { return Type::FIELD; }
    std::string toString() const override {
        return field_name + ":" + query->toString();
    }
};

/**
 * Boolean AND operator node
 */
class AndNode : public QueryNode {
public:
    std::vector<std::unique_ptr<QueryNode>> children;
    
    void addChild(std::unique_ptr<QueryNode> child) {
        children.push_back(std::move(child));
    }
    
    Type getType() const override { return Type::AND; }
    std::string toString() const override;
};

/**
 * Boolean OR operator node
 */
class OrNode : public QueryNode {
public:
    std::vector<std::unique_ptr<QueryNode>> children;
    
    void addChild(std::unique_ptr<QueryNode> child) {
        children.push_back(std::move(child));
    }
    
    Type getType() const override { return Type::OR; }
    std::string toString() const override;
};

/**
 * Boolean NOT operator node
 */
class NotNode : public QueryNode {
public:
    std::unique_ptr<QueryNode> child;
    
    explicit NotNode(std::unique_ptr<QueryNode> c) : child(std::move(c)) {}
    
    Type getType() const override { return Type::NOT; }
    std::string toString() const override {
        return "NOT(" + child->toString() + ")";
    }
};

/**
 * Token types for query lexical analysis
 */
enum class QueryTokenType {
    WORD,
    QUOTE,
    LPAREN,
    RPAREN,
    COLON,
    TILDE,
    NUMBER,
    AND_OP,
    OR_OP,
    NOT_OP,
    END
};

/**
 * Query token structure
 */
struct QueryToken {
    QueryTokenType type;
    std::string value;
    
    QueryToken(QueryTokenType t, std::string v = "") : type(t), value(std::move(v)) {}
};

/**
 * Query Parser with full AST support
 */
class QueryParser {
public:
    QueryParser();
    ~QueryParser();
    
    /**
     * Parse query string into AST
     * Supports:
     * - Simple terms: machine
     * - Phrases: "machine learning"
     * - Boolean: machine AND learning, machine OR learning, NOT deprecated
     * - Fielded: title:machine
     * - Nested: (machine OR ai) AND learning
     * - Proximity: "machine learning"~5
     * - Implicit AND: machine learning (treated as machine AND learning)
     */
    std::unique_ptr<QueryNode> parse(const std::string& query_string);
    
    /**
     * Extract simple terms from query (backward compatibility)
     */
    std::vector<std::string> extractTerms(const std::string& query_string);

private:
    std::vector<QueryToken> tokens_;
    size_t current_position_ = 0;
    
    // Tokenize query string
    void tokenizeQuery(const std::string& query_string);
    
    // Recursive descent parser (following BNF grammar)
    std::unique_ptr<QueryNode> parseExpression();
    std::unique_ptr<QueryNode> parseTermExpression();
    std::unique_ptr<QueryNode> parseFactorExpression();
    std::unique_ptr<QueryNode> parseAtom();
    std::unique_ptr<QueryNode> parsePhrase();
    std::unique_ptr<QueryNode> parseFieldedTerm();
    std::unique_ptr<QueryNode> parseTerm();
    
    // Helper methods
    QueryToken peek() const;
    QueryToken advance();
    bool match(QueryTokenType type);
    bool isAtEnd() const;
    
    // Helper to join strings
    static std::string join(const std::vector<std::string>& vec, const std::string& delim);
};

} 