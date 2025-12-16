#include "search_engine.hpp"
#include <iostream>

using namespace search_engine;

int main() {
    SearchEngine engine;
    
    // TODO: Implement simple search example
    // Index a few documents
    // Perform searches
    // Display results
    
    std::cout << "Search Engine - Simple Example\n";
    std::cout << "===============================\n\n";
    
    // Index documents
    std::cout << "Indexing documents...\n";
    engine.indexDocument({1, "The quick brown fox jumps over the lazy dog"});
    engine.indexDocument({2, "A quick brown dog runs in the park"});
    engine.indexDocument({3, "The lazy cat sleeps all day"});
    
    // Search
    std::cout << "\nSearching for 'quick dog'...\n";
    auto results = engine.search("quick dog");
    
    // Display results
    std::cout << "\nResults:\n";
    for (const auto& result : results) {
        std::cout << "Doc " << result.document.id 
                  << " (score: " << result.score << "): "
                  << result.document.content << "\n";
    }
    
    return 0;
}
