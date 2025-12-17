 #include "search_engine.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <regex>
#include <chrono>
#include <ctime>

using namespace search_engine;

// Get current timestamp as string
std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

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

std::string urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value = std::stoi(str.substr(i + 1, 2), nullptr, 16);
            result += static_cast<char>(value);
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string getQueryParam(const std::string& query, const std::string& param) {
    std::regex pattern(param + "=([^&]*)");
    std::smatch match;
    if (std::regex_search(query, match, pattern)) {
        return urlDecode(match[1]);
    }
    return "";
}

std::string handleSearch(SearchEngine& engine, const std::string& query_string) {
    std::string q = getQueryParam(query_string, "q");
    std::string algorithm = getQueryParam(query_string, "algorithm");
    std::string max_results_str = getQueryParam(query_string, "max_results");
    
    if (q.empty()) {
        return "{\"error\": \"Missing query parameter\"}";
    }
    
    SearchOptions options;
    if (algorithm == "tfidf") {
        options.algorithm = SearchOptions::TF_IDF;
    }
    if (!max_results_str.empty()) {
        options.max_results = std::stoi(max_results_str);
    }
    
    auto results = engine.search(q, options);
    
    std::ostringstream json;
    json << "{\n  \"results\": [\n";
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        json << "    {\n";
        json << "      \"score\": " << std::fixed << std::setprecision(6) << result.score << ",\n";
        json << "      \"document\": {\n";
        json << "        \"id\": " << result.document.id << ",\n";
        json << "        \"content\": \"" << escapeJson(result.document.content) << "\"\n";
        json << "      }\n";
        json << "    }";
        if (i < results.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n";
    json << "  \"total_results\": " << results.size() << "\n";
    json << "}";
    
    return json.str();
}

std::string handleStats(SearchEngine& engine) {
    auto stats = engine.getStats();
    
    std::ostringstream json;
    json << "{\n";
    json << "  \"total_documents\": " << stats.total_documents << ",\n";
    json << "  \"total_terms\": " << stats.total_terms << ",\n";
    json << "  \"avg_doc_length\": " << std::fixed << std::setprecision(2) << stats.avg_doc_length << "\n";
    json << "}";
    
    return json.str();
}

std::string handleIndex(SearchEngine& engine, const std::string& body) {
    // Parse JSON body (simple parser for {"id": 123, "content": "..."} format)
    std::regex id_pattern("\"id\"\\s*:\\s*(\\d+)");
    std::regex content_pattern("\"content\"\\s*:\\s*\"([^\"]*)\"");
    
    std::smatch id_match, content_match;
    
    if (!std::regex_search(body, id_match, id_pattern) || 
        !std::regex_search(body, content_match, content_pattern)) {
        return "{\"error\": \"Invalid request body. Expected {\\\"id\\\": number, \\\"content\\\": \\\"text\\\"}\"";
    }
    
    uint64_t id = std::stoull(id_match[1]);
    std::string content = content_match[1];
    
    Document doc(id, content);
    engine.indexDocument(doc);
    
    std::ostringstream json;
    json << "{\"success\": true, \"doc_id\": " << id << "}";
    return json.str();
}

std::string handleDelete(SearchEngine& engine, const std::string& path) {
    // Extract ID from path like /delete/123
    std::regex id_pattern("/delete/(\\d+)");
    std::smatch match;
    
    if (!std::regex_search(path, match, id_pattern)) {
        return "{\"error\": \"Invalid document ID\"}";
    }
    
    uint64_t id = std::stoull(match[1]);
    bool success = engine.deleteDocument(id);
    
    std::ostringstream json;
    json << "{\"success\": " << (success ? "true" : "false") << ", \"doc_id\": " << id << "}";
    return json.str();
}

std::string handleSave(SearchEngine& engine, const std::string& body) {
    // Parse JSON body for filename
    std::regex filename_pattern("\"filename\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    
    if (!std::regex_search(body, match, filename_pattern)) {
        return "{\"error\": \"Missing filename in request body\"}";
    }
    
    std::string filename = match[1];
    bool success = engine.saveSnapshot(filename);
    
    std::ostringstream json;
    json << "{\"success\": " << (success ? "true" : "false") << ", \"filename\": \"" << escapeJson(filename) << "\"}";
    return json.str();
}

std::string handleLoad(SearchEngine& engine, const std::string& body) {
    // Parse JSON body for filename
    std::regex filename_pattern("\"filename\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    
    if (!std::regex_search(body, match, filename_pattern)) {
        return "{\"error\": \"Missing filename in request body\"}";
    }
    
    std::string filename = match[1];
    bool success = engine.loadSnapshot(filename);
    
    std::ostringstream json;
    json << "{\"success\": " << (success ? "true" : "false") << ", \"filename\": \"" << escapeJson(filename) << "\"}";
    return json.str();
}

void handleClient(int client_socket, SearchEngine& engine) {
    char buffer[4096] = {0};
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    
    // Get client address
    sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_socket, (struct sockaddr*)&client_addr, &addr_len);
    std::string client_ip = inet_ntoa(client_addr.sin_addr);
    
    std::string request(buffer);
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version;
    
    // Log incoming request
    std::cout << "📥 [" << getTimestamp() << "] [" << client_ip << "] " 
              << method << " " << path << std::endl;
    
    // Extract request body for POST requests
    std::string body;
    size_t body_pos = request.find("\r\n\r\n");
    if (body_pos != std::string::npos) {
        body = request.substr(body_pos + 4);
    }
    
    std::string response_body;
    std::string response_headers;
    
    // CORS headers
    response_headers = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application/json\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
                      "Access-Control-Allow-Headers: Content-Type\r\n";
    
    if (method == "OPTIONS") {
        response_body = "";
    } else if (method == "GET" && path.find("/search") == 0) {
        size_t query_pos = path.find('?');
        std::string query_string = (query_pos != std::string::npos) ? path.substr(query_pos + 1) : "";
        response_body = handleSearch(engine, query_string);
    } else if (method == "GET" && path == "/stats") {
        response_body = handleStats(engine);
    } else if (method == "POST" && path == "/index") {
        response_body = handleIndex(engine, body);
    } else if (method == "DELETE" && path.find("/delete/") == 0) {
        response_body = handleDelete(engine, path);
    } else if (method == "POST" && path == "/save") {
        response_body = handleSave(engine, body);
    } else if (method == "POST" && path == "/load") {
        response_body = handleLoad(engine, body);
    } else {
        response_body = "{\"error\": \"Not found\"}";
        response_headers = "HTTP/1.1 404 Not Found\r\n"
                          "Content-Type: application/json\r\n"
                          "Access-Control-Allow-Origin: *\r\n";
    }
    
    response_headers += "Content-Length: " + std::to_string(response_body.length()) + "\r\n";
    response_headers += "\r\n";
    
    // Determine status code from response headers
    std::string status_code = "200";
    std::string emoji = "✅";
    if (response_headers.find("404") != std::string::npos) {
        status_code = "404";
        emoji = "⚠️";
    } else if (response_headers.find("400") != std::string::npos) {
        status_code = "400";
        emoji = "⚠️";
    } else if (response_headers.find("500") != std::string::npos) {
        status_code = "500";
        emoji = "❌";
    }
    
    std::string full_response = response_headers + response_body;
    write(client_socket, full_response.c_str(), full_response.length());
    
    // Log response status
    std::cout << emoji << " [" << getTimestamp() << "] [" << client_ip << "] " 
              << method << " " << path << " → " << status_code << std::endl;
    
    close(client_socket);
}

using namespace search_engine;

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    
    SearchEngine engine;
    
    // Load sample data from file
    std::cout << "Loading sample data from data/wikipedia_sample.txt...\n";
    std::ifstream file("../data/wikipedia_sample.txt");
    if (file.is_open()) {
        std::string line;
        int count = 0;
        while (std::getline(file, line)) {
            size_t delimiter_pos = line.find('|');
            if (delimiter_pos != std::string::npos) {
                uint64_t id = std::stoull(line.substr(0, delimiter_pos));
                std::string content = line.substr(delimiter_pos + 1);
                engine.indexDocument(Document(id, content));
                count++;
            }
        }
        file.close();
        std::cout << "Loaded " << count << " documents\n";
    } else {
        std::cerr << "Warning: Could not open data/wikipedia_sample.txt, starting with empty index\n";
    }
    
    // Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options\n";
        close(server_fd);
        return 1;
    }
    
    // Bind to port
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port << "\n";
        close(server_fd);
        return 1;
    }
    
    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Failed to listen on socket\n";
        close(server_fd);
        return 1;
    }
    
    std::cout << "=== Search Engine REST Server ===\n";
    std::cout << "Server listening on http://localhost:" << port << "\n";
    std::cout << "Endpoints:\n";
    std::cout << "  GET    /search?q=<query>&algorithm=<bm25|tfidf>&max_results=<n>\n";
    std::cout << "  GET    /stats\n";
    std::cout << "  POST   /index - body: {\"id\": number, \"content\": \"text\"}\n";
    std::cout << "  DELETE /delete/<id>\n";
    std::cout << "  POST   /save - body: {\"filename\": \"path\"}\n";
    std::cout << "  POST   /load - body: {\"filename\": \"path\"}\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    
    // Accept connections
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }
        
        // Handle request in a separate thread
        std::thread([client_socket, &engine]() {
            handleClient(client_socket, engine);
        }).detach();
    }
    
    close(server_fd);
    return 0;
}
