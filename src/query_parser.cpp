#include "query_parser.hpp"
#include <sstream>
#include <cctype>
#include <algorithm>
#include <stdexcept>

namespace search_engine {

// ============================================================================
// QueryNode implementations
// ============================================================================

std::string PhraseNode::toString() const {
    std::string result = "\"";
    for (size_t i = 0; i < terms.size(); ++i) {
        if (i > 0) result += " ";
        result += terms[i];
    }
    result += "\"";
    if (max_distance > 0) {
        result += "~" + std::to_string(max_distance);
    }
    return result;
}

std::string AndNode::toString() const {
    std::string result = "AND(";
    for (size_t i = 0; i < children.size(); ++i) {
        if (i > 0) result += ", ";
        result += children[i]->toString();
    }
    result += ")";
    return result;
}

std::string OrNode::toString() const {
    std::string result = "OR(";
    for (size_t i = 0; i < children.size(); ++i) {
        if (i > 0) result += ", ";
        result += children[i]->toString();
    }
    result += ")";
    return result;
}

// ============================================================================
// QueryParser implementation
// ============================================================================

QueryParser::QueryParser() = default;

QueryParser::~QueryParser() = default;

std::string QueryParser::join(const std::vector<std::string>& vec, const std::string& delim) {
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) result += delim;
        result += vec[i];
    }
    return result;
}

void QueryParser::tokenizeQuery(const std::string& query_string) {
    tokens_.clear();
    current_position_ = 0;
    
    size_t i = 0;
    while (i < query_string.length()) {
        char c = query_string[i];
        
        // Skip whitespace
        if (std::isspace(c)) {
            ++i;
            continue;
        }
        
        // Single-character tokens
        if (c == '(') {
            tokens_.emplace_back(QueryTokenType::LPAREN, "(");
            ++i;
            continue;
        }
        if (c == ')') {
            tokens_.emplace_back(QueryTokenType::RPAREN, ")");
            ++i;
            continue;
        }
        if (c == ':') {
            tokens_.emplace_back(QueryTokenType::COLON, ":");
            ++i;
            continue;
        }
        if (c == '~') {
            tokens_.emplace_back(QueryTokenType::TILDE, "~");
            ++i;
            continue;
        }
        if (c == '"') {
            tokens_.emplace_back(QueryTokenType::QUOTE, "\"");
            ++i;
            continue;
        }
        
        // Numbers
        if (std::isdigit(c)) {
            std::string number;
            while (i < query_string.length() && std::isdigit(query_string[i])) {
                number += query_string[i];
                ++i;
            }
            tokens_.emplace_back(QueryTokenType::NUMBER, number);
            continue;
        }
        
        // Words (alphanumeric + underscore)
        if (std::isalnum(c) || c == '_') {
            std::string word;
            while (i < query_string.length() && 
                   (std::isalnum(query_string[i]) || query_string[i] == '_')) {
                word += query_string[i];
                ++i;
            }
            
            // Check for boolean operators
            std::string word_upper = word;
            std::transform(word_upper.begin(), word_upper.end(), word_upper.begin(), ::toupper);
            
            if (word_upper == "AND") {
                tokens_.emplace_back(QueryTokenType::AND_OP, word);
            } else if (word_upper == "OR") {
                tokens_.emplace_back(QueryTokenType::OR_OP, word);
            } else if (word_upper == "NOT") {
                tokens_.emplace_back(QueryTokenType::NOT_OP, word);
            } else {
                // Convert to lowercase for terms
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                tokens_.emplace_back(QueryTokenType::WORD, word);
            }
            continue;
        }
        
        // Unknown character, skip it
        ++i;
    }
    
    tokens_.emplace_back(QueryTokenType::END, "");
}

QueryToken QueryParser::peek() const {
    if (current_position_ < tokens_.size()) {
        return tokens_[current_position_];
    }
    return QueryToken(QueryTokenType::END, "");
}

QueryToken QueryParser::advance() {
    if (current_position_ < tokens_.size()) {
        return tokens_[current_position_++];
    }
    return QueryToken(QueryTokenType::END, "");
}

bool QueryParser::match(QueryTokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

bool QueryParser::isAtEnd() const {
    return current_position_ >= tokens_.size() || peek().type == QueryTokenType::END;
}

std::unique_ptr<QueryNode> QueryParser::parse(const std::string& query_string) {
    if (query_string.empty()) {
        return std::make_unique<TermNode>("");
    }
    
    tokenizeQuery(query_string);
    current_position_ = 0;
    
    try {
        auto result = parseExpression();
        if (result == nullptr) {
            // Empty query, return empty term
            return std::make_unique<TermNode>("");
        }
        return result;
    } catch (const std::exception& e) {
        // On parse error, fall back to simple term
        return std::make_unique<TermNode>(query_string);
    }
}

std::unique_ptr<QueryNode> QueryParser::parseExpression() {
    // expr ::= term_expr (AND term_expr)*
    // Implicit AND between adjacent terms
    
    if (isAtEnd()) {
        return nullptr;
    }
    
    auto left = parseTermExpression();
    if (!left) {
        return nullptr;
    }
    
    // Handle explicit AND or implicit AND (adjacent terms)
    while (!isAtEnd() && peek().type != QueryTokenType::RPAREN) {
        bool explicit_and = match(QueryTokenType::AND_OP);
        
        // Check if there's another term coming (implicit AND)
        if (!explicit_and && (peek().type == QueryTokenType::WORD || 
                              peek().type == QueryTokenType::QUOTE ||
                              peek().type == QueryTokenType::LPAREN ||
                              peek().type == QueryTokenType::NOT_OP)) {
            // Implicit AND
            explicit_and = true;
        }
        
        if (explicit_and) {
            auto right = parseTermExpression();
            if (!right) {
                break;
            }
            
            // Create or extend AND node
            if (left->getType() == QueryNode::Type::AND) {
                static_cast<AndNode*>(left.get())->addChild(std::move(right));
            } else {
                auto and_node = std::make_unique<AndNode>();
                and_node->addChild(std::move(left));
                and_node->addChild(std::move(right));
                left = std::move(and_node);
            }
        } else {
            break;
        }
    }
    
    return left;
}

std::unique_ptr<QueryNode> QueryParser::parseTermExpression() {
    // term_expr ::= factor_expr (OR factor_expr)*
    
    auto left = parseFactorExpression();
    if (!left) {
        return nullptr;
    }
    
    while (match(QueryTokenType::OR_OP)) {
        auto right = parseFactorExpression();
        if (!right) {
            break;
        }
        
        // Create or extend OR node
        if (left->getType() == QueryNode::Type::OR) {
            static_cast<OrNode*>(left.get())->addChild(std::move(right));
        } else {
            auto or_node = std::make_unique<OrNode>();
            or_node->addChild(std::move(left));
            or_node->addChild(std::move(right));
            left = std::move(or_node);
        }
    }
    
    return left;
}

std::unique_ptr<QueryNode> QueryParser::parseFactorExpression() {
    // factor_expr ::= NOT? atom
    
    if (match(QueryTokenType::NOT_OP)) {
        auto child = parseAtom();
        if (!child) {
            throw std::runtime_error("Expected expression after NOT");
        }
        return std::make_unique<NotNode>(std::move(child));
    }
    
    return parseAtom();
}

std::unique_ptr<QueryNode> QueryParser::parseAtom() {
    // atom ::= '(' expr ')' | phrase | fielded_term | term
    
    // Parenthesized expression
    if (match(QueryTokenType::LPAREN)) {
        auto expr = parseExpression();
        if (!match(QueryTokenType::RPAREN)) {
            throw std::runtime_error("Expected closing parenthesis");
        }
        return expr;
    }
    
    // Phrase query
    if (peek().type == QueryTokenType::QUOTE) {
        return parsePhrase();
    }
    
    // Check for fielded term (field:term or field:"phrase")
    if (peek().type == QueryTokenType::WORD && 
        current_position_ + 1 < tokens_.size() &&
        tokens_[current_position_ + 1].type == QueryTokenType::COLON) {
        return parseFieldedTerm();
    }
    
    // Simple term
    return parseTerm();
}

std::unique_ptr<QueryNode> QueryParser::parsePhrase() {
    // phrase ::= '"' word+ '"' proximity?
    
    if (!match(QueryTokenType::QUOTE)) {
        throw std::runtime_error("Expected opening quote");
    }
    
    std::vector<std::string> terms;
    while (peek().type == QueryTokenType::WORD) {
        terms.push_back(advance().value);
    }
    
    if (!match(QueryTokenType::QUOTE)) {
        throw std::runtime_error("Expected closing quote");
    }
    
    if (terms.empty()) {
        throw std::runtime_error("Empty phrase query");
    }
    
    // Check for proximity (~N)
    int proximity = 0;
    if (match(QueryTokenType::TILDE)) {
        if (peek().type == QueryTokenType::NUMBER) {
            proximity = std::stoi(advance().value);
        }
    }
    
    return std::make_unique<PhraseNode>(std::move(terms), proximity);
}

std::unique_ptr<QueryNode> QueryParser::parseFieldedTerm() {
    // fielded_term ::= field ':' (term | phrase)
    
    std::string field_name = advance().value;
    
    if (!match(QueryTokenType::COLON)) {
        throw std::runtime_error("Expected colon after field name");
    }
    
    std::unique_ptr<QueryNode> query;
    if (peek().type == QueryTokenType::QUOTE) {
        query = parsePhrase();
    } else {
        query = parseTerm();
    }
    
    return std::make_unique<FieldNode>(field_name, std::move(query));
}

std::unique_ptr<QueryNode> QueryParser::parseTerm() {
    // term ::= word
    
    if (peek().type != QueryTokenType::WORD) {
        return nullptr;
    }
    
    std::string term = advance().value;
    return std::make_unique<TermNode>(term);
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
