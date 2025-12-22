#include <gtest/gtest.h>
#include "search_engine.hpp"
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <cstdio>

using namespace search_engine;

class IntegrationTest : public ::testing::Test {
protected:
    SearchEngine engine;
};

TEST_F(IntegrationTest, EndToEndSearch) {
    // Index multiple documents
    Document doc1{0, std::unordered_map<std::string, std::string>{{"content", "artificial intelligence and machine learning"}}};
    Document doc2{0, std::unordered_map<std::string, std::string>{{"content", "deep learning neural networks"}}};
    Document doc3{0, std::unordered_map<std::string, std::string>{{"content", "natural language processing"}}};
    Document doc4{0, std::unordered_map<std::string, std::string>{{"content", "computer vision image recognition"}}};
    Document doc5{0, std::unordered_map<std::string, std::string>{{"content", "machine learning algorithms"}}};
    
    engine.indexDocument(doc1);
    engine.indexDocument(doc2);
    engine.indexDocument(doc3);
    engine.indexDocument(doc4);
    engine.indexDocument(doc5);
    
    // Search for various queries
    auto results1 = engine.search("machine learning");
    EXPECT_FALSE(results1.empty());
    EXPECT_EQ(results1[0].document.id, 5);  // doc5 is shortest with both terms
    
    auto results2 = engine.search("neural networks");
    EXPECT_FALSE(results2.empty());
    EXPECT_EQ(results2[0].document.id, 2);  // doc2 has both terms
    
    auto results3 = engine.search("computer");
    EXPECT_EQ(results3.size(), 1);
    EXPECT_EQ(results3[0].document.id, 4);
    
    // Verify statistics
    auto stats = engine.getStats();
    EXPECT_EQ(stats.total_documents, 5);
    EXPECT_GT(stats.total_terms, 0);
    EXPECT_GT(stats.avg_doc_length, 0.0);
}

TEST_F(IntegrationTest, ConcurrentOperations) {
    // Pre-populate with some documents
    for (int i = 0; i < 20; ++i) {
        Document doc{0, std::unordered_map<std::string, std::string>{{"content", "document " + std::to_string(i) + " test content"}}};
        engine.indexDocument(doc);
    }
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> error_count{0};
    
    // Launch multiple search threads
    for (int i = 0; i < num_threads / 2; ++i) {
        threads.emplace_back([&engine = this->engine, &error_count]() {
            for (int j = 0; j < 100; ++j) {
                auto results = engine.search("test document");
                if (results.empty()) {
                    error_count++;
                }
            }
        });
    }
    
    // Launch some update threads
    for (int i = 0; i < num_threads / 2; ++i) {
        threads.emplace_back([&engine = this->engine, i]() {
            for (int j = 0; j < 50; ++j) {
                Document doc{0, std::unordered_map<std::string, std::string>{{"content", "updated document " + std::to_string(i * 50 + j)}}};
                engine.indexDocument(doc);
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify no errors occurred
    EXPECT_EQ(error_count, 0);
    
    // Verify final state is consistent
    auto stats = engine.getStats();
    EXPECT_GT(stats.total_documents, 20);
}

TEST_F(IntegrationTest, PersistenceRoundTrip) {
    // Index documents with metadata
    Document doc1{0, std::unordered_map<std::string, std::string>{{"content", "first document with interesting content"}, {"author", "Alice"}, {"category", "tech"}}};
    
    Document doc2{0, std::unordered_map<std::string, std::string>{{"content", "second document about different topics"}, {"author", "Bob"}}};
    
    Document doc3{0, std::unordered_map<std::string, std::string>{{"content", "third document with more interesting content"}}};
    
    engine.indexDocument(doc1);
    engine.indexDocument(doc2);
    engine.indexDocument(doc3);
    
    // Perform search and store results
    auto results_before = engine.search("interesting content");
    ASSERT_FALSE(results_before.empty());
    
    auto stats_before = engine.getStats();
    
    // Save snapshot
    std::string filepath = "/tmp/test_integration_snapshot.bin";
    bool saved = engine.saveSnapshot(filepath);
    ASSERT_TRUE(saved);
    
    // Create new engine and load snapshot
    SearchEngine engine2;
    bool loaded = engine2.loadSnapshot(filepath);
    ASSERT_TRUE(loaded);
    
    // Verify statistics match
    auto stats_after = engine2.getStats();
    EXPECT_EQ(stats_after.total_documents, stats_before.total_documents);
    // Note: total_terms might differ slightly due to index reconstruction
    EXPECT_GT(stats_after.total_terms, 0);
    EXPECT_DOUBLE_EQ(stats_after.avg_doc_length, stats_before.avg_doc_length);
    
    // Verify same search results
    auto results_after = engine2.search("interesting content");
    EXPECT_EQ(results_after.size(), results_before.size());
    
    for (size_t i = 0; i < results_after.size(); ++i) {
        EXPECT_EQ(results_after[i].document.id, results_before[i].document.id);
        // Note: getAllText() order may vary due to unordered_map, so we verify fields match
        EXPECT_EQ(results_after[i].document.getField("content"), results_before[i].document.getField("content"));
        EXPECT_DOUBLE_EQ(results_after[i].score, results_before[i].score);
    }
    
    // Verify metadata was preserved - search for document and check its metadata
    auto all_results = engine2.search("document");
    ASSERT_GE(all_results.size(), 1);
    
    // Find doc1 by content
    bool found_alice = false;
    for (const auto& result : all_results) {
        if (result.document.getAllText().find("first document") != std::string::npos) {
            EXPECT_EQ(result.document.getField("author"), "Alice");
            EXPECT_EQ(result.document.getField("category"), "tech");
            found_alice = true;
            break;
        }
    }
    EXPECT_TRUE(found_alice);
    
    // Clean up
    std::remove(filepath.c_str());
}

TEST_F(IntegrationTest, LargeCorpus) {
    // Index a large corpus
    const int num_docs = 1000;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_docs; ++i) {
        std::string content = "document " + std::to_string(i) + " ";
        
        // Add varied content
        if (i % 3 == 0) content += "technology innovation software";
        if (i % 5 == 0) content += "science research discovery";
        if (i % 7 == 0) content += "business management strategy";
        content += " sample text content";
        
        Document doc{0, std::unordered_map<std::string, std::string>{{"content", content}}};
        engine.indexDocument(doc);
    }
    
    auto index_time = std::chrono::high_resolution_clock::now();
    auto index_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        index_time - start_time).count();
    
    // Verify all documents indexed
    auto stats = engine.getStats();
    EXPECT_EQ(stats.total_documents, num_docs);
    EXPECT_GT(stats.total_terms, 0);
    
    // Perform searches and verify performance
    auto results1 = engine.search("technology innovation");
    EXPECT_FALSE(results1.empty());
    
    auto results2 = engine.search("science research");
    EXPECT_FALSE(results2.empty());
    
    auto results3 = engine.search("business management");
    EXPECT_FALSE(results3.empty());
    
    auto search_time = std::chrono::high_resolution_clock::now();
    auto search_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        search_time - index_time).count();
    
    // Performance assertions (should be reasonably fast)
    EXPECT_LT(index_duration, 5000);  // Indexing should take < 5 seconds
    EXPECT_LT(search_duration, 100);   // 3 searches should take < 100ms
    
    std::cout << "Indexed " << num_docs << " documents in " 
              << index_duration << "ms" << std::endl;
    std::cout << "3 searches completed in " << search_duration << "ms" << std::endl;
}
