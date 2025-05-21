#pragma once
#include "linked_list.hpp"
#include <vector>
#include <optional>
#include <functional>
#include <ranges>

namespace rv {

/** Open‑addressing hash table (linear probing). */
template <
    typename K, typename V,
    typename Hash = std::hash<K>,
    typename KeyEq = std::equal_to<K>>
class HashTable : public MemoryBus
{
    enum class BucketState : std::uint8_t { empty, full, tomb };
    struct Bucket {
        K            key;
        V            val;
        BucketState  st{BucketState::empty};
    };

  public:
    explicit HashTable(std::size_t cap = 64)
        : buckets_(cap) {}

    /* MemoryBus interface – drive by key = address.                    */
    std::optional<std::uint32_t> load_word(std::uint32_t addr) override
    {
        if (auto opt = get(static_cast<K>(addr))) return *opt;
        return std::nullopt;
    }
    bool store_word(std::uint32_t addr, std::uint32_t val) override
    {
        put(static_cast<K>(addr), static_cast<V>(val)); return true;
    }

    /* Standard map‑like interface                                      */
    [[nodiscard]] std::optional<V> get(const K& key) const
    {
        auto idx = probe(key);
        if (buckets_[idx].st == BucketState::full) return buckets_[idx].val;
        return std::nullopt;
    }

    bool put(const K& key, const V& val)
    {
        maybe_rehash();
        auto idx = probe(key);
        bool replaced = buckets_[idx].st == BucketState::full;
        buckets_[idx] = Bucket{key, val, BucketState::full};
        if (!replaced) ++size_;
        return replaced;
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }

  private:
    std::vector<Bucket> buckets_;
    std::size_t         size_  = 0;
    Hash                hasher_;
    KeyEq               eq_;

    static constexpr float max_load = 0.7f;

    void maybe_rehash()
    {
        if (static_cast<float>(size_) / buckets_.size() < max_load) return;

        std::vector<Bucket> old = std::move(buckets_);
        buckets_.assign(old.size()*2, {});
        size_ = 0;

        for (auto& b : old)
            if (b.st == BucketState::full) put(std::move(b.key), std::move(b.val));
    }

    std::size_t probe(const K& key) const
    {
        std::size_t mask = buckets_.size() - 1;
        std::size_t i    = hasher_(key) & mask;

        while (buckets_[i].st != BucketState::empty &&
               (buckets_[i].st == BucketState::tomb || !eq_(buckets_[i].key, key)))
            i = (i + 1) & mask;
        return i;
    }
};

} // namespace rv
