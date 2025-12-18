#include <benchmark/benchmark.h>
#include "search_engine.hpp"
#include <fstream>
#include <vector>
#include <sstream>

#ifdef __APPLE__
#include <mach/mach.h>
#elif __linux__
#include <sys/resource.h>
#include <fstream>
#endif

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
        file.clear();
    }
    
    if (!found) {
        return docs;
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

// Get current memory usage in bytes
size_t getCurrentMemoryUsage() {
#ifdef __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &infoCount) == KERN_SUCCESS) {
        return info.resident_size;
    }
    return 0;
#elif __linux__
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line.substr(6));
            size_t kb;
            iss >> kb;
            return kb * 1024;
        }
    }
    return 0;
#else
    return 0;
#endif
}

static void BM_MemoryPerDocument(benchmark::State& state) {
    auto docs = loadWikipediaSample();
    if (docs.empty()) {
        state.SkipWithError("No Wikipedia sample data found");
        return;
    }
    
    int num_docs = state.range(0);
    SearchEngine* engine = nullptr;
    
    for (auto _ : state) {
        state.PauseTiming();
        
        size_t mem_before = getCurrentMemoryUsage();
        
        state.ResumeTiming();
        
        engine = new SearchEngine();
        for (int i = 0; i < num_docs && i < static_cast<int>(docs.size()); ++i) {
            Document doc;
            doc.id = i;
            doc.content = docs[i % docs.size()].first + " " + docs[i % docs.size()].second;
            engine->indexDocument(doc);
        }
        
        benchmark::DoNotOptimize(engine);
        
        state.PauseTiming();
        
        size_t mem_after = getCurrentMemoryUsage();
        
        if (mem_after > mem_before) {
            size_t memory_used = mem_after - mem_before;
            double bytes_per_doc = static_cast<double>(memory_used) / num_docs;
            state.counters["bytes_per_doc"] = benchmark::Counter(bytes_per_doc);
            state.counters["total_memory_kb"] = benchmark::Counter(memory_used / 1024.0);
        }
        
        delete engine;
        engine = nullptr;
        
        state.ResumeTiming();
    }
}

BENCHMARK(BM_MemoryPerDocument)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(100);  // Fixed iterations for consistent memory measurement

static void BM_IndexSize(benchmark::State& state) {
    auto docs = loadWikipediaSample();
    if (docs.empty()) {
        state.SkipWithError("No Wikipedia sample data found");
        return;
    }
    
    size_t total_corpus_size = 0;
    for (const auto& doc : docs) {
        total_corpus_size += doc.first.size() + doc.second.size();
    }
    
    SearchEngine* engine = nullptr;
    
    for (auto _ : state) {
        state.PauseTiming();
        
        size_t mem_before = getCurrentMemoryUsage();
        
        state.ResumeTiming();
        
        engine = new SearchEngine();
        for (size_t i = 0; i < docs.size(); ++i) {
            Document doc;
            doc.id = i;
            doc.content = docs[i].first + " " + docs[i].second;
            engine->indexDocument(doc);
        }
        
        benchmark::DoNotOptimize(engine);
        
        state.PauseTiming();
        
        size_t mem_after = getCurrentMemoryUsage();
        size_t index_size = mem_after > mem_before ? (mem_after - mem_before) : 0;
        
        double compression_ratio = total_corpus_size > 0 ? 
            static_cast<double>(index_size) / total_corpus_size : 0;
        
        state.counters["corpus_size_kb"] = benchmark::Counter(total_corpus_size / 1024.0);
        state.counters["index_size_kb"] = benchmark::Counter(index_size / 1024.0);
        state.counters["index_size_mb"] = benchmark::Counter(index_size / (1024.0 * 1024.0));
        state.counters["compression_ratio"] = benchmark::Counter(compression_ratio);
        
        delete engine;
        engine = nullptr;
        
        state.ResumeTiming();
    }
}

BENCHMARK(BM_IndexSize)->Unit(benchmark::kMicrosecond)->Iterations(100);  // Fixed iterations for consistent memory measurement

BENCHMARK_MAIN();
