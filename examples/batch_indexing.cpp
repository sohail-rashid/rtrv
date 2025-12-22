#include "search_engine.hpp"
#include "document_loader.hpp"
#include <iostream>
#include <chrono>

using namespace search_engine;

int main(int argc, char* argv[]) {
    std::string corpus_file = "../data/wikipedia_sample.json";
    
    if (argc >= 2) {
        corpus_file = argv[1];
    }
    
    SearchEngine engine;
    
    std::cout << "Batch Indexing Example\n";
    std::cout << "======================\n\n";
    
    std::cout << "Loading documents from " << corpus_file << "...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Load documents using DocumentLoader
    DocumentLoader loader;
    std::vector<Document> documents;
    
    try {
        documents = loader.loadJSONL(corpus_file);
    } catch (const std::exception& e) {
        std::cerr << "Error loading documents: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "Loaded " << documents.size() << " documents from file.\n";
    std::cout << "Indexing documents...\n";
    
    // Index all documents
    size_t count = 0;
    for (const auto& doc : documents) {
        engine.indexDocument(doc);
        count++;
        
        // Progress indicator every 10 documents
        if (count % 10 == 0) {
            std::cout << "  Indexed " << count << " documents...\r" << std::flush;
        }
    }
    
    if (count >= 10) {
        std::cout << std::endl;  // New line after progress indicator
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "\nIndexed " << count << " documents in " 
              << duration.count() << "ms\n";
    
    if (duration.count() > 0) {
        std::cout << "Throughput: " << (count * 1000.0 / duration.count()) 
                  << " docs/sec\n";
    }
    
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
