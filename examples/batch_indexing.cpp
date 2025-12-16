#include "search_engine.hpp"
#include <iostream>
#include <fstream>
#include <chrono>

using namespace search_engine;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <corpus_file>\n";
        return 1;
    }
    
    SearchEngine engine;
    
    // TODO: Implement batch indexing from file
    // Read documents from file
    // Measure indexing throughput
    // Save snapshot
    
    std::cout << "Batch Indexing Example\n";
    std::cout << "======================\n\n";
    
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: Cannot open file " << argv[1] << "\n";
        return 1;
    }
    
    std::cout << "Reading documents from " << argv[1] << "...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // TODO: Read and index documents
    size_t count = 0;
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Indexed " << count << " documents in " 
              << duration.count() << "ms\n";
    std::cout << "Throughput: " << (count * 1000.0 / duration.count()) 
              << " docs/sec\n";
    
    // Display statistics
    auto stats = engine.getStats();
    std::cout << "\nIndex Statistics:\n";
    std::cout << "  Total documents: " << stats.total_documents << "\n";
    std::cout << "  Total terms: " << stats.total_terms << "\n";
    std::cout << "  Avg doc length: " << stats.avg_doc_length << "\n";
    
    return 0;
}
