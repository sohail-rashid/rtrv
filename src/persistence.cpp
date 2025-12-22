#include "persistence.hpp"
#include "search_engine.hpp"
#include <fstream>

namespace search_engine {

bool Persistence::save(const SearchEngine& engine, const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Write header
    SnapshotHeader header;
    header.num_documents = engine.documents_.size();
    header.num_terms = engine.index_->getTermCount();
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Write next_doc_id
    file.write(reinterpret_cast<const char*>(&engine.next_doc_id_), sizeof(engine.next_doc_id_));
    
    // Write documents
    for (const auto& [doc_id, doc] : engine.documents_) {
        // Write doc_id
        file.write(reinterpret_cast<const char*>(&doc_id), sizeof(doc_id));
        
        // Write term_count
        file.write(reinterpret_cast<const char*>(&doc.term_count), sizeof(doc.term_count));
        
        // Write fields size and entries
        size_t fields_size = doc.fields.size();
        file.write(reinterpret_cast<const char*>(&fields_size), sizeof(fields_size));
        for (const auto& [key, value] : doc.fields) {
            size_t key_len = key.size();
            file.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
            file.write(key.data(), key_len);
            
            size_t val_len = value.size();
            file.write(reinterpret_cast<const char*>(&val_len), sizeof(val_len));
            file.write(value.data(), val_len);
        }
    }
    
    // Write inverted index
    // Get all terms and their postings
    const auto& index_map = engine.index_->index_;
    size_t num_index_terms = index_map.size();
    file.write(reinterpret_cast<const char*>(&num_index_terms), sizeof(num_index_terms));
    
    for (const auto& [term, postings] : index_map) {
        // Write term
        size_t term_len = term.size();
        file.write(reinterpret_cast<const char*>(&term_len), sizeof(term_len));
        file.write(term.data(), term_len);
        
        // Write postings count
        size_t postings_count = postings.size();
        file.write(reinterpret_cast<const char*>(&postings_count), sizeof(postings_count));
        
        // Write each posting
        for (const auto& posting : postings) {
            file.write(reinterpret_cast<const char*>(&posting.doc_id), sizeof(posting.doc_id));
            file.write(reinterpret_cast<const char*>(&posting.term_frequency), sizeof(posting.term_frequency));
            
            // Write positions
            size_t pos_count = posting.positions.size();
            file.write(reinterpret_cast<const char*>(&pos_count), sizeof(pos_count));
            file.write(reinterpret_cast<const char*>(posting.positions.data()), 
                      pos_count * sizeof(uint32_t));
        }
    }
    
    return file.good();
}

bool Persistence::load(SearchEngine& engine, const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Read and validate header
    SnapshotHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.magic != 0x53454152 || header.version != 1) {
        return false;  // Invalid file format
    }
    
    // Clear existing state
    engine.documents_.clear();
    engine.index_->clear();
    
    // Read next_doc_id
    file.read(reinterpret_cast<char*>(&engine.next_doc_id_), sizeof(engine.next_doc_id_));
    
    // Read documents
    for (size_t i = 0; i < header.num_documents; ++i) {
        uint64_t doc_id;
        file.read(reinterpret_cast<char*>(&doc_id), sizeof(doc_id));
        
        // Read term_count
        size_t term_count;
        file.read(reinterpret_cast<char*>(&term_count), sizeof(term_count));
        
        // Read fields
        size_t fields_size;
        file.read(reinterpret_cast<char*>(&fields_size), sizeof(fields_size));
        std::unordered_map<std::string, std::string> fields;
        for (size_t j = 0; j < fields_size; ++j) {
            size_t key_len;
            file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
            std::string key(key_len, '\0');
            file.read(&key[0], key_len);
            
            size_t val_len;
            file.read(reinterpret_cast<char*>(&val_len), sizeof(val_len));
            std::string value(val_len, '\0');
            file.read(&value[0], val_len);
            
            fields[key] = value;
        }
        
        // Create and store document
        Document doc{static_cast<uint32_t>(doc_id), fields};
        doc.term_count = term_count;
        engine.documents_[doc_id] = doc;
    }
    
    // Read inverted index
    size_t num_index_terms;
    file.read(reinterpret_cast<char*>(&num_index_terms), sizeof(num_index_terms));
    
    for (size_t i = 0; i < num_index_terms; ++i) {
        // Read term
        size_t term_len;
        file.read(reinterpret_cast<char*>(&term_len), sizeof(term_len));
        std::string term(term_len, '\0');
        file.read(&term[0], term_len);
        
        // Read postings count
        size_t postings_count;
        file.read(reinterpret_cast<char*>(&postings_count), sizeof(postings_count));
        
        // Read each posting
        for (size_t j = 0; j < postings_count; ++j) {
            uint64_t doc_id;
            uint32_t term_frequency;
            file.read(reinterpret_cast<char*>(&doc_id), sizeof(doc_id));
            file.read(reinterpret_cast<char*>(&term_frequency), sizeof(term_frequency));
            
            // Read positions
            size_t pos_count;
            file.read(reinterpret_cast<char*>(&pos_count), sizeof(pos_count));
            std::vector<uint32_t> positions(pos_count);
            file.read(reinterpret_cast<char*>(positions.data()), 
                     pos_count * sizeof(uint32_t));
            
            // Add to index (reconstruct posting list)
            for (uint32_t pos : positions) {
                engine.index_->addTerm(term, doc_id, pos);
            }
        }
    }
    
    return file.good();
}

} 
