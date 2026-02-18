#pragma once

#include "search_types.hpp"
#include <atomic>
#include <chrono>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace rtrv_search_engine {

struct QueryCacheKey {
    std::string normalized_query;
    size_t options_hash = 0;

    bool operator==(const QueryCacheKey& other) const {
        return normalized_query == other.normalized_query && options_hash == other.options_hash;
    }
};

struct QueryCacheKeyHasher {
    size_t operator()(const QueryCacheKey& key) const {
        size_t seed = std::hash<std::string>{}(key.normalized_query);
        seed ^= key.options_hash + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        return seed;
    }
};

class QueryCache {
public:
    QueryCache(size_t max_entries = 1024,
               std::chrono::milliseconds ttl = std::chrono::seconds(60));

    bool get(const QueryCacheKey& key, std::vector<SearchResult>* out_results);
    void put(const QueryCacheKey& key, const std::vector<SearchResult>& results);

    void clear();
    void setMaxEntries(size_t max_entries);
    void setTtl(std::chrono::milliseconds ttl);

    CacheStatistics getStats() const;

private:
    struct Entry {
        std::vector<SearchResult> results;
        std::chrono::steady_clock::time_point timestamp;
        std::list<QueryCacheKey>::iterator lru_it;
    };

    bool isExpired(const Entry& entry, std::chrono::steady_clock::time_point now) const;
    void touch(std::unordered_map<QueryCacheKey, Entry, QueryCacheKeyHasher>::iterator it);
    void evictIfNeeded();
    void eraseEntry(std::unordered_map<QueryCacheKey, Entry, QueryCacheKeyHasher>::iterator it,
                    bool count_eviction);

    mutable std::shared_mutex mutex_;
    std::unordered_map<QueryCacheKey, Entry, QueryCacheKeyHasher> entries_;
    std::list<QueryCacheKey> lru_order_;
    size_t max_entries_;
    std::chrono::milliseconds ttl_;

    std::atomic<size_t> hit_count_;
    std::atomic<size_t> miss_count_;
    std::atomic<size_t> eviction_count_;
};

} // namespace rtrv_search_engine
