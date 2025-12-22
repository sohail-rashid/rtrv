 #include "search_engine.hpp"
#include "document_loader.hpp"
#include <crow.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <memory>
#include <chrono>
#include <ctime>
#include <vector>

using namespace search_engine;

// Global search engine instance
std::shared_ptr<SearchEngine> g_engine;

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

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    
    // Create search engine instance
    g_engine = std::make_shared<SearchEngine>();
    
    // Load sample data from file
    std::cout << "Loading sample data from wikipedia_sample.json...\n";
    DocumentLoader loader;
    
    // Try multiple paths to find the data file
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
                g_engine->indexDocument(doc);
            }
            std::cout << "✅ Loaded " << documents.size() << " documents from " << path << "\n";
            loaded = true;
            break;
        } catch (const std::exception& e) {
            continue;
        }
    }
    
    if (!loaded) {
        std::cerr << "⚠️  Warning: Could not load wikipedia_sample.json from any location, starting with empty index\n";
    }
    
    // Create Crow app
    crow::SimpleApp app;
    
    // Middleware for CORS
    CROW_ROUTE(app, "/")
        .methods("OPTIONS"_method)
        ([](const crow::request&) {
            crow::response res(200);
            res.add_header("Access-Control-Allow-Origin", "*");
            res.add_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
            res.add_header("Access-Control-Allow-Headers", "Content-Type");
            return res;
        });
    
    // Search endpoint - GET /search?q=<query>&algorithm=<bm25|tfidf>&max_results=<n>
    CROW_ROUTE(app, "/search")
    ([](const crow::request& req) {
        std::string client_ip = req.remote_ip_address;
        std::cout << "📥 [" << getTimestamp() << "] [" << client_ip << "] GET /search" << std::endl;
        
        auto q = req.url_params.get("q");
        if (!q) {
            std::cout << "⚠️ [" << getTimestamp() << "] [" << client_ip << "] GET /search → 400" << std::endl;
            crow::response res(400, R"({"error": "Missing query parameter"})");
            res.add_header("Content-Type", "application/json");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        
        SearchOptions options;
        
        auto algorithm = req.url_params.get("algorithm");
        if (algorithm && std::string(algorithm) == "tfidf") {
            options.algorithm = SearchOptions::TF_IDF;
        }
        
        auto max_results = req.url_params.get("max_results");
        if (max_results) {
            options.max_results = std::stoi(std::string(max_results));
        }
        
        auto results = g_engine->search(q, options);
        
        // Build JSON response
        crow::json::wvalue response;
        response["total_results"] = results.size();
        
        std::vector<crow::json::wvalue> result_array;
        for (const auto& result : results) {
            crow::json::wvalue item;
            item["score"] = result.score;
            item["document"]["id"] = result.document.id;
            item["document"]["content"] = result.document.getAllText();
            result_array.push_back(std::move(item));
        }
        response["results"] = std::move(result_array);
        
        std::cout << "✅ [" << getTimestamp() << "] [" << client_ip << "] GET /search → 200" << std::endl;
        crow::response res(200, response);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });
    
    // Stats endpoint - GET /stats
    CROW_ROUTE(app, "/stats")
    ([](const crow::request& req) {
        std::string client_ip = req.remote_ip_address;
        std::cout << "📥 [" << getTimestamp() << "] [" << client_ip << "] GET /stats" << std::endl;
        
        auto stats = g_engine->getStats();
        
        crow::json::wvalue response;
        response["total_documents"] = stats.total_documents;
        response["total_terms"] = stats.total_terms;
        response["avg_doc_length"] = stats.avg_doc_length;
        
        std::cout << "✅ [" << getTimestamp() << "] [" << client_ip << "] GET /stats → 200" << std::endl;
        crow::response res(200, response);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });
    
    // Index endpoint - POST /index with body: {"id": number, "content": "text"}
    CROW_ROUTE(app, "/index").methods("POST"_method)
    ([](const crow::request& req) {
        std::string client_ip = req.remote_ip_address;
        std::cout << "📥 [" << getTimestamp() << "] [" << client_ip << "] POST /index" << std::endl;
        
        try {
            auto body = crow::json::load(req.body);
            if (!body) {
                std::cout << "⚠️ [" << getTimestamp() << "] [" << client_ip << "] POST /index → 400" << std::endl;
                crow::response res(400, R"({"error": "Invalid JSON body"})");
                res.add_header("Content-Type", "application/json");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }
            
            if (!body.has("id") || !body.has("content")) {
                std::cout << "⚠️ [" << getTimestamp() << "] [" << client_ip << "] POST /index → 400" << std::endl;
                crow::response res(400, R"({"error": "Missing id or content field"})");
                res.add_header("Content-Type", "application/json");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }
            
            uint64_t id = body["id"].u();
            std::string content = body["content"].s();
            
            Document doc{static_cast<uint32_t>(id), std::unordered_map<std::string, std::string>{{"content", content}}};
            g_engine->indexDocument(doc);
            
            crow::json::wvalue response;
            response["success"] = true;
            response["doc_id"] = id;
            
            std::cout << "✅ [" << getTimestamp() << "] [" << client_ip << "] POST /index → 200" << std::endl;
            crow::response res(200, response);
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
            
        } catch (const std::exception& e) {
            std::cout << "❌ [" << getTimestamp() << "] [" << client_ip << "] POST /index → 500" << std::endl;
            crow::json::wvalue error;
            error["error"] = std::string("Server error: ") + e.what();
            crow::response res(500, error);
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
    });
    
    // Delete endpoint - DELETE /delete/<id>
    CROW_ROUTE(app, "/delete/<uint>").methods("DELETE"_method)
    ([](const crow::request& req, uint64_t id) {
        std::string client_ip = req.remote_ip_address;
        std::cout << "📥 [" << getTimestamp() << "] [" << client_ip << "] DELETE /delete/" << id << std::endl;
        
        bool success = g_engine->deleteDocument(id);
        
        crow::json::wvalue response;
        response["success"] = success;
        response["doc_id"] = id;
        
        std::cout << "✅ [" << getTimestamp() << "] [" << client_ip << "] DELETE /delete/" << id << " → 200" << std::endl;
        crow::response res(200, response);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });
    
    // Save endpoint - POST /save with body: {"filename": "path"}
    CROW_ROUTE(app, "/save").methods("POST"_method)
    ([](const crow::request& req) {
        std::string client_ip = req.remote_ip_address;
        std::cout << "📥 [" << getTimestamp() << "] [" << client_ip << "] POST /save" << std::endl;
        
        try {
            auto body = crow::json::load(req.body);
            if (!body) {
                std::cout << "⚠️ [" << getTimestamp() << "] [" << client_ip << "] POST /save → 400" << std::endl;
                crow::response res(400, R"({"error": "Invalid JSON body"})");
                res.add_header("Content-Type", "application/json");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }
            
            if (!body.has("filename")) {
                std::cout << "⚠️ [" << getTimestamp() << "] [" << client_ip << "] POST /save → 400" << std::endl;
                crow::response res(400, R"({"error": "Missing filename field"})");
                res.add_header("Content-Type", "application/json");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }
            
            std::string filename = body["filename"].s();
            bool success = g_engine->saveSnapshot(filename);
            
            crow::json::wvalue response;
            response["success"] = success;
            response["filename"] = filename;
            
            std::cout << "✅ [" << getTimestamp() << "] [" << client_ip << "] POST /save → 200" << std::endl;
            crow::response res(200, response);
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
            
        } catch (const std::exception& e) {
            std::cout << "❌ [" << getTimestamp() << "] [" << client_ip << "] POST /save → 500" << std::endl;
            crow::json::wvalue error;
            error["error"] = std::string("Server error: ") + e.what();
            crow::response res(500, error);
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
    });
    
    // Load endpoint - POST /load with body: {"filename": "path"}
    CROW_ROUTE(app, "/load").methods("POST"_method)
    ([](const crow::request& req) {
        std::string client_ip = req.remote_ip_address;
        std::cout << "📥 [" << getTimestamp() << "] [" << client_ip << "] POST /load" << std::endl;
        
        try {
            auto body = crow::json::load(req.body);
            if (!body) {
                std::cout << "⚠️ [" << getTimestamp() << "] [" << client_ip << "] POST /load → 400" << std::endl;
                crow::response res(400, R"({"error": "Invalid JSON body"})");
                res.add_header("Content-Type", "application/json");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }
            
            if (!body.has("filename")) {
                std::cout << "⚠️ [" << getTimestamp() << "] [" << client_ip << "] POST /load → 400" << std::endl;
                crow::response res(400, R"({"error": "Missing filename field"})");
                res.add_header("Content-Type", "application/json");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }
            
            std::string filename = body["filename"].s();
            bool success = g_engine->loadSnapshot(filename);
            
            crow::json::wvalue response;
            response["success"] = success;
            response["filename"] = filename;
            
            std::cout << "✅ [" << getTimestamp() << "] [" << client_ip << "] POST /load → 200" << std::endl;
            crow::response res(200, response);
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
            
        } catch (const std::exception& e) {
            std::cout << "❌ [" << getTimestamp() << "] [" << client_ip << "] POST /load → 500" << std::endl;
            crow::json::wvalue error;
            error["error"] = std::string("Server error: ") + e.what();
            crow::response res(500, error);
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
    });
    
    // Skip rebuild endpoint - POST /skip/rebuild
    CROW_ROUTE(app, "/skip/rebuild").methods("POST"_method)
    ([](const crow::request& req) {
        std::string client_ip = req.remote_ip_address;
        std::cout << "📥 [" << getTimestamp() << "] [" << client_ip << "] POST /skip/rebuild" << std::endl;
        
        g_engine->getIndex()->rebuildSkipPointers();
        
        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "All skip pointers rebuilt";
        
        std::cout << "✅ [" << getTimestamp() << "] [" << client_ip << "] POST /skip/rebuild → 200" << std::endl;
        crow::response res(200, response);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });
    
    // Skip rebuild for term endpoint - POST /skip/rebuild/<term>
    CROW_ROUTE(app, "/skip/rebuild/<string>").methods("POST"_method)
    ([](const crow::request& req, const std::string& term) {
        std::string client_ip = req.remote_ip_address;
        std::cout << "📥 [" << getTimestamp() << "] [" << client_ip << "] POST /skip/rebuild/" << term << std::endl;
        
        g_engine->getIndex()->rebuildSkipPointers(term);
        
        crow::json::wvalue response;
        response["success"] = true;
        response["term"] = term;
        
        std::cout << "✅ [" << getTimestamp() << "] [" << client_ip << "] POST /skip/rebuild/" << term << " → 200" << std::endl;
        crow::response res(200, response);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });
    
    // Skip stats endpoint - GET /skip/stats?term=<term>
    CROW_ROUTE(app, "/skip/stats")
    ([](const crow::request& req) {
        std::string client_ip = req.remote_ip_address;
        std::cout << "📥 [" << getTimestamp() << "] [" << client_ip << "] GET /skip/stats" << std::endl;
        
        auto term = req.url_params.get("term");
        if (!term) {
            std::cout << "⚠️ [" << getTimestamp() << "] [" << client_ip << "] GET /skip/stats → 400" << std::endl;
            crow::response res(400, R"({"error": "Missing term parameter"})");
            res.add_header("Content-Type", "application/json");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        
        auto posting_list = g_engine->getIndex()->getPostingList(term);
        
        crow::json::wvalue response;
        response["term"] = term;
        response["postings_count"] = posting_list.postings.size();
        response["skip_pointers_count"] = posting_list.skip_pointers.size();
        
        if (!posting_list.skip_pointers.empty() && posting_list.skip_pointers.size() > 1) {
            size_t interval = posting_list.skip_pointers[1].position - posting_list.skip_pointers[0].position;
            response["skip_interval"] = interval;
        } else {
            response["skip_interval"] = 0;
        }
        
        response["needs_rebuild"] = posting_list.needsSkipRebuild();
        
        std::cout << "✅ [" << getTimestamp() << "] [" << client_ip << "] GET /skip/stats → 200" << std::endl;
        crow::response res(200, response);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });
    
    std::cout << "=== Search Engine REST Server (Crow) ===\n";
    std::cout << "Server listening on http://localhost:" << port << "\n";
    std::cout << "Endpoints:\n";
    std::cout << "  GET    /search?q=<query>&algorithm=<bm25|tfidf>&max_results=<n>\n";
    std::cout << "  GET    /stats\n";
    std::cout << "  POST   /index - body: {\"id\": number, \"content\": \"text\"}\n";
    std::cout << "  DELETE /delete/<id>\n";
    std::cout << "  POST   /save - body: {\"filename\": \"path\"}\n";
    std::cout << "  POST   /load - body: {\"filename\": \"path\"}\n";
    std::cout << "  POST   /skip/rebuild\n";
    std::cout << "  POST   /skip/rebuild/<term>\n";
    std::cout << "  GET    /skip/stats?term=<term>\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    
    // Run the server
    app.port(port).multithreaded().run();
    
    return 0;
}
