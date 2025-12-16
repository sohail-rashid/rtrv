 #include "search_engine.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

using namespace search_engine;

// Simple JSON helper functions
std::string escapeJson(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c; break;
        }
    }
    return oss.str();
}

void printHelp() {
    std::cout << "\nAvailable commands:\n";
    std::cout << "  index <id> <content>  - Index a document\n";
    std::cout << "  search <query>        - Search for documents\n";
    std::cout << "  delete <id>           - Delete a document\n";
    std::cout << "  stats                 - Show index statistics\n";
    std::cout << "  save <file>           - Save snapshot to file\n";
    std::cout << "  load <file>           - Load snapshot from file\n";
    std::cout << "  help                  - Show this help\n";
    std::cout << "  quit                  - Exit the server\n";
}

void handleIndex(SearchEngine& engine, const std::string& args) {
    std::istringstream iss(args);
    uint64_t id;
    std::string content;
    
    if (!(iss >> id)) {
        std::cout << "{\"error\": \"Invalid document ID\"}\n";
        return;
    }
    
    std::getline(iss >> std::ws, content);
    if (content.empty()) {
        std::cout << "{\"error\": \"Empty content\"}\n";
        return;
    }
    
    Document doc{id, content};
    uint64_t result_id = engine.indexDocument(doc);
    
    std::cout << "{\"success\": true, \"doc_id\": " << result_id << "}\n";
}

void handleSearch(SearchEngine& engine, const std::string& query) {
    if (query.empty()) {
        std::cout << "{\"error\": \"Empty query\"}\n";
        return;
    }
    
    auto results = engine.search(query);
    
    std::cout << "{\n  \"results\": [\n";
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        std::cout << "    {\n";
        std::cout << "      \"doc_id\": " << result.document.id << ",\n";
        std::cout << "      \"score\": " << std::fixed << std::setprecision(6) 
                  << result.score << ",\n";
        std::cout << "      \"content\": \"" << escapeJson(result.document.content) << "\"\n";
        std::cout << "    }";
        if (i < results.size() - 1) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  ],\n";
    std::cout << "  \"total\": " << results.size() << "\n";
    std::cout << "}\n";
}

void handleDelete(SearchEngine& engine, const std::string& args) {
    std::istringstream iss(args);
    uint64_t id;
    
    if (!(iss >> id)) {
        std::cout << "{\"error\": \"Invalid document ID\"}\n";
        return;
    }
    
    bool success = engine.deleteDocument(id);
    std::cout << "{\"success\": " << (success ? "true" : "false") << "}\n";
}

void handleStats(SearchEngine& engine) {
    auto stats = engine.getStats();
    
    std::cout << "{\n";
    std::cout << "  \"total_documents\": " << stats.total_documents << ",\n";
    std::cout << "  \"total_terms\": " << stats.total_terms << ",\n";
    std::cout << "  \"avg_doc_length\": " << std::fixed << std::setprecision(2) 
              << stats.avg_doc_length << "\n";
    std::cout << "}\n";
}

void handleSave(SearchEngine& engine, const std::string& filepath) {
    if (filepath.empty()) {
        std::cout << "{\"error\": \"No filepath specified\"}\n";
        return;
    }
    
    bool success = engine.saveSnapshot(filepath);
    std::cout << "{\"success\": " << (success ? "true" : "false") << "}\n";
}

void handleLoad(SearchEngine& engine, const std::string& filepath) {
    if (filepath.empty()) {
        std::cout << "{\"error\": \"No filepath specified\"}\n";
        return;
    }
    
    bool success = engine.loadSnapshot(filepath);
    std::cout << "{\"success\": " << (success ? "true" : "false") << "}\n";
}

using namespace search_engine;

int main() {
    SearchEngine engine;
    
    std::cout << "Search Engine Interactive API Server\n";
    std::cout << "====================================\n";
    std::cout << "Type 'help' for available commands\n";
    
    printHelp();
    
    std::string line;
    while (true) {
        std::cout << "\n> ";
        if (!std::getline(std::cin, line)) {
            break;  // EOF
        }
        
        // Trim leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            continue;  // Empty line
        }
        line = line.substr(start);
        
        // Parse command
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        std::string args;
        std::getline(iss >> std::ws, args);
        
        if (command == "quit" || command == "exit") {
            std::cout << "Goodbye!\n";
            break;
        } else if (command == "help") {
            printHelp();
        } else if (command == "index") {
            handleIndex(engine, args);
        } else if (command == "search") {
            handleSearch(engine, args);
        } else if (command == "delete") {
            handleDelete(engine, args);
        } else if (command == "stats") {
            handleStats(engine);
        } else if (command == "save") {
            handleSave(engine, args);
        } else if (command == "load") {
            handleLoad(engine, args);
        } else {
            std::cout << "{\"error\": \"Unknown command: " << command << "\"}\n";
            std::cout << "Type 'help' for available commands\n";
        }
    }
    
    return 0;
}
