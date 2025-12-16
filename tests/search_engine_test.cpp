#include <gtest/gtest.h>
#include "search_engine.hpp"

using namespace search_engine;

class SearchEngineTest : public ::testing::Test {
protected:
    SearchEngine engine;
};

TEST_F(SearchEngineTest, IndexAndSearch) {
    // Index some documents
    Document doc1{0, "the quick fox"};
    Document doc2{0, "the lazy dog"};
    Document doc3{0, "quick brown dog"};
    
    uint64_t id1 = engine.indexDocument(doc1);
    uint64_t id2 = engine.indexDocument(doc2);
    uint64_t id3 = engine.indexDocument(doc3);
    
    EXPECT_EQ(id1, 1);
    EXPECT_EQ(id2, 2);
    EXPECT_EQ(id3, 3);
    
    // Search for documents
    auto results = engine.search("quick brown");
    
    EXPECT_FALSE(results.empty());
    EXPECT_EQ(results[0].document.id, id3); // doc1 has both terms
    EXPECT_GT(results[0].score, 0.0);
}

TEST_F(SearchEngineTest, UpdateDocument) {
    // Index a document
    Document doc{0, "original content"};
    uint64_t id = engine.indexDocument(doc);
    
    // Search for original content
    auto results1 = engine.search("original");
    EXPECT_EQ(results1.size(), 1);
    EXPECT_EQ(results1[0].document.id, id);
    
    // Update the document
    Document updated{id, "updated content"};
    bool success = engine.updateDocument(id, updated);
    EXPECT_TRUE(success);
    
    // Search for old content should return nothing
    auto results2 = engine.search("original");
    EXPECT_TRUE(results2.empty());
    
    // Search for new content should return the document
    auto results3 = engine.search("updated");
    EXPECT_EQ(results3.size(), 1);
    EXPECT_EQ(results3[0].document.id, id);
    
    // Try updating non-existent document
    bool fail = engine.updateDocument(9999, updated);
    EXPECT_FALSE(fail);
}

TEST_F(SearchEngineTest, DeleteDocument) {
    // Index documents
    Document doc1{0, "first document"};
    Document doc2{0, "second document"};
    
    uint64_t id1 = engine.indexDocument(doc1);
    uint64_t id2 = engine.indexDocument(doc2);
    
    // Verify both exist
    auto results1 = engine.search("document");
    EXPECT_EQ(results1.size(), 2);
    
    // Delete first document
    bool success = engine.deleteDocument(id1);
    EXPECT_TRUE(success);
    
    // Verify only second document remains
    auto results2 = engine.search("document");
    EXPECT_EQ(results2.size(), 1);
    EXPECT_EQ(results2[0].document.id, id2);
    
    // Try deleting non-existent document
    bool fail = engine.deleteDocument(9999);
    EXPECT_FALSE(fail);
}

TEST_F(SearchEngineTest, EmptySearch) {
    // Search on empty index
    auto results = engine.search("anything");
    EXPECT_TRUE(results.empty());
    
    // Empty query
    auto results2 = engine.search("");
    EXPECT_TRUE(results2.empty());
    
    // Whitespace only
    auto results3 = engine.search("   ");
    EXPECT_TRUE(results3.empty());
}

TEST_F(SearchEngineTest, NoResults) {
    // Index documents
    Document doc1{0, "cats and dogs"};
    Document doc2{0, "birds and fish"};
    
    engine.indexDocument(doc1);
    engine.indexDocument(doc2);
    
    // Search for term not in any document
    auto results = engine.search("elephants");
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchEngineTest, RankingOrder) {
    // Index documents with varying relevance
    Document doc1{0, "machine learning algorithms"};
    Document doc2{0, "algorithms and data structures"};
    Document doc3{0, "machine learning deep learning neural networks"};
    
    uint64_t id1 = engine.indexDocument(doc1);
    engine.indexDocument(doc2);
    engine.indexDocument(doc3);
    
    // Search for "machine learning"
    auto results = engine.search("machine learning");
    
    EXPECT_GE(results.size(), 2);
    
    // Results should be sorted by descending score
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i-1].score, results[i].score);
    }
    
    // Doc1 ranks highest due to BM25 length normalization favoring shorter docs with both terms
    // Doc3 has "learning" twice but is penalized for being longer
    EXPECT_EQ(results[0].document.id, id1);
}

TEST_F(SearchEngineTest, MaxResults) {
    // Index multiple documents
    for (int i = 0; i < 10; ++i) {
        Document doc{0, "test document number " + std::to_string(i)};
        engine.indexDocument(doc);
    }
    
    // Search with default max_results
    auto results1 = engine.search("test");
    EXPECT_EQ(results1.size(), 10);
    
    // Search with max_results = 3
    SearchOptions options;
    options.max_results = 3;
    auto results2 = engine.search("test", options);
    EXPECT_EQ(results2.size(), 3);
    
    // Search with max_results = 0 (should return all)
    options.max_results = 0;
    auto results3 = engine.search("test", options);
    EXPECT_TRUE(results3.empty()); // 0 means return 0 results
}

TEST_F(SearchEngineTest, Statistics) {
    // Check empty stats
    auto stats1 = engine.getStats();
    EXPECT_EQ(stats1.total_documents, 0);
    EXPECT_EQ(stats1.total_terms, 0);
    EXPECT_EQ(stats1.avg_doc_length, 0.0);
    
    // Index documents
    Document doc1{0, "short"};
    Document doc2{0, "this is a longer document"};
    
    engine.indexDocument(doc1);
    engine.indexDocument(doc2);
    
    // Check updated stats
    auto stats2 = engine.getStats();
    EXPECT_EQ(stats2.total_documents, 2);
    EXPECT_GT(stats2.total_terms, 0);
    EXPECT_GT(stats2.avg_doc_length, 0.0);
    
    // Average doc length should be between 1 and 5 tokens
    EXPECT_GE(stats2.avg_doc_length, 1.0);
    EXPECT_LE(stats2.avg_doc_length, 5.0);
}
