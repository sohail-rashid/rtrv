#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace search_engine {

/**
 * Available stemmer types
 */
enum class StemmerType {
    NONE,
    PORTER,
    SIMPLE
};

/**
 * Converts raw text into searchable terms
 */
class Tokenizer {
public:
    Tokenizer();
    ~Tokenizer();
    
    /**
     * Tokenize text into terms
     */
    std::vector<std::string> tokenize(const std::string& text);
    
    /**
     * Enable/disable lowercase normalization
     */
    void setLowercase(bool enabled);
    
    /**
     * Set stop words to filter
     */
    void setStopWords(const std::unordered_set<std::string>& stops);
    
    /**
     * Set stemmer type
     */
    void setStemmer(StemmerType type);
    
private:
    bool lowercase_enabled_;
    std::unordered_set<std::string> stop_words_;
    StemmerType stemmer_type_;
};

} 
