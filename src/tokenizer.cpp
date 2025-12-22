#include "tokenizer.hpp"
#include <cctype>
#include <algorithm>

// SIMD headers
#if defined(__SSE4_2__) || defined(__AVX2__)
    #include <emmintrin.h>  // SSE2
    #include <smmintrin.h>  // SSE4.2
    #ifdef __AVX2__
        #include <immintrin.h>  // AVX2
    #endif
#elif defined(__ARM_NEON) || defined(__aarch64__)
    #include <arm_neon.h>  // ARM NEON
#endif

namespace search_engine {

Tokenizer::Tokenizer()
    : lowercase_enabled_(true),
      remove_stopwords_(true),
      simd_enabled_(false),
      stemmer_type_(StemmerType::NONE) {
    initializeDefaultStopWords();
    
    // Auto-enable SIMD if supported
    if (detectSIMDSupport()) {
        simd_enabled_ = true;
    }
}

Tokenizer::~Tokenizer() = default;

void Tokenizer::initializeDefaultStopWords() {
    // Common English stop words
    stop_words_ = {
        "a", "an", "and", "are", "as", "at", "be", "by", "for",
        "from", "has", "he", "in", "is", "it", "its", "of", "on",
        "that", "the", "to", "was", "will", "with", "the", "this",
        "but", "they", "have", "had", "what", "when", "where", "who",
        "which", "why", "how", "all", "each", "every", "both", "few",
        "more", "most", "other", "some", "such", "no", "nor", "not",
        "only", "own", "same", "so", "than", "too", "very", "can",
        "just", "should", "now"
    };
}

std::vector<std::string> Tokenizer::tokenize(const std::string& text) {
    // Use position-aware tokenization and extract just the text
    auto tokens_with_pos = tokenizeWithPositions(text);
    std::vector<std::string> tokens;
    tokens.reserve(tokens_with_pos.size());
    
    for (const auto& token : tokens_with_pos) {
        tokens.push_back(token.text);
    }
    
    return tokens;
}

std::vector<Token> Tokenizer::tokenizeWithPositions(const std::string& text) {
    std::vector<Token> tokens;
    tokens.reserve(text.size() / 6);  // Estimate: avg 5 chars + space
    
    if (text.empty()) return tokens;
    
    // Create mutable copy for SIMD normalization
    std::string normalized_text = text;
    
    // Apply SIMD normalization if enabled
    if (lowercase_enabled_) {
        if (simd_enabled_) {
            normalizeSIMD(&normalized_text[0], normalized_text.size());
        } else {
            normalizeScalar(&normalized_text[0], normalized_text.size());
        }
    }
    
    // Use SIMD character classification for faster tokenization
    if (simd_enabled_ && normalized_text.size() >= 16) {
        std::vector<uint8_t> char_types(normalized_text.size());
        classifyCharactersSIMD(normalized_text.c_str(), char_types.data(), normalized_text.size());
        
        uint32_t position = 0;
        uint32_t token_start = 0;
        std::string current_token;
        
        for (size_t i = 0; i < normalized_text.size(); ++i) {
            if (char_types[i] == 1) {  // Alphanumeric
                if (current_token.empty()) {
                    token_start = static_cast<uint32_t>(i);
                }
                current_token += normalized_text[i];
            } else if (!current_token.empty()) {  // End of token
                // Apply post-processing
                if (!remove_stopwords_ || !isStopword(current_token)) {
                    std::string final_token = (stemmer_type_ != StemmerType::NONE) 
                        ? applyStemming(current_token) 
                        : current_token;
                    
                    tokens.push_back({
                        final_token,
                        position,
                        token_start,
                        static_cast<uint32_t>(i)
                    });
                    position++;
                }
                current_token.clear();
            }
        }
        
        // Handle last token
        if (!current_token.empty()) {
            if (!remove_stopwords_ || !isStopword(current_token)) {
                std::string final_token = (stemmer_type_ != StemmerType::NONE)
                    ? applyStemming(current_token)
                    : current_token;
                
                tokens.push_back({
                    final_token,
                    position,
                    token_start,
                    static_cast<uint32_t>(normalized_text.size())
                });
            }
        }
    } else {
        // Fallback to scalar tokenization
        uint32_t position = 0;
        uint32_t char_offset = 0;
        std::string current_token;
        uint32_t token_start = 0;
        
        for (size_t i = 0; i < normalized_text.size(); ++i) {
            char c = normalized_text[i];
            
            // Accept alphanumeric and apostrophes (for contractions)
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '\'') {
                if (current_token.empty()) {
                    token_start = char_offset;
                }
                current_token += c;
            } else if (!current_token.empty()) {
                // End of token - apply post-processing
                if (!remove_stopwords_ || !isStopword(current_token)) {
                    // Apply stemming if enabled
                    std::string final_token = (stemmer_type_ != StemmerType::NONE) 
                        ? applyStemming(current_token) 
                        : current_token;
                    
                    tokens.push_back({
                        final_token,
                        position,
                        token_start,
                        char_offset
                    });
                    position++;
                }
                current_token.clear();
            }
            
            char_offset++;
        }
        
        // Handle last token
        if (!current_token.empty()) {
            if (!remove_stopwords_ || !isStopword(current_token)) {
                std::string final_token = (stemmer_type_ != StemmerType::NONE)
                    ? applyStemming(current_token)
                    : current_token;
                
                tokens.push_back({
                    final_token,
                    position,
                    token_start,
                    char_offset
                });
            }
        }
    }
    
    return tokens;
}

bool Tokenizer::isStopword(const std::string& term) const {
    return stop_words_.find(term) != stop_words_.end();
}

void Tokenizer::normalizeScalar(char* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        if (data[i] >= 'A' && data[i] <= 'Z') {
            data[i] += 32;  // Convert to lowercase
        }
    }
}

void Tokenizer::normalizeSIMD(char* data, size_t length) {
#ifdef __AVX2__
    // AVX2 implementation: 32 bytes per iteration
    const __m256i upper_A = _mm256_set1_epi8('A');
    const __m256i upper_Z = _mm256_set1_epi8('Z');
    const __m256i to_lower = _mm256_set1_epi8(32);
    
    size_t i = 0;
    for (; i + 32 <= length; i += 32) {
        __m256i chunk = _mm256_loadu_si256((__m256i*)(data + i));
        
        // Check if character is uppercase: A <= c <= Z
        __m256i ge_A = _mm256_cmpgt_epi8(chunk, _mm256_sub_epi8(upper_A, _mm256_set1_epi8(1)));
        __m256i le_Z = _mm256_cmpgt_epi8(_mm256_add_epi8(upper_Z, _mm256_set1_epi8(1)), chunk);
        __m256i is_upper = _mm256_and_si256(ge_A, le_Z);
        
        // Convert to lowercase: c += 32 if uppercase
        __m256i lower_mask = _mm256_and_si256(is_upper, to_lower);
        chunk = _mm256_add_epi8(chunk, lower_mask);
        
        _mm256_storeu_si256((__m256i*)(data + i), chunk);
    }
    
    // Handle remaining bytes with scalar
    normalizeScalar(data + i, length - i);
    
#elif defined(__SSE4_2__)
    // SSE4.2 implementation: 16 bytes per iteration
    const __m128i upper_A = _mm_set1_epi8('A');
    const __m128i upper_Z = _mm_set1_epi8('Z');
    const __m128i to_lower = _mm_set1_epi8(32);
    
    size_t i = 0;
    for (; i + 16 <= length; i += 16) {
        __m128i chunk = _mm_loadu_si128((__m128i*)(data + i));
        
        // Find uppercase letters: A <= c <= Z
        __m128i ge_A = _mm_cmpgt_epi8(chunk, _mm_sub_epi8(upper_A, _mm_set1_epi8(1)));
        __m128i le_Z = _mm_cmpgt_epi8(_mm_add_epi8(upper_Z, _mm_set1_epi8(1)), chunk);
        __m128i is_upper = _mm_and_si128(ge_A, le_Z);
        
        // Convert uppercase to lowercase: c += 32 if uppercase
        __m128i lower_mask = _mm_and_si128(is_upper, to_lower);
        chunk = _mm_add_epi8(chunk, lower_mask);
        
        _mm_storeu_si128((__m128i*)(data + i), chunk);
    }
    
    // Handle remaining bytes with scalar
    normalizeScalar(data + i, length - i);
    
#elif defined(__ARM_NEON) || defined(__aarch64__)
    // ARM NEON implementation: 16 bytes per iteration (like SSE)
    const uint8x16_t upper_A = vdupq_n_u8('A');
    const uint8x16_t upper_Z = vdupq_n_u8('Z');
    const uint8x16_t to_lower = vdupq_n_u8(32);
    
    size_t i = 0;
    for (; i + 16 <= length; i += 16) {
        uint8x16_t chunk = vld1q_u8(reinterpret_cast<uint8_t*>(data + i));
        
        // Find uppercase letters: A <= c <= Z
        uint8x16_t ge_A = vcgeq_u8(chunk, upper_A);
        uint8x16_t le_Z = vcleq_u8(chunk, upper_Z);
        uint8x16_t is_upper = vandq_u8(ge_A, le_Z);
        
        // Convert uppercase to lowercase: c += 32 if uppercase
        uint8x16_t lower_mask = vandq_u8(is_upper, to_lower);
        chunk = vaddq_u8(chunk, lower_mask);
        
        vst1q_u8(reinterpret_cast<uint8_t*>(data + i), chunk);
    }
    
    // Handle remaining bytes with scalar
    normalizeScalar(data + i, length - i);
    
#else
    // Fallback to scalar implementation
    normalizeScalar(data, length);
#endif
}

// ============================================================================
// SIMD Character Classification
// ============================================================================

void Tokenizer::classifyCharactersScalar(const char* data, uint8_t* types, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        if (std::isalnum(c) || c == '\'') {
            types[i] = 1; // alphanumeric
        } else if (std::isspace(c)) {
            types[i] = 2; // whitespace
        } else {
            types[i] = 0; // other (punctuation, etc.)
        }
    }
}

void Tokenizer::classifyCharactersSIMD(const char* data, uint8_t* types, size_t length) {
#ifdef __AVX2__
    // AVX2: 32 bytes per iteration
    const __m256i zero = _mm256_setzero_si256();
    const __m256i one = _mm256_set1_epi8(1);
    const __m256i two = _mm256_set1_epi8(2);
    
    // Character ranges
    const __m256i lower_a = _mm256_set1_epi8('a');
    const __m256i lower_z = _mm256_set1_epi8('z');
    const __m256i upper_A = _mm256_set1_epi8('A');
    const __m256i upper_Z = _mm256_set1_epi8('Z');
    const __m256i digit_0 = _mm256_set1_epi8('0');
    const __m256i digit_9 = _mm256_set1_epi8('9');
    const __m256i apostrophe = _mm256_set1_epi8('\'');
    const __m256i space = _mm256_set1_epi8(' ');
    const __m256i tab = _mm256_set1_epi8('\t');
    const __m256i newline = _mm256_set1_epi8('\n');
    const __m256i cr = _mm256_set1_epi8('\r');
    
    size_t i = 0;
    for (; i + 32 <= length; i += 32) {
        __m256i chunk = _mm256_loadu_si256((__m256i*)(data + i));
        
        // Check for lowercase letters (a-z)
        __m256i is_lower = _mm256_and_si256(
            _mm256_cmpgt_epi8(chunk, _mm256_sub_epi8(lower_a, one)),
            _mm256_cmpgt_epi8(_mm256_add_epi8(lower_z, one), chunk)
        );
        
        // Check for uppercase letters (A-Z)
        __m256i is_upper = _mm256_and_si256(
            _mm256_cmpgt_epi8(chunk, _mm256_sub_epi8(upper_A, one)),
            _mm256_cmpgt_epi8(_mm256_add_epi8(upper_Z, one), chunk)
        );
        
        // Check for digits (0-9)
        __m256i is_digit = _mm256_and_si256(
            _mm256_cmpgt_epi8(chunk, _mm256_sub_epi8(digit_0, one)),
            _mm256_cmpgt_epi8(_mm256_add_epi8(digit_9, one), chunk)
        );
        
        // Check for apostrophe
        __m256i is_apostrophe = _mm256_cmpeq_epi8(chunk, apostrophe);
        
        // Alphanumeric = lowercase | uppercase | digit | apostrophe
        __m256i is_alnum = _mm256_or_si256(
            _mm256_or_si256(is_lower, is_upper),
            _mm256_or_si256(is_digit, is_apostrophe)
        );
        
        // Check for whitespace
        __m256i is_space = _mm256_cmpeq_epi8(chunk, space);
        __m256i is_tab = _mm256_cmpeq_epi8(chunk, tab);
        __m256i is_newline = _mm256_cmpeq_epi8(chunk, newline);
        __m256i is_cr = _mm256_cmpeq_epi8(chunk, cr);
        __m256i is_ws = _mm256_or_si256(
            _mm256_or_si256(is_space, is_tab),
            _mm256_or_si256(is_newline, is_cr)
        );
        
        // Classify: 1 for alnum, 2 for whitespace, 0 for other
        __m256i result = _mm256_and_si256(is_alnum, one);
        result = _mm256_or_si256(result, _mm256_and_si256(is_ws, two));
        
        _mm256_storeu_si256((__m256i*)(types + i), result);
    }
    
    // Handle remaining bytes
    classifyCharactersScalar(data + i, types + i, length - i);
    
#elif defined(__SSE4_2__)
    // SSE4.2: 16 bytes per iteration
    const __m128i zero = _mm_setzero_si128();
    const __m128i one = _mm_set1_epi8(1);
    const __m128i two = _mm_set1_epi8(2);
    
    const __m128i lower_a = _mm_set1_epi8('a');
    const __m128i lower_z = _mm_set1_epi8('z');
    const __m128i upper_A = _mm_set1_epi8('A');
    const __m128i upper_Z = _mm_set1_epi8('Z');
    const __m128i digit_0 = _mm_set1_epi8('0');
    const __m128i digit_9 = _mm_set1_epi8('9');
    const __m128i apostrophe = _mm_set1_epi8('\'');
    const __m128i space = _mm_set1_epi8(' ');
    const __m128i tab = _mm_set1_epi8('\t');
    const __m128i newline = _mm_set1_epi8('\n');
    const __m128i cr = _mm_set1_epi8('\r');
    
    size_t i = 0;
    for (; i + 16 <= length; i += 16) {
        __m128i chunk = _mm_loadu_si128((__m128i*)(data + i));
        
        // Check for lowercase letters
        __m128i is_lower = _mm_and_si128(
            _mm_cmpgt_epi8(chunk, _mm_sub_epi8(lower_a, one)),
            _mm_cmpgt_epi8(_mm_add_epi8(lower_z, one), chunk)
        );
        
        // Check for uppercase letters
        __m128i is_upper = _mm_and_si128(
            _mm_cmpgt_epi8(chunk, _mm_sub_epi8(upper_A, one)),
            _mm_cmpgt_epi8(_mm_add_epi8(upper_Z, one), chunk)
        );
        
        // Check for digits
        __m128i is_digit = _mm_and_si128(
            _mm_cmpgt_epi8(chunk, _mm_sub_epi8(digit_0, one)),
            _mm_cmpgt_epi8(_mm_add_epi8(digit_9, one), chunk)
        );
        
        // Check for apostrophe
        __m128i is_apostrophe = _mm_cmpeq_epi8(chunk, apostrophe);
        
        // Alphanumeric
        __m128i is_alnum = _mm_or_si128(
            _mm_or_si128(is_lower, is_upper),
            _mm_or_si128(is_digit, is_apostrophe)
        );
        
        // Whitespace
        __m128i is_space = _mm_cmpeq_epi8(chunk, space);
        __m128i is_tab = _mm_cmpeq_epi8(chunk, tab);
        __m128i is_newline = _mm_cmpeq_epi8(chunk, newline);
        __m128i is_cr = _mm_cmpeq_epi8(chunk, cr);
        __m128i is_ws = _mm_or_si128(
            _mm_or_si128(is_space, is_tab),
            _mm_or_si128(is_newline, is_cr)
        );
        
        // Classify
        __m128i result = _mm_and_si128(is_alnum, one);
        result = _mm_or_si128(result, _mm_and_si128(is_ws, two));
        
        _mm_storeu_si128((__m128i*)(types + i), result);
    }
    
    classifyCharactersScalar(data + i, types + i, length - i);
    
#elif defined(__ARM_NEON) || defined(__aarch64__)
    // ARM NEON: 16 bytes per iteration
    const uint8x16_t one = vdupq_n_u8(1);
    const uint8x16_t two = vdupq_n_u8(2);
    
    const uint8x16_t lower_a = vdupq_n_u8('a');
    const uint8x16_t lower_z = vdupq_n_u8('z');
    const uint8x16_t upper_A = vdupq_n_u8('A');
    const uint8x16_t upper_Z = vdupq_n_u8('Z');
    const uint8x16_t digit_0 = vdupq_n_u8('0');
    const uint8x16_t digit_9 = vdupq_n_u8('9');
    const uint8x16_t apostrophe = vdupq_n_u8('\'');
    const uint8x16_t space = vdupq_n_u8(' ');
    const uint8x16_t tab = vdupq_n_u8('\t');
    const uint8x16_t newline = vdupq_n_u8('\n');
    const uint8x16_t cr = vdupq_n_u8('\r');
    
    size_t i = 0;
    for (; i + 16 <= length; i += 16) {
        uint8x16_t chunk = vld1q_u8(reinterpret_cast<const uint8_t*>(data + i));
        
        // Check for lowercase letters (a-z)
        uint8x16_t is_lower = vandq_u8(
            vcgeq_u8(chunk, lower_a),
            vcleq_u8(chunk, lower_z)
        );
        
        // Check for uppercase letters (A-Z)
        uint8x16_t is_upper = vandq_u8(
            vcgeq_u8(chunk, upper_A),
            vcleq_u8(chunk, upper_Z)
        );
        
        // Check for digits (0-9)
        uint8x16_t is_digit = vandq_u8(
            vcgeq_u8(chunk, digit_0),
            vcleq_u8(chunk, digit_9)
        );
        
        // Check for apostrophe
        uint8x16_t is_apostrophe = vceqq_u8(chunk, apostrophe);
        
        // Alphanumeric = lowercase | uppercase | digit | apostrophe
        uint8x16_t is_alnum = vorrq_u8(
            vorrq_u8(is_lower, is_upper),
            vorrq_u8(is_digit, is_apostrophe)
        );
        
        // Check for whitespace
        uint8x16_t is_space = vceqq_u8(chunk, space);
        uint8x16_t is_tab = vceqq_u8(chunk, tab);
        uint8x16_t is_newline = vceqq_u8(chunk, newline);
        uint8x16_t is_cr = vceqq_u8(chunk, cr);
        uint8x16_t is_ws = vorrq_u8(
            vorrq_u8(is_space, is_tab),
            vorrq_u8(is_newline, is_cr)
        );
        
        // Classify: 1 for alnum, 2 for whitespace, 0 for other
        uint8x16_t result = vandq_u8(is_alnum, one);
        result = vorrq_u8(result, vandq_u8(is_ws, two));
        
        vst1q_u8(types + i, result);
    }
    
    classifyCharactersScalar(data + i, types + i, length - i);
    
#else
    classifyCharactersScalar(data, types, length);
#endif
}

// ============================================================================
// SIMD String Comparison
// ============================================================================

bool Tokenizer::equalsScalar(const char* a, const char* b, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool Tokenizer::equalsSIMD(const char* a, const char* b, size_t length) {
#ifdef __AVX2__
    size_t i = 0;
    for (; i + 32 <= length; i += 32) {
        __m256i chunk_a = _mm256_loadu_si256((__m256i*)(a + i));
        __m256i chunk_b = _mm256_loadu_si256((__m256i*)(b + i));
        __m256i cmp = _mm256_cmpeq_epi8(chunk_a, chunk_b);
        
        // If not all bytes match, return false
        if (_mm256_movemask_epi8(cmp) != -1) {
            return false;
        }
    }
    
    return equalsScalar(a + i, b + i, length - i);
    
#elif defined(__SSE4_2__)
    size_t i = 0;
    for (; i + 16 <= length; i += 16) {
        __m128i chunk_a = _mm_loadu_si128((__m128i*)(a + i));
        __m128i chunk_b = _mm_loadu_si128((__m128i*)(b + i));
        __m128i cmp = _mm_cmpeq_epi8(chunk_a, chunk_b);
        
        if (_mm_movemask_epi8(cmp) != 0xFFFF) {
            return false;
        }
    }
    
    return equalsScalar(a + i, b + i, length - i);
    
#elif defined(__ARM_NEON) || defined(__aarch64__)
    size_t i = 0;
    for (; i + 16 <= length; i += 16) {
        uint8x16_t chunk_a = vld1q_u8(reinterpret_cast<const uint8_t*>(a + i));
        uint8x16_t chunk_b = vld1q_u8(reinterpret_cast<const uint8_t*>(b + i));
        uint8x16_t cmp = vceqq_u8(chunk_a, chunk_b);
        
        // Check if all bytes match (all bits set to 1)
        uint64x2_t cmp64 = vreinterpretq_u64_u8(cmp);
        if (vgetq_lane_u64(cmp64, 0) != 0xFFFFFFFFFFFFFFFFULL ||
            vgetq_lane_u64(cmp64, 1) != 0xFFFFFFFFFFFFFFFFULL) {
            return false;
        }
    }
    
    return equalsScalar(a + i, b + i, length - i);
    
#else
    return equalsScalar(a, b, length);
#endif
}

std::string Tokenizer::applyStemming(const std::string& token) {
    switch (stemmer_type_) {
        case StemmerType::SIMPLE:
            return simpleStem(token);
        case StemmerType::PORTER:
            // TODO: Implement Porter Stemmer
            return token;
        case StemmerType::NONE:
        default:
            return token;
    }
}

std::string Tokenizer::simpleStem(const std::string& token) {
    if (token.length() < 4) {
        return token;  // Too short to stem
    }
    
    std::string result = token;
    
    // Remove common suffixes (order matters - check more specific patterns first)
    // Note: "tional" must be checked before "ational" to avoid incorrect matching
    if (result.length() > 6 && result.substr(result.length() - 6) == "tional") {
        result = result.substr(0, result.length() - 6) + "tion";  // "national" -> "nation"
    } else if (result.length() > 7 && result.substr(result.length() - 7) == "ational") {
        result = result.substr(0, result.length() - 7) + "ate";  // "relational" -> "relate"
    } else if (result.length() > 5 && result.substr(result.length() - 5) == "ional") {
        result = result.substr(0, result.length() - 2);  // "educational" -> "education" (remove "al")
    } else if (result.length() > 3 && result.substr(result.length() - 3) == "ing") {
        result = result.substr(0, result.length() - 3);  // "running" -> "runn"
    } else if (result.length() > 2 && result.substr(result.length() - 2) == "ed") {
        result = result.substr(0, result.length() - 2);  // "walked" -> "walk"
    } else if (result.length() > 2 && result.substr(result.length() - 2) == "ly") {
        result = result.substr(0, result.length() - 2);  // "quickly" -> "quick"
    } else if (result.length() > 1 && result[result.length() - 1] == 's' && 
               result[result.length() - 2] != 's') {
        result = result.substr(0, result.length() - 1);  // "cats" -> "cat"
    }
    
    return result;
}

void Tokenizer::setLowercase(bool enabled) {
    lowercase_enabled_ = enabled;
}

void Tokenizer::setStopWords(const std::unordered_set<std::string>& stops) {
    stop_words_ = stops;
}

void Tokenizer::setRemoveStopwords(bool enabled) {
    remove_stopwords_ = enabled;
}

void Tokenizer::setStemmer(StemmerType type) {
    stemmer_type_ = type;
}

void Tokenizer::enableSIMD(bool enabled) {
    if (enabled && !detectSIMDSupport()) {
        // Can't enable SIMD if hardware doesn't support it
        simd_enabled_ = false;
    } else {
        simd_enabled_ = enabled;
    }
}

bool Tokenizer::detectSIMDSupport() {
#ifdef __AVX2__
    return true;
#elif defined(__SSE4_2__)
    return true;
#elif defined(__ARM_NEON) || defined(__aarch64__)
    return true;
#else
    return false;
#endif
}

} 
