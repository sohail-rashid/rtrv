#include "search_engine.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>

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
    
    // Read and index documents
    // Expected format: doc_id|content (one per line)
    size_t count = 0;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        // Parse line: doc_id|content
        size_t pos = line.find('|');
        if (pos != std::string::npos) {
            std::string id_str = line.substr(0, pos);
            std::string content = line.substr(pos + 1);
            
            uint64_t doc_id = std::stoull(id_str);
            Document doc{doc_id, content};
            engine.indexDocument(doc);
            count++;
            
            // Progress indicator every 100 documents
            if (count % 100 == 0) {
                std::cout << "  Indexed " << count << " documents...\r" << std::flush;
            }
        }
    }
    
    if (count >= 100) {
        std::cout << std::endl;  // New line after progress indicator
    }
    
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
    
    // Save snapshot
    std::string snapshot_file = "index_snapshot.bin";
    std::cout << "\nSaving snapshot to " << snapshot_file << "...\n";
    if (engine.saveSnapshot(snapshot_file)) {
        std::cout << "Snapshot saved successfully.\n";
    } else {
        std::cerr << "Error: Failed to save snapshot.\n";
    }
    
    return 0;
}
