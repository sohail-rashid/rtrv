#include "query_cache.hpp"

namespace search_engine {

QueryCache::QueryCache(size_t max_entries, std::chrono::milliseconds ttl)
    : max_entries_(max_entries),
      ttl_(ttl),
      hit_count_(0),
      miss_count_(0),
      eviction_count_(0) {}

bool QueryCache::get(const QueryCacheKey& key, std::vector<SearchResult>* out_results) {
    const auto now = std::chrono::steady_clock::now();

    {
        std::shared_lock read_lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            miss_count_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        if (isExpired(it->second, now)) {
            // Upgrade to write lock to remove expired entry.
        } else {
            // Upgrade to write lock to update LRU order.
        }
    }

    std::unique_lock write_lock(mutex_);
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        miss_count_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    if (isExpired(it->second, now)) {
        eraseEntry(it, true);
        miss_count_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    touch(it);
    hit_count_.fetch_add(1, std::memory_order_relaxed);

    if (out_results) {
        *out_results = it->second.results;
    }

    return true;
}

void QueryCache::put(const QueryCacheKey& key, const std::vector<SearchResult>& results) {
    const auto now = std::chrono::steady_clock::now();
    std::unique_lock write_lock(mutex_);

    auto it = entries_.find(key);
    if (it != entries_.end()) {
        it->second.results = results;
        it->second.timestamp = now;
        touch(it);
        return;
    }

    lru_order_.push_front(key);
    Entry entry{results, now, lru_order_.begin()};
    entries_.emplace(key, std::move(entry));

    evictIfNeeded();
}

void QueryCache::clear() {
    std::unique_lock write_lock(mutex_);
    entries_.clear();
    lru_order_.clear();
}

void QueryCache::setMaxEntries(size_t max_entries) {
    std::unique_lock write_lock(mutex_);
    max_entries_ = max_entries;
    evictIfNeeded();
}

void QueryCache::setTtl(std::chrono::milliseconds ttl) {
    std::unique_lock write_lock(mutex_);
    ttl_ = ttl;
}

CacheStatistics QueryCache::getStats() const {
    std::shared_lock read_lock(mutex_);

    CacheStatistics stats;
    stats.hit_count = hit_count_.load(std::memory_order_relaxed);
    stats.miss_count = miss_count_.load(std::memory_order_relaxed);
    stats.eviction_count = eviction_count_.load(std::memory_order_relaxed);
    stats.current_size = entries_.size();
    stats.max_size = max_entries_;

    const size_t total = stats.hit_count + stats.miss_count;
    stats.hit_rate = total > 0 ? static_cast<double>(stats.hit_count) / total : 0.0;

    return stats;
}

bool QueryCache::isExpired(const Entry& entry, std::chrono::steady_clock::time_point now) const {
    if (ttl_.count() <= 0) {
        return false;
    }
    return now - entry.timestamp > ttl_;
}

void QueryCache::touch(std::unordered_map<QueryCacheKey, Entry, QueryCacheKeyHasher>::iterator it) {
    lru_order_.erase(it->second.lru_it);
    lru_order_.push_front(it->first);
    it->second.lru_it = lru_order_.begin();
}

void QueryCache::evictIfNeeded() {
    while (entries_.size() > max_entries_ && !lru_order_.empty()) {
        const auto key = lru_order_.back();
        auto it = entries_.find(key);
        if (it != entries_.end()) {
            eraseEntry(it, true);
        } else {
            lru_order_.pop_back();
        }
    }
}

void QueryCache::eraseEntry(
    std::unordered_map<QueryCacheKey, Entry, QueryCacheKeyHasher>::iterator it,
    bool count_eviction) {
    lru_order_.erase(it->second.lru_it);
    entries_.erase(it);
    if (count_eviction) {
        eviction_count_.fetch_add(1, std::memory_order_relaxed);
    }
}

} // namespace search_engine
