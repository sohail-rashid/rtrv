#include <benchmark/benchmark.h>
#include "search_engine.hpp"
#include <fstream>
#include <vector>
#include <sstream>

using namespace search_engine;

// Helper function to load Wikipedia sample data
std::vector<std::pair<std::string, std::string>> loadWikipediaSample() {
    std::vector<std::pair<std::string, std::string>> docs;
    
    // Try multiple paths to find the data file
    std::vector<std::string> paths = {
        "data/wikipedia_sample.txt",
        "../data/wikipedia_sample.txt",
        "../../data/wikipedia_sample.txt"
    };
    
    std::ifstream file;
    bool found = false;
    for (const auto& path : paths) {
        file.open(path);
        if (file.is_open()) {
            found = true;
            break;
        }
        file.clear(); // Clear error flags before trying next path
    }
    
    if (!found) {
        return docs; // Return empty if file not found
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            auto pos = line.find('|');
            if (pos != std::string::npos) {
                std::string title = line.substr(0, pos);
                std::string content = line.substr(pos + 1);
                docs.emplace_back(title, content);
            }
        }
    }
    
    return docs;
}

static void BM_IndexDocument(benchmark::State& state) {
    auto docs = loadWikipediaSample();
    if (docs.empty()) {
        state.SkipWithError("No Wikipedia sample data found");
        return;
    }
    
    size_t doc_index = 0;
    
    for (auto _ : state) {
        SearchEngine engine;
        Document doc;
        doc.fields["title"] = docs[doc_index % docs.size()].first;
        doc.fields["content"] = docs[doc_index % docs.size()].second;
        engine.indexDocument(doc);
        doc_index++;
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_IndexDocument)->Range(1, 1<<10);

static void BM_BatchIndexing(benchmark::State& state) {
    auto docs = loadWikipediaSample();
    if (docs.empty()) {
        state.SkipWithError("No Wikipedia sample data found");
        return;
    }
    
    int batch_size = state.range(0);
    
    for (auto _ : state) {
        SearchEngine engine;
        
        for (int i = 0; i < batch_size && i < static_cast<int>(docs.size()); ++i) {
            Document doc;
            doc.fields["title"] = docs[i % docs.size()].first;
            doc.fields["content"] = docs[i % docs.size()].second;
            engine.indexDocument(doc);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}

BENCHMARK(BM_BatchIndexing)->Arg(100)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();
