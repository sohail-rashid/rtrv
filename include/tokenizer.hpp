#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <cstdint>

namespace rtrv_search_engine {

/**
 * Available stemmer types
 */
enum class StemmerType {
    NONE,
    PORTER,
    SIMPLE
};

/**
 * Token with position and offset information
 * Used for phrase queries and result highlighting
 */
struct Token {
    std::string text;          // The actual token text
    uint32_t position;         // Position in document (for phrase queries)
    uint32_t start_offset;     // Character offset start (for highlighting)
    uint32_t end_offset;       // Character offset end (for highlighting)
};

/**
 * Converts raw text into searchable terms
 * Supports SIMD acceleration for high-performance tokenization
 */
class Tokenizer {
public:
    Tokenizer();
    ~Tokenizer();
    
    /**
     * Basic tokenization (returns just terms)
     * Backward compatible with existing code
     */
    std::vector<std::string> tokenize(const std::string& text);
    
    /**
     * Advanced tokenization with position tracking
     * Required for phrase queries and result highlighting
     */
    std::vector<Token> tokenizeWithPositions(const std::string& text);
    
    /**
     * Enable/disable lowercase normalization
     */
    void setLowercase(bool enabled);
    
    /**
     * Set stop words to filter
     */
    void setStopWords(const std::unordered_set<std::string>& stops);
    
    /**
     * Enable/disable stop word removal
     */
    void setRemoveStopwords(bool enabled);
    
    /**
     * Set stemmer type
     */
    void setStemmer(StemmerType type);
    
    /**
     * Enable/disable SIMD acceleration
     * Automatically disabled if hardware doesn't support it
     */
    void enableSIMD(bool enabled);
    
    /**
     * Check if SIMD is available on current hardware
     */
    static bool detectSIMDSupport();
    
private:
    bool lowercase_enabled_;
    bool remove_stopwords_;
    bool simd_enabled_;
    std::unordered_set<std::string> stop_words_;
    StemmerType stemmer_type_;
    
    /**
     * Check if a term is a stop word
     */
    bool isStopword(const std::string& term) const;
    
    /**
     * SIMD-accelerated lowercase conversion
     */
    void normalizeSIMD(char* data, size_t length);
    
    /**
     * Scalar fallback for lowercase conversion
     */
    void normalizeScalar(char* data, size_t length);
    
    /**
     * SIMD character classification
     * Classifies each character: 0=other, 1=alphanumeric, 2=whitespace
     */
    void classifyCharactersSIMD(const char* data, uint8_t* types, size_t length);
    void classifyCharactersScalar(const char* data, uint8_t* types, size_t length);
    
    /**
     * SIMD string comparison (for exact matches)
     */
    bool equalsSIMD(const char* a, const char* b, size_t length);
    bool equalsScalar(const char* a, const char* b, size_t length);
    
    /**
     * Apply stemming to a token
     */
    std::string applyStemming(const std::string& token);
    
    /**
     * Simple suffix stemmer (removes common suffixes)
     */
    std::string simpleStem(const std::string& token);
    
    /**
     * Initialize default English stop words
     */
    void initializeDefaultStopWords();
};

} 
