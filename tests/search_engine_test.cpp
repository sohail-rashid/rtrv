#include <gtest/gtest.h>
#include "search_engine.hpp"
#include <cstdio>
#include <fstream>
#include <set>

using namespace rtrv_search_engine;

class SearchEngineTest : public ::testing::Test {
protected:
    SearchEngine engine;
};

TEST_F(SearchEngineTest, IndexAndSearch) {
    // Index some documents
    Document doc1{0, std::unordered_map<std::string, std::string>{{"content", "the quick fox"}}};
    Document doc2{0, std::unordered_map<std::string, std::string>{{"content", "the lazy dog"}}};
    Document doc3{0, std::unordered_map<std::string, std::string>{{"content", "quick brown dog"}}};
    
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
    Document doc{0, std::unordered_map<std::string, std::string>{{"content", "original content"}}};
    uint64_t id = engine.indexDocument(doc);
    
    // Search for original content
    auto results1 = engine.search("original");
    EXPECT_EQ(results1.size(), 1);
    EXPECT_EQ(results1[0].document.id, id);
    
    // Update the document
    Document updated{static_cast<uint32_t>(id), {{"content", "updated content"}}};
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
    Document doc1{0, std::unordered_map<std::string, std::string>{{"content", "first document"}}};
    Document doc2{0, std::unordered_map<std::string, std::string>{{"content", "second document"}}};
    
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
    Document doc1{0, std::unordered_map<std::string, std::string>{{"content", "cats and dogs"}}};
    Document doc2{0, std::unordered_map<std::string, std::string>{{"content", "birds and fish"}}};
    
    engine.indexDocument(doc1);
    engine.indexDocument(doc2);
    
    // Search for term not in any document
    auto results = engine.search("elephants");
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchEngineTest, RankingOrder) {
    // Index documents with varying relevance
    Document doc1{0, std::unordered_map<std::string, std::string>{{"content", "machine learning algorithms"}}};
    Document doc2{0, std::unordered_map<std::string, std::string>{{"content", "algorithms and data structures"}}};
    Document doc3{0, std::unordered_map<std::string, std::string>{{"content", "machine learning deep learning neural networks"}}};
    
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
        Document doc{0, std::unordered_map<std::string, std::string>{{"content", "test document number " + std::to_string(i)}}};
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
    Document doc1{0, std::unordered_map<std::string, std::string>{{"content", "short"}}};
    Document doc2{0, std::unordered_map<std::string, std::string>{{"content", "this is a longer document"}}};
    
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

TEST_F(SearchEngineTest, QueryCacheIntegration) {
    Document doc1{0, std::unordered_map<std::string, std::string>{{"content", "cache test content"}}};
    engine.indexDocument(doc1);

    SearchOptions options;
    options.use_cache = true;

    auto stats_before = engine.getCacheStats();
    EXPECT_EQ(stats_before.current_size, 0u);

    auto results1 = engine.search("cache test", options);
    EXPECT_FALSE(results1.empty());

    auto stats_after_first = engine.getCacheStats();
    EXPECT_EQ(stats_after_first.current_size, 1u);
    EXPECT_EQ(stats_after_first.miss_count, stats_before.miss_count + 1);

    auto results2 = engine.search("cache test", options);
    EXPECT_FALSE(results2.empty());

    auto stats_after_second = engine.getCacheStats();
    EXPECT_GE(stats_after_second.hit_count, stats_after_first.hit_count + 1);

    Document doc2{0, std::unordered_map<std::string, std::string>{{"content", "another cache test document"}}};
    engine.indexDocument(doc2);

    auto stats_after_invalidate = engine.getCacheStats();
    EXPECT_EQ(stats_after_invalidate.current_size, 0u);

    engine.search("cache test", options);
    auto stats_after_third = engine.getCacheStats();
    EXPECT_GE(stats_after_third.miss_count, stats_after_second.miss_count + 1);
}

TEST_F(SearchEngineTest, SaveSnapshot) {
    // Index some documents
    Document doc1{0, std::unordered_map<std::string, std::string>{{"content", "first document content"}, {"author", "Alice"}}};
    Document doc2{0, std::unordered_map<std::string, std::string>{{"content", "second document content"}, {"author", "Bob"}}};
    
    engine.indexDocument(doc1);
    engine.indexDocument(doc2);
    
    // Save snapshot
    std::string filepath = "/tmp/test_save_snapshot.bin";
    bool saved = engine.saveSnapshot(filepath);
    EXPECT_TRUE(saved);
    
    // Verify file exists (attempt to open for reading)
    std::ifstream check(filepath, std::ios::binary);
    EXPECT_TRUE(check.good());
    check.close();
    
    // Clean up
    std::remove(filepath.c_str());
}

TEST_F(SearchEngineTest, LoadSnapshot) {
    // Index documents
    Document doc1{0, std::unordered_map<std::string, std::string>{{"content", "test document one"}}};
    Document doc2{0, std::unordered_map<std::string, std::string>{{"content", "test document two"}}};
    
    engine.indexDocument(doc1);
    engine.indexDocument(doc2);
    
    // Save snapshot
    std::string filepath = "/tmp/test_load_snapshot.bin";
    engine.saveSnapshot(filepath);
    
    // Create new engine and load
    SearchEngine engine2;
    bool loaded = engine2.loadSnapshot(filepath);
    EXPECT_TRUE(loaded);
    
    // Verify documents were loaded
    auto stats = engine2.getStats();
    EXPECT_EQ(stats.total_documents, 2);
    
    // Verify search works
    auto results = engine2.search("document");
    EXPECT_EQ(results.size(), 2);
    
    // Clean up
    std::remove(filepath.c_str());
}

TEST_F(SearchEngineTest, SnapshotPreservesSearchResults) {
    // Index documents with specific content
    Document doc1{0, std::unordered_map<std::string, std::string>{{"content", "machine learning algorithms"}}};
    Document doc2{0, std::unordered_map<std::string, std::string>{{"content", "deep learning neural networks"}}};
    Document doc3{0, std::unordered_map<std::string, std::string>{{"content", "data structures and algorithms"}}};
    
    engine.indexDocument(doc1);
    engine.indexDocument(doc2);
    engine.indexDocument(doc3);
    
    // Perform search before save
    auto results_before = engine.search("algorithms");
    ASSERT_FALSE(results_before.empty());
    
    // Save and load
    std::string filepath = "/tmp/test_snapshot_search.bin";
    engine.saveSnapshot(filepath);
    
    SearchEngine engine2;
    engine2.loadSnapshot(filepath);
    
    // Perform same search after load
    auto results_after = engine2.search("algorithms");
    
    // Verify results match
    EXPECT_EQ(results_after.size(), results_before.size());
    for (size_t i = 0; i < results_after.size(); ++i) {
        EXPECT_EQ(results_after[i].document.id, results_before[i].document.id);
        EXPECT_EQ(results_after[i].document.getAllText(), results_before[i].document.getAllText());
        EXPECT_DOUBLE_EQ(results_after[i].score, results_before[i].score);
    }
    
    // Clean up
    std::remove(filepath.c_str());
}

TEST_F(SearchEngineTest, SnapshotPreservesMetadata) {
    // Index document with metadata
    Document doc{0, std::unordered_map<std::string, std::string>{{"content", "document with metadata"}, {"title", "Test Document"}, {"author", "John Doe"}, {"year", "2025"}}};
    
    uint64_t doc_id = engine.indexDocument(doc);
    
    // Save and load
    std::string filepath = "/tmp/test_snapshot_metadata.bin";
    engine.saveSnapshot(filepath);
    
    SearchEngine engine2;
    engine2.loadSnapshot(filepath);
    
    // Search and verify metadata
    auto results = engine2.search("metadata");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].document.id, doc_id);
    EXPECT_EQ(results[0].document.getField("title"), "Test Document");
    EXPECT_EQ(results[0].document.getField("author"), "John Doe");
    EXPECT_EQ(results[0].document.getField("year"), "2025");
    
    // Clean up
    std::remove(filepath.c_str());
}

TEST_F(SearchEngineTest, LoadNonExistentFile) {
    SearchEngine engine2;
    bool loaded = engine2.loadSnapshot("/tmp/nonexistent_file.bin");
    EXPECT_FALSE(loaded);
}

TEST_F(SearchEngineTest, SaveToInvalidPath) {
    Document doc{0, std::unordered_map<std::string, std::string>{{"content", "test"}}};
    engine.indexDocument(doc);
    
    // Try to save to invalid path
    bool saved = engine.saveSnapshot("/invalid/path/that/does/not/exist/snapshot.bin");
    EXPECT_FALSE(saved);
}

TEST_F(SearchEngineTest, SnapshotEmptyEngine) {
    // Save empty engine
    std::string filepath = "/tmp/test_empty_snapshot.bin";
    bool saved = engine.saveSnapshot(filepath);
    EXPECT_TRUE(saved);
    
    // Load into another engine
    SearchEngine engine2;
    bool loaded = engine2.loadSnapshot(filepath);
    EXPECT_TRUE(loaded);
    
    // Verify it's empty
    auto stats = engine2.getStats();
    EXPECT_EQ(stats.total_documents, 0);
    EXPECT_EQ(stats.total_terms, 0);
    
    // Clean up
    std::remove(filepath.c_str());
}

// ============================================================================
// Top-K Heap Tests
// ============================================================================

TEST_F(SearchEngineTest, TopKHeapVsTraditional) {
    // Index multiple documents
    for (int i = 0; i < 20; ++i) {
        Document doc{0, {{"content", "term" + std::to_string(i % 5) + " document number " + std::to_string(i)}}};
        engine.indexDocument(doc);
    }
    
    // Search with Top-K heap
    SearchOptions heap_opts;
    heap_opts.max_results = 5;
    heap_opts.use_top_k_heap = true;
    auto heap_results = engine.search("term1", heap_opts);
    
    // Search with traditional sort
    SearchOptions sort_opts;
    sort_opts.max_results = 5;
    sort_opts.use_top_k_heap = false;
    auto sort_results = engine.search("term1", sort_opts);
    
    // Both should return same number of results
    EXPECT_EQ(heap_results.size(), sort_results.size());
    
    // Both should have same scores (though order may differ for equal scores)
    ASSERT_FALSE(heap_results.empty());
    ASSERT_FALSE(sort_results.empty());
    
    // Collect scores from both results
    std::multiset<double> heap_scores;
    std::multiset<double> sort_scores;
    for (const auto& r : heap_results) {
        heap_scores.insert(r.score);
    }
    for (const auto& r : sort_results) {
        sort_scores.insert(r.score);
    }
    
    // Scores should match
    EXPECT_EQ(heap_scores, sort_scores);
}

TEST_F(SearchEngineTest, TopKHeapWithSmallK) {
    // Index 100 documents
    for (int i = 0; i < 100; ++i) {
        Document doc{0, {{"content", "common rare" + std::to_string(i)}}};
        engine.indexDocument(doc);
    }
    
    // Request only top 3 results
    SearchOptions opts;
    opts.max_results = 3;
    opts.use_top_k_heap = true;
    
    auto results = engine.search("common", opts);
    
    EXPECT_LE(results.size(), 3);
    
    // Verify descending order
    for (size_t i = 0; i < results.size() - 1; ++i) {
        EXPECT_GE(results[i].score, results[i + 1].score);
    }
}

TEST_F(SearchEngineTest, TopKHeapLargeDataset) {
    // Index many documents to test O(N log K) efficiency
    for (int i = 0; i < 500; ++i) {
        std::string content = "document " + std::to_string(i);
        if (i % 10 == 0) {
            content += " important keyword";
        }
        Document doc{0, {{"content", content}}};
        engine.indexDocument(doc);
    }
    
    SearchOptions opts;
    opts.max_results = 10;
    opts.use_top_k_heap = true;
    
    auto results = engine.search("important keyword", opts);
    
    EXPECT_LE(results.size(), 10);
    EXPECT_GT(results.size(), 0);
    
    // All results should have positive scores
    for (const auto& result : results) {
        EXPECT_GT(result.score, 0.0);
    }
}

TEST_F(SearchEngineTest, TopKHeapExplainScores) {
    // Index a few documents
    Document doc1{0, {{"content", "machine learning artificial intelligence"}}};
    Document doc2{0, {{"content", "machine learning deep neural networks"}}};
    Document doc3{0, {{"content", "machine learning basics tutorial"}}};
    
    engine.indexDocument(doc1);
    engine.indexDocument(doc2);
    engine.indexDocument(doc3);
    
    // Search with explanations using heap
    SearchOptions heap_opts;
    heap_opts.max_results = 3;
    heap_opts.use_top_k_heap = true;
    heap_opts.explain_scores = true;
    
    auto heap_results = engine.search("machine learning", heap_opts);
    ASSERT_FALSE(heap_results.empty());
    EXPECT_FALSE(heap_results[0].explanation.empty());
    EXPECT_NE(heap_results[0].explanation.find("Top-K Heap"), std::string::npos);
    
    // Search with explanations using sort
    SearchOptions sort_opts;
    sort_opts.max_results = 3;
    sort_opts.use_top_k_heap = false;
    sort_opts.explain_scores = true;
    
    auto sort_results = engine.search("machine learning", sort_opts);
    ASSERT_FALSE(sort_results.empty());
    EXPECT_FALSE(sort_results[0].explanation.empty());
    EXPECT_NE(sort_results[0].explanation.find("Full Sort"), std::string::npos);
}

TEST_F(SearchEngineTest, TopKHeapWithDifferentRankers) {
    // Index documents with clear score differences
    Document doc1{0, {{"content", "apple orange banana"}}};
    Document doc2{0, {{"content", "apple apple apple"}}};
    Document doc3{0, {{"content", "orange banana fruit"}}};
    
    engine.indexDocument(doc1);
    engine.indexDocument(doc2);
    engine.indexDocument(doc3);
    
    // Test with BM25 (default)
    SearchOptions bm25_opts;
    bm25_opts.use_top_k_heap = true;
    bm25_opts.ranker_name = "BM25";
    auto bm25_results = engine.search("apple", bm25_opts);
    EXPECT_FALSE(bm25_results.empty());
    
    // Test with TF-IDF
    SearchOptions tfidf_opts;
    tfidf_opts.use_top_k_heap = true;
    tfidf_opts.ranker_name = "TF-IDF";
    auto tfidf_results = engine.search("apple", tfidf_opts);
    EXPECT_FALSE(tfidf_results.empty());
    
    // Both should find documents containing "apple"
    // Doc1 and Doc2 both contain "apple", doc2 has more occurrences
    // Just verify we get results with positive scores
    EXPECT_GT(bm25_results[0].score, 0.0);
    EXPECT_GT(tfidf_results[0].score, 0.0);
    
    // Verify doc2 (with most "apple" occurrences) is in the results
    bool doc2_in_bm25 = false;
    bool doc2_in_tfidf = false;
    for (const auto& r : bm25_results) {
        if (r.document.id == 2) doc2_in_bm25 = true;
    }
    for (const auto& r : tfidf_results) {
        if (r.document.id == 2) doc2_in_tfidf = true;
    }
    EXPECT_TRUE(doc2_in_bm25);
    EXPECT_TRUE(doc2_in_tfidf);
}

TEST_F(SearchEngineTest, TopKHeapEmptyResults) {
    // Index a document
    Document doc{0, {{"content", "hello world"}}};
    engine.indexDocument(doc);
    
    // Search for non-existent term
    SearchOptions opts;
    opts.use_top_k_heap = true;
    auto results = engine.search("nonexistent", opts);
    
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchEngineTest, TopKHeapSingleResult) {
    // Index one document
    Document doc{0, {{"content", "unique document content"}}};
    engine.indexDocument(doc);
    
    SearchOptions opts;
    opts.max_results = 10;
    opts.use_top_k_heap = true;
    auto results = engine.search("unique", opts);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].document.id, 1);
}

TEST_F(SearchEngineTest, TopKHeapExactlyK) {
    // Index exactly K documents
    for (int i = 0; i < 5; ++i) {
        Document doc{0, {{"content", "search term document " + std::to_string(i)}}};
        engine.indexDocument(doc);
    }
    
    SearchOptions opts;
    opts.max_results = 5;
    opts.use_top_k_heap = true;
    auto results = engine.search("search term", opts);
    
    EXPECT_EQ(results.size(), 5);
}

// ============================================================================
// Pagination Tests
// ============================================================================

TEST_F(SearchEngineTest, PaginatedSearchBasic) {
    // Index 20 documents
    for (int i = 0; i < 20; ++i) {
        Document doc{0, {{"content", "common search term document " + std::to_string(i)}}};
        engine.indexDocument(doc);
    }

    SearchOptions opts;
    opts.max_results = 5;
    opts.offset = 0;
    auto page1 = engine.searchPaginated("common search", opts);

    EXPECT_EQ(page1.results.size(), 5);
    EXPECT_EQ(page1.pagination.total_hits, 20);
    EXPECT_EQ(page1.pagination.offset, 0);
    EXPECT_EQ(page1.pagination.page_size, 5);
    EXPECT_TRUE(page1.pagination.has_next_page);
}

TEST_F(SearchEngineTest, PaginatedSearchOffsetPages) {
    // Index 15 documents
    for (int i = 0; i < 15; ++i) {
        Document doc{0, {{"content", "paging test document " + std::to_string(i)}}};
        engine.indexDocument(doc);
    }

    // Page 1: offset=0, size=5
    SearchOptions opts;
    opts.max_results = 5;
    opts.offset = 0;
    auto page1 = engine.searchPaginated("paging test", opts);

    EXPECT_EQ(page1.results.size(), 5);
    EXPECT_EQ(page1.pagination.total_hits, 15);
    EXPECT_TRUE(page1.pagination.has_next_page);

    // Page 2: offset=5, size=5
    opts.offset = 5;
    auto page2 = engine.searchPaginated("paging test", opts);

    EXPECT_EQ(page2.results.size(), 5);
    EXPECT_EQ(page2.pagination.offset, 5);
    EXPECT_TRUE(page2.pagination.has_next_page);

    // Page 3: offset=10, size=5
    opts.offset = 10;
    auto page3 = engine.searchPaginated("paging test", opts);

    EXPECT_EQ(page3.results.size(), 5);
    EXPECT_EQ(page3.pagination.offset, 10);
    EXPECT_FALSE(page3.pagination.has_next_page);

    // Verify no overlap between pages
    std::set<uint32_t> all_ids;
    for (const auto& r : page1.results) all_ids.insert(r.document.id);
    for (const auto& r : page2.results) all_ids.insert(r.document.id);
    for (const auto& r : page3.results) all_ids.insert(r.document.id);
    EXPECT_EQ(all_ids.size(), 15);
}

TEST_F(SearchEngineTest, PaginatedSearchBeyondResults) {
    // Index 5 documents
    for (int i = 0; i < 5; ++i) {
        Document doc{0, {{"content", "beyond test doc " + std::to_string(i)}}};
        engine.indexDocument(doc);
    }

    // Offset past all results
    SearchOptions opts;
    opts.max_results = 5;
    opts.offset = 100;
    auto page = engine.searchPaginated("beyond test", opts);

    EXPECT_TRUE(page.results.empty());
    EXPECT_EQ(page.pagination.total_hits, 5);
    EXPECT_FALSE(page.pagination.has_next_page);
}

TEST_F(SearchEngineTest, PaginatedSearchPartialLastPage) {
    // Index 7 documents
    for (int i = 0; i < 7; ++i) {
        Document doc{0, {{"content", "partial page document " + std::to_string(i)}}};
        engine.indexDocument(doc);
    }

    // Request page size 5, offset 5 → should get 2 results
    SearchOptions opts;
    opts.max_results = 5;
    opts.offset = 5;
    auto page = engine.searchPaginated("partial page", opts);

    EXPECT_EQ(page.results.size(), 2);
    EXPECT_EQ(page.pagination.page_size, 2);
    EXPECT_FALSE(page.pagination.has_next_page);
    EXPECT_EQ(page.pagination.total_hits, 7);
}

TEST_F(SearchEngineTest, PaginatedSearchCursorBased) {
    // Index 10 documents
    for (int i = 0; i < 10; ++i) {
        Document doc{0, {{"content", "cursor test document " + std::to_string(i)}}};
        engine.indexDocument(doc);
    }

    // Get first page
    SearchOptions opts;
    opts.max_results = 3;
    auto page1 = engine.searchPaginated("cursor test", opts);

    ASSERT_EQ(page1.results.size(), 3);
    EXPECT_TRUE(page1.pagination.has_next_page);

    // Use cursor from last result of page 1 to get page 2
    SearchOptions opts2;
    opts2.max_results = 3;
    opts2.search_after_score = page1.results.back().score;
    opts2.search_after_id = page1.results.back().document.id;
    auto page2 = engine.searchPaginated("cursor test", opts2);

    ASSERT_FALSE(page2.results.empty());
    EXPECT_LE(page2.results.size(), 3);

    // Verify page2 results come after page1 results (lower or equal scores)
    double last_page1_score = page1.results.back().score;
    for (const auto& r : page2.results) {
        EXPECT_LE(r.score, last_page1_score);
    }

    // Verify no overlap between pages
    std::set<uint32_t> page1_ids, page2_ids;
    for (const auto& r : page1.results) page1_ids.insert(r.document.id);
    for (const auto& r : page2.results) page2_ids.insert(r.document.id);
    for (auto id : page2_ids) {
        EXPECT_EQ(page1_ids.count(id), 0) << "Document " << id << " appears in both pages";
    }
}

TEST_F(SearchEngineTest, PaginatedSearchEmptyQuery) {
    Document doc{0, {{"content", "test document"}}};
    engine.indexDocument(doc);

    auto page = engine.searchPaginated("", SearchOptions{});
    EXPECT_TRUE(page.results.empty());
    EXPECT_EQ(page.pagination.total_hits, 0);
    EXPECT_FALSE(page.pagination.has_next_page);
}

TEST_F(SearchEngineTest, PaginatedSearchNoMatches) {
    Document doc{0, {{"content", "hello world"}}};
    engine.indexDocument(doc);

    auto page = engine.searchPaginated("nonexistent", SearchOptions{});
    EXPECT_TRUE(page.results.empty());
    EXPECT_EQ(page.pagination.total_hits, 0);
    EXPECT_FALSE(page.pagination.has_next_page);
}

TEST_F(SearchEngineTest, PaginatedSearchSinglePage) {
    // Index 3 documents, request page of 10
    for (int i = 0; i < 3; ++i) {
        Document doc{0, {{"content", "single page test " + std::to_string(i)}}};
        engine.indexDocument(doc);
    }

    SearchOptions opts;
    opts.max_results = 10;
    auto page = engine.searchPaginated("single page", opts);

    EXPECT_EQ(page.results.size(), 3);
    EXPECT_EQ(page.pagination.total_hits, 3);
    EXPECT_FALSE(page.pagination.has_next_page);
}
