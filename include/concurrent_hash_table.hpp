#pragma once
#include "lock_free_list.hpp"
#include "memory_bus.hpp"
#include <shared_mutex>
#include <exception>
#include <vector>
#include <optional>
#ifndef __EMSCRIPTEN__
#  include <oneapi/dpl/execution>
#  include <oneapi/dpl/algorithm>
#endif

namespace rv {

template <typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
class ConcurrentHashTable : public MemoryBus
{
    using Bucket = LockFreeList<K,V>;

  public:
    explicit ConcurrentHashTable(std::size_t cap = 64)
        : buckets_(cap) {}

    /*
    MemoryBus facade
    */
    std::optional<std::uint32_t> load_word(std::uint32_t addr) override
    {
        if (auto v = get(static_cast<K>(addr))) return *v;
        return std::nullopt;
    }
    bool store_word(std::uint32_t addr, std::uint32_t val) override
    {
        put(static_cast<K>(addr), static_cast<V>(val)); return true;
    }

    /*
    map-like interface
    */
    [[nodiscard]]
    std::optional<V> get(const K& key) const
    {
        std::shared_lock lk(table_mtx_);
        return buckets_[bucket_index(key)].find(key);
    }

    bool put(const K& key, const V& val)
    {
        {   std::shared_lock lk(table_mtx_);
            auto& b = buckets_[bucket_index(key)];
            if (!b.put(key,val)) ++size_;
        }
        maybe_rehash();
        return true;
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_.load(); }

  private:
    mutable std::shared_mutex table_mtx_; // guards resize
    std::vector<Bucket>       buckets_;
    std::atomic<std::size_t>  size_{0};
    Hash                      hasher_;

    static constexpr float max_load = 0.75f;

    std::size_t bucket_index(const K& k) const noexcept
    { return hasher_(k) & (buckets_.size() - 1); }

    void maybe_rehash()
    {
        if (static_cast<float>(size_.load()) / static_cast<float>(buckets_.size()) < max_load)
            return;

        std::unique_lock lk(table_mtx_); // exclusive
        std::size_t new_cap = buckets_.size() * 2;
        std::vector<Bucket> new_buckets(new_cap);

    #ifdef __EMSCRIPTEN__
        // Sequential rehash on WebAssembly
        for (auto const& old_b : buckets_) {
            old_b.for_each([&](const K& k, const V& v){
                auto idx = hasher_(k) & (new_cap - 1);
                new_buckets[idx].put(k, v);
            });
        }
    #else
        oneapi::dpl::for_each(
            oneapi::dpl::execution::par, // using par because each sub-loop does heap-allocs and atomics, so vectorization isn't beneficial.
            buckets_.begin(), buckets_.end(),
            [&](Bucket const& old_b) {
                old_b.for_each([&](const K& k, const V& v){
                    new_buckets[hasher_(k) & (new_cap - 1)].put(k, v);
                });
            });
    #endif
        
        buckets_.swap(new_buckets);
    }
};

} // namespace rv
