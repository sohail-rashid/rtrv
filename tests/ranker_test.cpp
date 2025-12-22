#include <gtest/gtest.h>
#include "ranker.hpp"

using namespace search_engine;

class RankerTest : public ::testing::Test {
protected:
    TfIdfRanker tfidf_ranker;
    Bm25Ranker bm25_ranker;
};

TEST_F(RankerTest, TfIdfBasicScoring) {
    // Create test document
    Document doc(1, {{"content", "the quick brown fox jumps over the lazy dog"}});
    doc.term_count = 9;
    
    // Create query
    Query query;
    query.terms = {"quick", "fox"};
    
    // Create index statistics
    IndexStats stats;
    stats.total_docs = 100;
    stats.avg_doc_length = 10.0;
    stats.doc_frequency["quick"] = 10;  // Common term
    stats.doc_frequency["fox"] = 5;     // Rarer term
    
    // Score the document
    double score = tfidf_ranker.score(query, doc, stats);
    
    // Score should be positive since both terms appear
    EXPECT_GT(score, 0.0);
    
    // Score with just "fox" (rarer) should be higher than just "quick" (common)
    Query query_fox;
    query_fox.terms = {"fox"};
    double score_fox = tfidf_ranker.score(query_fox, doc, stats);
    
    Query query_quick;
    query_quick.terms = {"quick"};
    double score_quick = tfidf_ranker.score(query_quick, doc, stats);
    
    EXPECT_GT(score_fox, score_quick);  // Rarer term should score higher
}

TEST_F(RankerTest, Bm25BasicScoring) {
    // Create test document
    Document doc(1, {{"content", "the quick brown fox jumps over the lazy dog"}});
    doc.term_count = 9;
    
    // Create query
    Query query;
    query.terms = {"quick", "fox"};
    
    // Create index statistics
    IndexStats stats;
    stats.total_docs = 100;
    stats.avg_doc_length = 10.0;
    stats.doc_frequency["quick"] = 10;
    stats.doc_frequency["fox"] = 5;
    
    // Score the document
    double score = bm25_ranker.score(query, doc, stats);
    
    // Score should be positive
    EXPECT_GT(score, 0.0);
    
    // Verify BM25 produces different scores than TF-IDF
    double tfidf_score = tfidf_ranker.score(query, doc, stats);
    EXPECT_NE(score, tfidf_score);
}

TEST_F(RankerTest, EmptyDocument) {
    // Create empty document
    Document doc(1, {{"content", ""}});
    doc.term_count = 0;
    
    // Create query
    Query query;
    query.terms = {"test"};
    
    // Create index statistics
    IndexStats stats;
    stats.total_docs = 100;
    stats.avg_doc_length = 10.0;
    stats.doc_frequency["test"] = 10;
    
    // Both rankers should return 0 for empty document
    double tfidf_score = tfidf_ranker.score(query, doc, stats);
    double bm25_score = bm25_ranker.score(query, doc, stats);
    
    EXPECT_EQ(tfidf_score, 0.0);
    EXPECT_EQ(bm25_score, 0.0);
}

TEST_F(RankerTest, RareTerms) {
    // Create document with rare term
    Document doc(1, {{"content", "unique specialized terminology"}});
    doc.term_count = 3;
    
    // Create query with rare term
    Query query;
    query.terms = {"specialized"};
    
    // Create index statistics - rare term in only 1 doc out of 1000
    IndexStats stats;
    stats.total_docs = 1000;
    stats.avg_doc_length = 10.0;
    stats.doc_frequency["specialized"] = 1;  // Very rare
    
    // Score should be high due to high IDF
    double score = tfidf_ranker.score(query, doc, stats);
    EXPECT_GT(score, 0.0);
    
    // Compare with common term
    Document doc2(2, {{"content", "the common word"}});
    doc2.term_count = 3;
    Query query2;
    query2.terms = {"common"};
    stats.doc_frequency["common"] = 500;  // Very common
    
    double common_score = tfidf_ranker.score(query2, doc2, stats);
    
    // Rare term should score much higher
    EXPECT_GT(score, common_score);
}

TEST_F(RankerTest, CommonTerms) {
    // Create document
    Document doc(1, {{"content", "the quick brown fox"}});
    doc.term_count = 4;
    
    // Create query with very common term
    Query query;
    query.terms = {"the"};
    
    // Create index statistics - "the" appears in most documents
    IndexStats stats;
    stats.total_docs = 100;
    stats.avg_doc_length = 10.0;
    stats.doc_frequency["the"] = 95;  // Appears in 95 out of 100 docs
    
    // Score should be low due to low IDF
    double score = tfidf_ranker.score(query, doc, stats);
    
    // Should still be positive but relatively low
    EXPECT_GT(score, 0.0);
    EXPECT_LT(score, 1.0);  // Common terms have low scores
}

TEST_F(RankerTest, Bm25LengthNormalization) {
    // Create two documents with same term but different lengths
    Document short_doc(1, {{"content", "fox"}});
    short_doc.term_count = 1;
    
    Document long_doc(2, {{"content", "the quick brown fox jumps over the lazy dog and many other words to make it longer"}});
    long_doc.term_count = 15;
    
    // Create query
    Query query;
    query.terms = {"fox"};
    
    // Create index statistics
    IndexStats stats;
    stats.total_docs = 100;
    stats.avg_doc_length = 10.0;
    stats.doc_frequency["fox"] = 10;
    
    // Score both documents
    double short_score = bm25_ranker.score(query, short_doc, stats);
    double long_score = bm25_ranker.score(query, long_doc, stats);
    
    // Both should be positive
    EXPECT_GT(short_score, 0.0);
    EXPECT_GT(long_score, 0.0);
    
    // Shorter document should score higher (length normalization)
    EXPECT_GT(short_score, long_score);
    
    // BM25 should handle length better than TF-IDF
    double tfidf_short = tfidf_ranker.score(query, short_doc, stats);
    double tfidf_long = tfidf_ranker.score(query, long_doc, stats);
    
    // BM25 should show more difference due to length normalization
    double bm25_ratio = short_score / long_score;
    double tfidf_ratio = tfidf_short / tfidf_long;
    
    EXPECT_GT(bm25_ratio, 1.0);  // BM25 favors shorter doc
    EXPECT_LT(tfidf_ratio, bm25_ratio);  // TF-IDF less sensitive to length
}
