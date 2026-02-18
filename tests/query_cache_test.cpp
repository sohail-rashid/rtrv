#include <gtest/gtest.h>
#include "query_cache.hpp"

#include <chrono>
#include <thread>

using namespace search_engine;

static std::vector<SearchResult> makeResults(uint32_t doc_id, const std::string& content) {
    SearchResult result;
    result.document = Document{doc_id, {{"content", content}}};
    result.score = 1.0;
    return {result};
}

TEST(QueryCacheTest, HitAndMiss) {
    QueryCache cache(4, std::chrono::seconds(60));

    QueryCacheKey key{"machine learning", 42};
    std::vector<SearchResult> out;

    EXPECT_FALSE(cache.get(key, &out));
    cache.put(key, makeResults(1, "machine learning basics"));
    EXPECT_TRUE(cache.get(key, &out));
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].document.id, 1u);

    auto stats = cache.getStats();
    EXPECT_EQ(stats.hit_count, 1u);
    EXPECT_EQ(stats.miss_count, 1u);
    EXPECT_EQ(stats.current_size, 1u);
}

TEST(QueryCacheTest, EvictsLeastRecentlyUsed) {
    QueryCache cache(2, std::chrono::seconds(60));

    QueryCacheKey key1{"q1", 1};
    QueryCacheKey key2{"q2", 2};
    QueryCacheKey key3{"q3", 3};

    cache.put(key1, makeResults(1, "a"));
    cache.put(key2, makeResults(2, "b"));

    std::vector<SearchResult> out;
    EXPECT_TRUE(cache.get(key1, &out));

    cache.put(key3, makeResults(3, "c"));

    EXPECT_TRUE(cache.get(key1, &out));
    EXPECT_FALSE(cache.get(key2, &out));
    EXPECT_TRUE(cache.get(key3, &out));

    auto stats = cache.getStats();
    EXPECT_EQ(stats.current_size, 2u);
    EXPECT_GE(stats.eviction_count, 1u);
}

TEST(QueryCacheTest, TtlExpiry) {
    QueryCache cache(4, std::chrono::seconds(0));

    QueryCacheKey key{"expire", 99};
    cache.put(key, makeResults(5, "soon expired"));

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    std::vector<SearchResult> out;
    EXPECT_TRUE(cache.get(key, &out));

    cache.setTtl(std::chrono::milliseconds(10));
    cache.put(key, makeResults(5, "soon expired"));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_FALSE(cache.get(key, &out));
    auto stats = cache.getStats();
    EXPECT_GE(stats.eviction_count, 1u);
}

TEST(QueryCacheTest, ClearResetsSize) {
    QueryCache cache(4, std::chrono::seconds(60));

    cache.put({"alpha", 1}, makeResults(1, "alpha"));
    cache.put({"beta", 2}, makeResults(2, "beta"));
    cache.clear();

    auto stats = cache.getStats();
    EXPECT_EQ(stats.current_size, 0u);
}

TEST(QueryCacheTest, ThreadSafety) {
    QueryCache cache(64, std::chrono::seconds(60));

    auto worker = [&cache](int base) {
        for (int i = 0; i < 100; ++i) {
            QueryCacheKey key{"q" + std::to_string(base + i), static_cast<size_t>(base + i)};
            cache.put(key, makeResults(static_cast<uint32_t>(base + i), "content"));
            std::vector<SearchResult> out;
            cache.get(key, &out);
        }
    };

    std::thread t1(worker, 0);
    std::thread t2(worker, 1000);
    std::thread t3(worker, 2000);
    std::thread t4(worker, 3000);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    auto stats = cache.getStats();
    EXPECT_LE(stats.current_size, 64u);
}
