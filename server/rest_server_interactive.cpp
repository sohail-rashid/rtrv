 #include "search_engine.hpp"
#include "document_loader.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <functional>
#include <map>
#include <algorithm>
#include <vector>

using namespace search_engine;

// Command handler type
using CommandHandler = std::function<void(SearchEngine&, const std::string&)>;

// Simple JSON helper function for output
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
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    Available Commands                          ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ index <id> <content>  │ Index a document                       ║\n";
    std::cout << "║ search <query>        │ Search for documents                   ║\n";
    std::cout << "║ delete <id>           │ Delete a document                      ║\n";
    std::cout << "║ stats                 │ Show index statistics                  ║\n";
    std::cout << "║ save <file>           │ Save snapshot to file                  ║\n";
    std::cout << "║ load <file>           │ Load snapshot from file                ║\n";
    std::cout << "║ clear                 │ Clear the screen                       ║\n";
    std::cout << "║ help (or ?)           │ Show this help                         ║\n";
    std::cout << "║ quit (or q, exit)     │ Exit the server                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
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
    
    Document doc{static_cast<uint32_t>(id), std::unordered_map<std::string, std::string>{{"content", content}}};
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
        std::cout << "      \"score\": " << std::fixed << std::setprecision(6) 
                  << result.score << ",\n";
        std::cout << "      \"document\": {\n";
        std::cout << "        \"id\": " << result.document.id << ",\n";
        std::cout << "        \"content\": \"" << escapeJson(result.document.getAllText()) << "\"\n";
        std::cout << "      }\n";
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

void handleStats(SearchEngine& engine, const std::string&) {
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

void handleClear(SearchEngine&, const std::string&) {
    // Clear screen: ANSI escape code
    std::cout << "\033[2J\033[1;1H";
}

void handleHelpCmd(SearchEngine&, const std::string&) {
    printHelp();
}

// Command registry
struct Command {
    std::string name;
    std::string description;
    CommandHandler handler;
    std::vector<std::string> aliases;
};

class CommandRegistry {
private:
    std::map<std::string, CommandHandler> handlers_;
    std::vector<Command> commands_;
    
public:
    void registerCommand(const std::string& name, const std::string& description,
                        CommandHandler handler, std::vector<std::string> aliases = {}) {
        commands_.push_back({name, description, handler, aliases});
        handlers_[name] = handler;
        for (const auto& alias : aliases) {
            handlers_[alias] = handler;
        }
    }
    
    bool execute(const std::string& cmd, SearchEngine& engine, const std::string& args) {
        auto it = handlers_.find(cmd);
        if (it != handlers_.end()) {
            it->second(engine, args);
            return true;
        }
        return false;
    }
    
    std::vector<std::string> getSuggestions(const std::string& partial) {
        std::vector<std::string> suggestions;
        std::string lower_partial = partial;
        std::transform(lower_partial.begin(), lower_partial.end(), lower_partial.begin(), ::tolower);
        
        for (const auto& cmd : commands_) {
            std::string lower_name = cmd.name;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
            if (lower_name.find(lower_partial) == 0) {
                suggestions.push_back(cmd.name);
            }
        }
        return suggestions;
    }
};

int main() {
    SearchEngine engine;
    CommandRegistry registry;
    
    // Load sample data from file
    DocumentLoader loader;
    std::vector<std::string> paths = {
        "../data/wikipedia_sample.json",      // Run from build/
        "../../data/wikipedia_sample.json",   // Run from build/server/
        "data/wikipedia_sample.json"          // Run from root
    };
    
    bool loaded = false;
    for (const auto& path : paths) {
        try {
            auto documents = loader.loadJSONL(path);
            for (const auto& doc : documents) {
                engine.indexDocument(doc);
            }
            std::cout << "✅ Loaded " << documents.size() << " documents from " << path << "\n";
            loaded = true;
            break;
        } catch (const std::exception& e) {
            continue;
        }
    }
    
    if (!loaded) {
        std::cout << "⚠️  No sample data loaded. Starting with empty index.\n";
    }
    
    // Register all commands with their handlers and aliases
    registry.registerCommand("index", "Index a document", handleIndex);
    registry.registerCommand("search", "Search for documents", handleSearch, {"find"});
    registry.registerCommand("delete", "Delete a document", handleDelete, {"del", "rm"});
    registry.registerCommand("stats", "Show index statistics", handleStats);
    registry.registerCommand("save", "Save snapshot to file", handleSave);
    registry.registerCommand("load", "Load snapshot from file", handleLoad);
    registry.registerCommand("clear", "Clear the screen", handleClear, {"cls"});
    registry.registerCommand("help", "Show this help", handleHelpCmd, {"?"});
    
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Search Engine Interactive CLI v1.0                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\nType 'help' or '?' for available commands\n";
    
    std::string line;
    bool running = true;
    
    while (running) {
        std::cout << "\n⚡ > ";
        if (!std::getline(std::cin, line)) {
            break;  // EOF
        }
        
        // Trim leading and trailing whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            continue;  // Empty line
        }
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t");
        line = line.substr(0, end + 1);
        
        // Parse command
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        // Convert command to lowercase for case-insensitive matching
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);
        
        // Get arguments
        std::string args;
        std::getline(iss >> std::ws, args);
        
        // Handle quit commands
        if (command == "quit" || command == "exit" || command == "q") {
            std::cout << "\n✨ Goodbye! Thank you for using Search Engine CLI.\n";
            running = false;
            continue;
        }
        
        // Try to execute command
        if (!registry.execute(command, engine, args)) {
            std::cout << "❌ Unknown command: '" << command << "'\n";
            
            // Provide suggestions
            auto suggestions = registry.getSuggestions(command);
            if (!suggestions.empty()) {
                std::cout << "💡 Did you mean: ";
                for (size_t i = 0; i < suggestions.size() && i < 3; ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << "'" << suggestions[i] << "'";
                }
                std::cout << "?\n";
            }
            std::cout << "Type 'help' for available commands.\n";
        }
    }
    
    return 0;
}
