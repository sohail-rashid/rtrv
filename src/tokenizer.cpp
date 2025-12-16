#include "tokenizer.hpp"
#include <cctype>


namespace search_engine {

Tokenizer::Tokenizer()
    : lowercase_enabled_(true), stemmer_type_(StemmerType::NONE) {
    // TODO: Initialize default stop words
}

Tokenizer::~Tokenizer() = default;

std::vector<std::string> Tokenizer::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    tokens.reserve(text.size() / 5);

    std::string token;

    for (unsigned char c : text) {
        if (std::isalnum(c) || c == '\'') {

            if (lowercase_enabled_) {
                c = static_cast<unsigned char>(std::tolower(c));
            }

            token.push_back(static_cast<char>(c));
        }
        else if (!token.empty()) {

            if (stop_words_.find(token) == stop_words_.end()) {
                tokens.push_back(token);
            }

            token.clear();
        }
    }

    // Final token
    if (!token.empty() &&
        stop_words_.find(token) == stop_words_.end()) {
        tokens.push_back(token);
    }

    return tokens;
}

void Tokenizer::setLowercase(bool enabled) {
    lowercase_enabled_ = enabled;
}

void Tokenizer::setStopWords(const std::unordered_set<std::string>& stops) {
    stop_words_ = stops;
}

void Tokenizer::setStemmer(StemmerType type) {
    stemmer_type_ = type;
}

} 
