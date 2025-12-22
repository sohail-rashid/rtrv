#include "document_loader.hpp"
#include "document.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace search_engine {

namespace {

// Helper function to parse CSV line with proper quote handling
std::vector<std::string> parseCsvLine(const std::string& line) {
    std::vector<std::string> result;
    std::string current;
    bool in_quotes = false;
    
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        
        if (c == '"') {
            // Check for escaped quote (two consecutive quotes)
            if (i + 1 < line.size() && line[i + 1] == '"') {
                current += '"';
                i++; // Skip next quote
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == ',' && !in_quotes) {
            result.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    
    // Add the last field
    result.push_back(current);
    
    return result;
}

} // anonymous namespace

std::vector<Document> DocumentLoader::loadJSONL(const std::string& filepath) {
    std::vector<Document> documents;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    
    std::string line;
    uint32_t line_number = 0;
    
    // Reserve space for better memory efficiency
    documents.reserve(1000);
    
    while (std::getline(file, line)) {
        line_number++;
        
        // Skip empty lines
        if (line.empty()) {
            continue;
        }
        
        try {
            nlohmann::json json_obj = nlohmann::json::parse(line);
            
            if (!json_obj.is_object()) {
                throw std::runtime_error("Line " + std::to_string(line_number) + 
                                       ": Expected JSON object, got " + json_obj.type_name());
            }
            
            Document doc;
            doc.id = next_doc_id_++;
            
            // Validate we haven't exceeded uint32_t limit
            if (next_doc_id_ == 0) {  // Overflow occurred
                throw std::runtime_error("Document ID overflow: exceeded 4 billion documents");
            }
            
            // Extract all fields
            for (auto& [key, value] : json_obj.items()) {
                if (key != "id") {  // Skip ID field if present
                    if (value.is_string()) {
                        doc.fields[key] = value.get<std::string>();
                    } else if (value.is_null()) {
                        doc.fields[key] = "";  // Store empty string for null
                    } else {
                        // Convert non-string values to strings
                        doc.fields[key] = value.dump();
                    }
                }
            }
            
            // Calculate term_count (approximate: count whitespace-separated tokens)
            std::string all_text = doc.getAllText();
            if (!all_text.empty()) {
                doc.term_count = 1;  // Start with 1 for first token
                for (char c : all_text) {
                    if (std::isspace(c)) {
                        doc.term_count++;
                    }
                }
            } else {
                doc.term_count = 0;
            }
            
            documents.push_back(std::move(doc));
            
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error("Line " + std::to_string(line_number) + 
                                   ": JSON parse error: " + std::string(e.what()));
        }
    }
    
    return documents;
}

std::vector<Document> DocumentLoader::loadCSV(
    const std::string& filepath,
    const std::vector<std::string>& column_names
) {
    std::vector<Document> documents;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    
    // Validate column_names is not empty
    if (column_names.empty()) {
        throw std::runtime_error("Column names cannot be empty for CSV loading");
    }
    
    std::string line;
    uint32_t line_number = 0;
    
    // Reserve space for better memory efficiency
    documents.reserve(1000);
    
    // Skip header
    if (std::getline(file, line)) {
        line_number++;
    } else {
        throw std::runtime_error("CSV file is empty");
    }
    
    while (std::getline(file, line)) {
        line_number++;
        
        // Skip empty lines
        if (line.empty()) {
            continue;
        }
        
        try {
            Document doc;
            doc.id = next_doc_id_++;
            
            // Validate we haven't exceeded uint32_t limit
            if (next_doc_id_ == 0) {  // Overflow occurred
                throw std::runtime_error("Document ID overflow: exceeded 4 billion documents");
            }
            
            std::vector<std::string> values = parseCsvLine(line);
            
            // Validate column count matches
            if (values.size() != column_names.size()) {
                throw std::runtime_error("Line " + std::to_string(line_number) + 
                                       ": Column count mismatch. Expected " + 
                                       std::to_string(column_names.size()) + 
                                       ", got " + std::to_string(values.size()));
            }
            
            // Extract fields
            for (size_t i = 0; i < column_names.size(); ++i) {
                if (column_names[i] != "id") {
                    doc.fields[column_names[i]] = values[i];
                }
            }
            
            // Calculate term_count (approximate: count whitespace-separated tokens)
            std::string all_text = doc.getAllText();
            if (!all_text.empty()) {
                doc.term_count = 1;  // Start with 1 for first token
                for (char c : all_text) {
                    if (std::isspace(c)) {
                        doc.term_count++;
                    }
                }
            } else {
                doc.term_count = 0;
            }
            
            documents.push_back(std::move(doc));
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Line " + std::to_string(line_number) + 
                                   ": " + std::string(e.what()));
        }
    }
    
    return documents;
}

Document DocumentLoader::createDocument(const std::unordered_map<std::string, std::string>& fields) {
    Document doc;
    doc.id = next_doc_id_++;
    
    // Validate we haven't exceeded uint32_t limit
    if (next_doc_id_ == 0) {  // Overflow occurred
        throw std::runtime_error("Document ID overflow: exceeded 4 billion documents");
    }
    
    doc.fields = fields;
    
    // Calculate term_count (approximate: count whitespace-separated tokens)
    std::string all_text = doc.getAllText();
    if (!all_text.empty()) {
        doc.term_count = 1;  // Start with 1 for first token
        for (char c : all_text) {
            if (std::isspace(c)) {
                doc.term_count++;
            }
        }
    } else {
        doc.term_count = 0;
    }
    
    return doc;
}

} // namespace search_engine
