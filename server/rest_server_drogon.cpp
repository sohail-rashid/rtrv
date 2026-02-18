 #include "search_engine.hpp"
#include "document_loader.hpp"
#include <drogon/drogon.h>
#include <iostream>
#include <string>
#include <fstream>
#include <memory>
#include <chrono>
#include <vector>
#include <filesystem>

using namespace rtrv_search_engine;
using namespace drogon;

// Global search engine instance
static std::shared_ptr<SearchEngine> g_engine;

static std::string resolveUiRoot() {
    namespace fs = std::filesystem;
    const std::vector<fs::path> candidates = {
        fs::current_path() / "ui",
        fs::current_path() / "../ui",
        fs::current_path() / "../../server/ui",
        fs::current_path() / "../server/ui"
    };
    for (const auto& path : candidates) {
        if (fs::exists(path / "index.html")) {
            return fs::absolute(path).string();
        }
    }
    return "";
}

// Search endpoint handler
void handleSearch(const HttpRequestPtr& req,
                  std::function<void(const HttpResponsePtr&)>&& callback) {
    auto query = req->getParameter("q");
    auto algorithm = req->getParameter("algorithm");
    auto max_results_str = req->getParameter("max_results");
    auto use_heap_str = req->getParameter("use_top_k_heap");
    auto highlight_str = req->getParameter("highlight");
    auto snippet_length_str = req->getParameter("snippet_length");
    auto num_snippets_str = req->getParameter("num_snippets");
    auto fuzzy_str = req->getParameter("fuzzy");
    auto max_edit_dist_str = req->getParameter("max_edit_distance");
    auto cache_str = req->getParameter("cache");
    
    Json::Value response;
    
    if (query.empty()) {
        response["error"] = "Missing query parameter";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    
    SearchOptions options;
    if (algorithm == "tfidf") {
        options.algorithm = SearchOptions::TF_IDF;
    }
    if (!max_results_str.empty()) {
        options.max_results = std::stoi(max_results_str);
    }
    if (!use_heap_str.empty()) {
        options.use_top_k_heap = (use_heap_str == "true" || use_heap_str == "1");
    }

    // Snippet / highlighting options
    if (!highlight_str.empty()) {
        options.generate_snippets = (highlight_str == "true" || highlight_str == "1");
    }
    if (!snippet_length_str.empty()) {
        options.snippet_options.max_snippet_length = std::stoul(snippet_length_str);
    }
    if (!num_snippets_str.empty()) {
        options.snippet_options.num_snippets = std::stoul(num_snippets_str);
    }

    // Fuzzy search options
    if (!fuzzy_str.empty()) {
        options.fuzzy_enabled = (fuzzy_str == "true" || fuzzy_str == "1");
    }
    if (!max_edit_dist_str.empty()) {
        options.max_edit_distance = static_cast<uint32_t>(std::stoul(max_edit_dist_str));
    }

    // Cache options
    if (!cache_str.empty()) {
        options.use_cache = !(cache_str == "false" || cache_str == "0");
    }

    
    auto results = g_engine->search(query, options);
    
    Json::Value resultsArray(Json::arrayValue);
    for (const auto& result : results) {
        Json::Value item;
        item["score"] = result.score;
        item["document"]["id"] = (Json::UInt64)result.document.id;
        item["document"]["content"] = result.document.getAllText();

        // Include snippets if highlighting was requested
        if (!result.snippets.empty()) {
            Json::Value snippet_array(Json::arrayValue);
            for (const auto& snippet : result.snippets) {
                snippet_array.append(snippet);
            }
            item["snippets"] = std::move(snippet_array);
        }

        // Include fuzzy expanded terms if any
        if (!result.expanded_terms.empty()) {
            Json::Value expanded;
            for (const auto& entry : result.expanded_terms) {
                expanded[entry.first] = entry.second;
            }
            item["expanded_terms"] = std::move(expanded);
        }
        resultsArray.append(item);
    }
    
    response["results"] = resultsArray;
    response["total_results"] = (Json::UInt)results.size();
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// Stats endpoint handler
void handleStats(const HttpRequestPtr&,
                 std::function<void(const HttpResponsePtr&)>&& callback) {
    auto stats = g_engine->getStats();
    
    Json::Value response;
    response["total_documents"] = (Json::UInt)stats.total_documents;
    response["total_terms"] = (Json::UInt)stats.total_terms;
    response["avg_doc_length"] = stats.avg_doc_length;
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// List documents endpoint handler
void handleListDocuments(const HttpRequestPtr& req,
                         std::function<void(const HttpResponsePtr&)>&& callback) {
    auto offset_str = req->getParameter("offset");
    auto limit_str = req->getParameter("limit");

    size_t offset = 0, limit = 10;
    if (!offset_str.empty()) offset = std::stoul(offset_str);
    if (!limit_str.empty()) limit = std::stoul(limit_str);
    if (limit > 1000) limit = 1000;

    auto docs = g_engine->getDocuments(offset, limit);
    auto stats = g_engine->getStats();

    Json::Value resultsArray(Json::arrayValue);
    for (const auto& [id, doc] : docs) {
        Json::Value item;
        item["score"] = 0.0;
        item["document"]["id"] = (Json::UInt64)id;
        item["document"]["content"] = doc.getAllText();
        resultsArray.append(item);
    }

    Json::Value response;
    response["results"] = resultsArray;
    response["total_results"] = (Json::UInt)docs.size();
    response["total_documents"] = (Json::UInt)stats.total_documents;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// Cache stats endpoint handler
void handleCacheStats(const HttpRequestPtr&,
                      std::function<void(const HttpResponsePtr&)>&& callback) {
    auto stats = g_engine->getCacheStats();

    Json::Value response;
    response["hit_count"] = (Json::UInt64)stats.hit_count;
    response["miss_count"] = (Json::UInt64)stats.miss_count;
    response["eviction_count"] = (Json::UInt64)stats.eviction_count;
    response["current_size"] = (Json::UInt64)stats.current_size;
    response["max_size"] = (Json::UInt64)stats.max_size;
    response["hit_rate"] = stats.hit_rate;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// Cache clear endpoint handler
void handleCacheClear(const HttpRequestPtr&,
                      std::function<void(const HttpResponsePtr&)>&& callback) {
    g_engine->clearCache();

    Json::Value response;
    response["success"] = true;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// Index document endpoint handler
void handleIndex(const HttpRequestPtr& req,
                 std::function<void(const HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    Json::Value response;
    
    if (!json || !json->isMember("id") || !json->isMember("content")) {
        response["error"] = "Invalid request body. Expected {\"id\": number, \"content\": \"text\"}";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    
    uint64_t id = (*json)["id"].asUInt64();
    std::string content = (*json)["content"].asString();
    
    Document doc{static_cast<uint32_t>(id), std::unordered_map<std::string, std::string>{{"content", content}}};
    g_engine->indexDocument(doc);
    
    response["success"] = true;
    response["doc_id"] = (Json::UInt64)id;
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// Delete document endpoint handler
void handleDelete(const HttpRequestPtr&,
                  std::function<void(const HttpResponsePtr&)>&& callback,
                  const std::string& id_str) {
    Json::Value response;
    
    try {
        uint64_t id = std::stoull(id_str);
        bool success = g_engine->deleteDocument(id);
        
        response["success"] = success;
        response["doc_id"] = (Json::UInt64)id;
        
        auto resp = HttpResponse::newHttpJsonResponse(response);
        callback(resp);
    } catch (const std::exception& e) {
        response["error"] = "Invalid document ID";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }
}

// Save snapshot endpoint handler
void handleSave(const HttpRequestPtr& req,
                std::function<void(const HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    Json::Value response;
    
    if (!json || !json->isMember("filename")) {
        response["error"] = "Missing filename in request body";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    
    std::string filename = (*json)["filename"].asString();
    bool success = g_engine->saveSnapshot(filename);
    
    response["success"] = success;
    response["filename"] = filename;
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// Load snapshot endpoint handler
void handleLoad(const HttpRequestPtr& req,
                std::function<void(const HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    Json::Value response;
    
    if (!json || !json->isMember("filename")) {
        response["error"] = "Missing filename in request body";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    
    std::string filename = (*json)["filename"].asString();
    bool success = g_engine->loadSnapshot(filename);
    
    response["success"] = success;
    response["filename"] = filename;
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// Skip pointer rebuild endpoint handler
void handleSkipRebuild(const HttpRequestPtr&,
                       std::function<void(const HttpResponsePtr&)>&& callback,
                       const std::string& term = "") {
    Json::Value response;
    
    if (!term.empty()) {
        // Rebuild for specific term
        g_engine->getIndex()->rebuildSkipPointers(term);
        response["success"] = true;
        response["term"] = term;
    } else {
        // Rebuild all skip pointers
        g_engine->getIndex()->rebuildSkipPointers();
        response["success"] = true;
        response["message"] = "All skip pointers rebuilt";
    }
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

// Skip pointer stats endpoint handler
void handleSkipStats(const HttpRequestPtr& req,
                     std::function<void(const HttpResponsePtr&)>&& callback) {
    auto term = req->getParameter("term");
    Json::Value response;
    
    if (term.empty()) {
        response["error"] = "Missing term parameter";
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    
    auto posting_list = g_engine->getIndex()->getPostingList(term);
    
    response["term"] = term;
    response["postings_count"] = (Json::UInt)posting_list.postings.size();
    response["skip_pointers_count"] = (Json::UInt)posting_list.skip_pointers.size();
    
    if (!posting_list.skip_pointers.empty() && posting_list.skip_pointers.size() > 1) {
        size_t interval = posting_list.skip_pointers[1].position - posting_list.skip_pointers[0].position;
        response["skip_interval"] = (Json::UInt)interval;
    } else {
        response["skip_interval"] = 0;
    }
    
    response["needs_rebuild"] = posting_list.needsSkipRebuild();
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    
    // Initialize search engine
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
    
    std::cout << "=== Rtrv REST Server (Drogon) ===\n";
    std::cout << "Server will listen on http://localhost:" << port << "\n";
    std::cout << "Endpoints:\n";
    std::cout << "  GET    /search?q=<query>&algorithm=<bm25|tfidf>&max_results=<n>&use_top_k_heap=<true|false>&cache=<true|false>\n";
    std::cout << "  GET    /stats\n";
    std::cout << "  GET    /cache/stats\n";
    std::cout << "  DELETE /cache\n";
    std::cout << "  POST   /index - body: {\"id\": number, \"content\": \"text\"}\n";
    std::cout << "  DELETE /delete/<id>\n";
    std::cout << "  POST   /save - body: {\"filename\": \"path\"}\n";
    std::cout << "  POST   /load - body: {\"filename\": \"path\"}\n";
    std::cout << "  POST   /skip/rebuild\n";
    std::cout << "  POST   /skip/rebuild/<term>\n";
    std::cout << "  GET    /skip/stats?term=<term>\n";
    std::cout << "  GET    / (web UI)\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    
    // Configure Drogon
    app().addListener("0.0.0.0", port);
    app().setThreadNum(4);  // Use 4 threads

    const std::string ui_root = resolveUiRoot();
    if (!ui_root.empty()) {
        app().setDocumentRoot(ui_root);
    }
    
    // Log incoming requests
    app().registerPreHandlingAdvice(
        [](const HttpRequestPtr& req) {
            auto method = req->getMethodString();
            auto path = req->path();
            auto query = req->query();
            auto client_ip = req->getPeerAddr().toIp();
            
            LOG_INFO << "📥 [" << client_ip << "] " << method << " " << path 
                     << (query.empty() ? "" : "?" + query);
        });
    
    // Enable CORS and log response status
    app().registerPostHandlingAdvice(
        [](const HttpRequestPtr& req, const HttpResponsePtr& resp) {
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
            
            auto method = req->getMethodString();
            auto path = req->path();
            auto status = resp->statusCode();
            auto client_ip = req->getPeerAddr().toIp();
            
            std::string emoji = status >= 200 && status < 300 ? "✅" : 
                               status >= 400 && status < 500 ? "⚠️" : "❌";
            
            LOG_INFO << emoji << " [" << client_ip << "] " << method << " " << path 
                     << " → " << status;
        });
    
    // Register routes
    app().registerHandler("/search?q={query}", &handleSearch, {Get});
    app().registerHandler("/stats", &handleStats, {Get});
    app().registerHandler("/documents", &handleListDocuments, {Get});
    app().registerHandler("/cache/stats", &handleCacheStats, {Get});
    app().registerHandler("/", [ui_root](const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& callback) {
        if (ui_root.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        auto resp = HttpResponse::newFileResponse(ui_root + "/index.html");
        callback(resp);
    }, {Get});
    app().registerHandler("/index", &handleIndex, {Post});
    app().registerHandler("/delete/{id}", &handleDelete, {Delete});
    app().registerHandler("/cache", &handleCacheClear, {Delete});
    app().registerHandler("/save", &handleSave, {Post});
    app().registerHandler("/load", &handleLoad, {Post});
    app().registerHandler("/skip/rebuild", 
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            handleSkipRebuild(req, std::move(callback), "");
        }, {Post});
    app().registerHandler("/skip/rebuild/{term}", &handleSkipRebuild, {Post});
    app().registerHandler("/skip/stats", &handleSkipStats, {Get});
    
    // Handle OPTIONS for CORS preflight
    app().registerHandler("/*",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback) {
            if (req->method() == Options) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k200OK);
                callback(resp);
            }
        }, {Options});
    
    // Run the server
    std::cout << "Starting server...\n";
    app().run();
    
    return 0;
}
