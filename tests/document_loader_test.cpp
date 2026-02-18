#include <gtest/gtest.h>
#include "document_loader.hpp"
#include <fstream>
#include <filesystem>

using namespace rtrv_search_engine;

// Test fixture for document loader tests
class DocumentLoaderTest : public ::testing::Test {
protected:
    std::string test_dir;
    
    void SetUp() override {
        // Create temp directory for test files
        test_dir = "/tmp/doc_loader_test";
        std::filesystem::create_directories(test_dir);
    }
    
    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all(test_dir);
    }
    
    std::string createTestFile(const std::string& filename, const std::string& content) {
        std::string filepath = test_dir + "/" + filename;
        std::ofstream file(filepath);
        file << content;
        file.close();
        return filepath;
    }
};

// Test loadJSONL with simple JSON objects
TEST_F(DocumentLoaderTest, LoadJSONL_SimpleObjects) {
    std::string content = 
        R"({"title": "First Document", "content": "This is the first document"})" "\n"
        R"({"title": "Second Document", "content": "This is the second document"})" "\n"
        R"({"title": "Third Document", "content": "This is the third document"})" "\n";
    
    std::string filepath = createTestFile("test.jsonl", content);
    
    DocumentLoader loader;
    auto docs = loader.loadJSONL(filepath);
    
    EXPECT_EQ(docs.size(), 3);
    
    // Check first document (DocumentLoader starts IDs at 1)
    EXPECT_EQ(docs[0].id, 1);
    EXPECT_EQ(docs[0].getField("title"), "First Document");
    EXPECT_EQ(docs[0].getField("content"), "This is the first document");
    EXPECT_GT(docs[0].term_count, 0);
    
    // Check second document
    EXPECT_EQ(docs[1].id, 2);
    EXPECT_EQ(docs[1].getField("title"), "Second Document");
    
    // Check third document
    EXPECT_EQ(docs[2].id, 3);
    EXPECT_EQ(docs[2].getField("title"), "Third Document");
}

// Test loadJSONL with empty lines
TEST_F(DocumentLoaderTest, LoadJSONL_EmptyLines) {
    std::string content = 
        R"({"title": "First", "content": "Content 1"})" "\n"
        "\n"  // Empty line
        R"({"title": "Second", "content": "Content 2"})" "\n"
        "\n"  // Another empty line
        R"({"title": "Third", "content": "Content 3"})" "\n";
    
    std::string filepath = createTestFile("test_empty_lines.jsonl", content);
    
    DocumentLoader loader;
    auto docs = loader.loadJSONL(filepath);
    
    EXPECT_EQ(docs.size(), 3);  // Should skip empty lines
}

// Test loadJSONL with null values
TEST_F(DocumentLoaderTest, LoadJSONL_NullValues) {
    std::string content = 
        R"({"title": "Document", "author": null, "content": "Some content"})" "\n";
    
    std::string filepath = createTestFile("test_nulls.jsonl", content);
    
    DocumentLoader loader;
    auto docs = loader.loadJSONL(filepath);
    
    EXPECT_EQ(docs.size(), 1);
    EXPECT_EQ(docs[0].getField("author"), "");  // Null becomes empty string
}

// Test loadJSONL with non-string values
TEST_F(DocumentLoaderTest, LoadJSONL_NonStringValues) {
    std::string content = 
        R"({"title": "Document", "year": 2024, "rating": 4.5, "published": true})" "\n";
    
    std::string filepath = createTestFile("test_types.jsonl", content);
    
    DocumentLoader loader;
    auto docs = loader.loadJSONL(filepath);
    
    EXPECT_EQ(docs.size(), 1);
    EXPECT_EQ(docs[0].getField("title"), "Document");
    EXPECT_EQ(docs[0].getField("year"), "2024");
    EXPECT_EQ(docs[0].getField("rating"), "4.5");
    EXPECT_EQ(docs[0].getField("published"), "true");
}

// Test loadJSONL with invalid JSON
TEST_F(DocumentLoaderTest, LoadJSONL_InvalidJSON) {
    std::string content = 
        R"({"title": "Valid"})" "\n"
        R"({invalid json})" "\n";  // Invalid JSON
    
    std::string filepath = createTestFile("test_invalid.jsonl", content);
    
    DocumentLoader loader;
    EXPECT_THROW(loader.loadJSONL(filepath), std::runtime_error);
}

// Test loadJSONL with missing file
TEST_F(DocumentLoaderTest, LoadJSONL_MissingFile) {
    DocumentLoader loader;
    EXPECT_THROW(loader.loadJSONL("/nonexistent/file.jsonl"), std::runtime_error);
}

// Test loadCSV with simple data
TEST_F(DocumentLoaderTest, LoadCSV_SimpleData) {
    std::string content = 
        "title,author,content\n"
        "First Doc,John Doe,Content of first document\n"
        "Second Doc,Jane Smith,Content of second document\n"
        "Third Doc,Bob Johnson,Content of third document\n";
    
    std::string filepath = createTestFile("test.csv", content);
    
    DocumentLoader loader;
    std::vector<std::string> columns = {"title", "author", "content"};
    auto docs = loader.loadCSV(filepath, columns);
    
    EXPECT_EQ(docs.size(), 3);
    
    // Check first document
    EXPECT_EQ(docs[0].id, 1);
    EXPECT_EQ(docs[0].getField("title"), "First Doc");
    EXPECT_EQ(docs[0].getField("author"), "John Doe");
    EXPECT_EQ(docs[0].getField("content"), "Content of first document");
    EXPECT_GT(docs[0].term_count, 0);
    
    // Check second document
    EXPECT_EQ(docs[1].id, 2);
    EXPECT_EQ(docs[1].getField("title"), "Second Doc");
}

// Test loadCSV with quoted fields
TEST_F(DocumentLoaderTest, LoadCSV_QuotedFields) {
    std::string content = 
        "title,description\n"
        "\"Title with, comma\",\"Description with \"\"quotes\"\"\"\n"
        "\"Simple Title\",Normal description\n";
    
    std::string filepath = createTestFile("test_quotes.csv", content);
    
    DocumentLoader loader;
    std::vector<std::string> columns = {"title", "description"};
    auto docs = loader.loadCSV(filepath, columns);
    
    EXPECT_EQ(docs.size(), 2);
    EXPECT_EQ(docs[0].getField("title"), "Title with, comma");
    EXPECT_EQ(docs[0].getField("description"), "Description with \"quotes\"");
}

// Test loadCSV with empty lines
TEST_F(DocumentLoaderTest, LoadCSV_EmptyLines) {
    std::string content = 
        "title,content\n"
        "First,Content 1\n"
        "\n"  // Empty line
        "Second,Content 2\n";
    
    std::string filepath = createTestFile("test_empty.csv", content);
    
    DocumentLoader loader;
    std::vector<std::string> columns = {"title", "content"};
    auto docs = loader.loadCSV(filepath, columns);
    
    EXPECT_EQ(docs.size(), 2);  // Should skip empty lines
}

// Test loadCSV with column count mismatch
TEST_F(DocumentLoaderTest, LoadCSV_ColumnMismatch) {
    std::string content = 
        "title,content\n"
        "First,Content 1,Extra Column\n";  // Too many columns
    
    std::string filepath = createTestFile("test_mismatch.csv", content);
    
    DocumentLoader loader;
    std::vector<std::string> columns = {"title", "content"};
    EXPECT_THROW(loader.loadCSV(filepath, columns), std::runtime_error);
}

// Test loadCSV with empty column names
TEST_F(DocumentLoaderTest, LoadCSV_EmptyColumns) {
    std::string filepath = createTestFile("test.csv", "title,content\nTest,Content\n");
    
    DocumentLoader loader;
    std::vector<std::string> empty_columns;
    EXPECT_THROW(loader.loadCSV(filepath, empty_columns), std::runtime_error);
}

// Test loadCSV with missing file
TEST_F(DocumentLoaderTest, LoadCSV_MissingFile) {
    DocumentLoader loader;
    std::vector<std::string> columns = {"title", "content"};
    EXPECT_THROW(loader.loadCSV("/nonexistent/file.csv", columns), std::runtime_error);
}

// Test createDocument
TEST_F(DocumentLoaderTest, CreateDocument) {
    DocumentLoader loader;
    
    std::unordered_map<std::string, std::string> fields = {
        {"title", "Test Document"},
        {"content", "This is test content"}
    };
    
    auto doc = loader.createDocument(fields);
    
    EXPECT_EQ(doc.id, 1);
    EXPECT_EQ(doc.getField("title"), "Test Document");
    EXPECT_EQ(doc.getField("content"), "This is test content");
    EXPECT_GT(doc.term_count, 0);
    
    // Create another document - ID should increment
    auto doc2 = loader.createDocument(fields);
    EXPECT_EQ(doc2.id, 2);
}

// Test term_count calculation
TEST_F(DocumentLoaderTest, TermCountCalculation) {
    std::string content = 
        R"({"title": "Document", "content": "This has five word tokens"})" "\n";
    
    std::string filepath = createTestFile("test_terms.jsonl", content);
    
    DocumentLoader loader;
    auto docs = loader.loadJSONL(filepath);
    
    EXPECT_EQ(docs.size(), 1);
    // "Document" + "This has five word tokens" = 6 tokens
    EXPECT_EQ(docs[0].term_count, 6);
}

// Test getAllText integration
TEST_F(DocumentLoaderTest, GetAllText) {
    std::string content = 
        R"({"title": "Title", "author": "Author", "content": "Content"})" "\n";
    
    std::string filepath = createTestFile("test_alltext.jsonl", content);
    
    DocumentLoader loader;
    auto docs = loader.loadJSONL(filepath);
    
    EXPECT_EQ(docs.size(), 1);
    
    std::string all_text = docs[0].getAllText();
    EXPECT_TRUE(all_text.find("Title") != std::string::npos);
    EXPECT_TRUE(all_text.find("Author") != std::string::npos);
    EXPECT_TRUE(all_text.find("Content") != std::string::npos);
}
