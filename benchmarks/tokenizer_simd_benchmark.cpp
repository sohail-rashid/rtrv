#include <benchmark/benchmark.h>
#include "tokenizer.hpp"
#include <fstream>
#include <vector>
#include <random>
#include <sstream>
#include <iostream>

using namespace search_engine;

// Helper function to generate test text of varying sizes
std::string generateTestText(size_t word_count) {
    std::vector<std::string> words = {
        "The", "quick", "brown", "fox", "jumps", "over", "the", "lazy", "dog",
        "Computer", "science", "is", "the", "study", "of", "computation", "and", "information",
        "Algorithm", "design", "requires", "careful", "analysis", "of", "complexity",
        "Data", "structures", "organize", "and", "store", "information", "efficiently",
        "Machine", "learning", "enables", "computers", "to", "learn", "from", "experience",
        "Natural", "language", "processing", "helps", "computers", "understand", "human", "text",
        "Artificial", "intelligence", "systems", "perform", "tasks", "requiring", "human", "cognition",
        "Software", "engineering", "practices", "improve", "code", "quality", "and", "maintainability",
        "Database", "management", "systems", "efficiently", "store", "and", "retrieve", "data",
        "Network", "protocols", "enable", "communication", "between", "distributed", "systems"
    };
    
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<size_t> dist(0, words.size() - 1);
    
    std::ostringstream oss;
    for (size_t i = 0; i < word_count; ++i) {
        oss << words[dist(rng)];
        if (i < word_count - 1) {
            oss << " ";
        }
    }
    
    return oss.str();
}

// Helper to load real Wikipedia data if available
std::string loadWikipediaText() {
    std::vector<std::string> paths = {
        "data/wikipedia_sample.txt",
        "../data/wikipedia_sample.txt",
        "../../data/wikipedia_sample.txt"
    };
    
    for (const auto& path : paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }
    
    // Return generated text if file not found
    return generateTestText(1000);
}

// Benchmark: SIMD-enabled tokenization (short text)
static void BM_Tokenize_SIMD_Short(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(true);
    std::string text = generateTestText(50); // ~50 words
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenize(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_Tokenize_SIMD_Short);

// Benchmark: Scalar tokenization (short text)
static void BM_Tokenize_Scalar_Short(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(false);
    std::string text = generateTestText(50); // ~50 words
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenize(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_Tokenize_Scalar_Short);

// Benchmark: SIMD-enabled tokenization (medium text)
static void BM_Tokenize_SIMD_Medium(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(true);
    std::string text = generateTestText(500); // ~500 words
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenize(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_Tokenize_SIMD_Medium);

// Benchmark: Scalar tokenization (medium text)
static void BM_Tokenize_Scalar_Medium(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(false);
    std::string text = generateTestText(500); // ~500 words
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenize(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_Tokenize_Scalar_Medium);

// Benchmark: SIMD-enabled tokenization (long text)
static void BM_Tokenize_SIMD_Long(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(true);
    std::string text = generateTestText(5000); // ~5000 words
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenize(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_Tokenize_SIMD_Long);

// Benchmark: Scalar tokenization (long text)
static void BM_Tokenize_Scalar_Long(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(false);
    std::string text = generateTestText(5000); // ~5000 words
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenize(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_Tokenize_Scalar_Long);

// Benchmark: SIMD tokenization with position tracking
static void BM_TokenizeWithPositions_SIMD(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(true);
    size_t text_size = state.range(0);
    std::string text = generateTestText(text_size);
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenizeWithPositions(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_TokenizeWithPositions_SIMD)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark: Scalar tokenization with position tracking
static void BM_TokenizeWithPositions_Scalar(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(false);
    size_t text_size = state.range(0);
    std::string text = generateTestText(text_size);
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenizeWithPositions(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_TokenizeWithPositions_Scalar)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark: Batch processing - SIMD
static void BM_BatchTokenize_SIMD(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(true);
    
    int batch_size = state.range(0);
    std::vector<std::string> texts;
    texts.reserve(batch_size);
    
    for (int i = 0; i < batch_size; ++i) {
        texts.push_back(generateTestText(100));
    }
    
    size_t total_bytes = 0;
    for (const auto& text : texts) {
        total_bytes += text.size();
    }
    
    for (auto _ : state) {
        for (const auto& text : texts) {
            auto tokens = tokenizer.tokenize(text);
            benchmark::DoNotOptimize(tokens);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetBytesProcessed(state.iterations() * total_bytes);
}

BENCHMARK(BM_BatchTokenize_SIMD)->Arg(10)->Arg(100)->Arg(1000);

// Benchmark: Batch processing - Scalar
static void BM_BatchTokenize_Scalar(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(false);
    
    int batch_size = state.range(0);
    std::vector<std::string> texts;
    texts.reserve(batch_size);
    
    for (int i = 0; i < batch_size; ++i) {
        texts.push_back(generateTestText(100));
    }
    
    size_t total_bytes = 0;
    for (const auto& text : texts) {
        total_bytes += text.size();
    }
    
    for (auto _ : state) {
        for (const auto& text : texts) {
            auto tokens = tokenizer.tokenize(text);
            benchmark::DoNotOptimize(tokens);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetBytesProcessed(state.iterations() * total_bytes);
}

BENCHMARK(BM_BatchTokenize_Scalar)->Arg(10)->Arg(100)->Arg(1000);

// Benchmark: Lowercase normalization only - SIMD
static void BM_Lowercase_SIMD(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(true);
    tokenizer.setRemoveStopwords(false); // Disable stopword removal for pure lowercase test
    
    std::string text = generateTestText(state.range(0));
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenize(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_Lowercase_SIMD)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark: Lowercase normalization only - Scalar
static void BM_Lowercase_Scalar(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(false);
    tokenizer.setRemoveStopwords(false); // Disable stopword removal for pure lowercase test
    
    std::string text = generateTestText(state.range(0));
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenize(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_Lowercase_Scalar)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark: Real Wikipedia data - SIMD
static void BM_RealData_SIMD(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(true);
    std::string text = loadWikipediaText();
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenize(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_RealData_SIMD);

// Benchmark: Real Wikipedia data - Scalar
static void BM_RealData_Scalar(benchmark::State& state) {
    Tokenizer tokenizer;
    tokenizer.enableSIMD(false);
    std::string text = loadWikipediaText();
    
    for (auto _ : state) {
        auto tokens = tokenizer.tokenize(text);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * text.size());
}

BENCHMARK(BM_RealData_Scalar);

// Custom main to display SIMD support information
int main(int argc, char** argv) {
    std::cout << "=================================================\n";
    std::cout << "     Tokenizer SIMD vs Scalar Benchmark\n";
    std::cout << "=================================================\n\n";
    
    std::cout << "SIMD Support Detection:\n";
    std::cout << "  Available: " << (Tokenizer::detectSIMDSupport() ? "YES" : "NO") << "\n";
    
#ifdef __AVX2__
    std::cout << "  Type: AVX2 (256-bit vectors)\n";
    std::cout << "  Processes: 32 bytes per iteration\n";
#elif defined(__SSE4_2__)
    std::cout << "  Type: SSE4.2 (128-bit vectors)\n";
    std::cout << "  Processes: 16 bytes per iteration\n";
#elif defined(__ARM_NEON) || defined(__aarch64__)
    std::cout << "  Type: ARM NEON (128-bit vectors)\n";
    std::cout << "  Processes: 16 bytes per iteration\n";
    std::cout << "  Architecture: Apple Silicon (M-series)\n";
#else
    std::cout << "  Type: None (Scalar fallback)\n";
    std::cout << "  Processes: 1 byte per iteration\n";
#endif
    
    std::cout << "\nBenchmark Categories:\n";
    std::cout << "  - Short text: ~50 words\n";
    std::cout << "  - Medium text: ~500 words\n";
    std::cout << "  - Long text: ~5000 words\n";
    std::cout << "  - Batch processing: Multiple documents\n";
    std::cout << "  - Real data: Wikipedia sample\n";
    std::cout << "\n=================================================\n\n";
    
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
