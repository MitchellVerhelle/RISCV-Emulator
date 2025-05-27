#pragma once
#include "memory_bus.hpp"
#include "cache_line.hpp"
#include "cache_stats.hpp"
#include <vector>
#include <atomic>
#include <ranges>
#include <optional>
#include <cassert>

namespace rv {

/*
Simple N-way set-associative cache.
*/
class Cache : public MemoryBus
{
  public:
    using Address = std::uint32_t;

    enum class WritePolicy { write_back, write_through };

    Cache(std::size_t sets,
      std::size_t ways,
      std::unique_ptr<MemoryBus> next,
      WritePolicy wp = WritePolicy::write_back)
    : sets_{sets},
      ways_{ways},
      policy_{wp},
      data_(sets * ways),
      lru_way_(sets, 0),
      lru_lock_{ std::make_unique<std::atomic_flag[]>(sets) },
      next_{std::move(next)}
{
    for (std::size_t i = 0; i < sets_; ++i) {
        lru_lock_[i].clear();
    }
}

    /*
    MemoryBus
    */
    std::optional<std::uint32_t> load_word(Address addr) override;
    bool store_word(Address addr, std::uint32_t v) override;

    [[nodiscard]] CacheStats const& stats() const noexcept { return stats_; }

  private:
    const std::size_t sets_;
    const std::size_t ways_;
    const WritePolicy policy_;

    std::vector<CacheLine> data_; // flat [set*ways + way]
    std::vector<std::size_t> lru_way_; // per-set MRU tracker
    std::unique_ptr<std::atomic_flag[]> lru_lock_;

    std::unique_ptr<MemoryBus> next_;
    CacheStats stats_;

    /*
    helpers
    */
    static constexpr std::size_t line_words = 4;
    static constexpr std::size_t line_shift = 4; // 16-byte line

    [[nodiscard]] static std::uint32_t tag(Address a) noexcept
    {
        return a >> (line_shift+6);
    }

    [[nodiscard]] std::size_t index(Address a) const noexcept
    {
        return (a >> line_shift) & (sets_ - 1);
    }

    [[nodiscard]] std::size_t slot(std::size_t set, std::size_t way) const noexcept
    {
        return set * ways_ + way;
    }

    void touch_lru(std::size_t set, std::size_t way) noexcept;
    std::size_t select_victim(std::size_t set) noexcept;
    void fill_line(std::size_t set, std::size_t way, Address addr);
};

/*
Implementation
*/
inline void Cache::touch_lru(std::size_t set, std::size_t way) noexcept
{
    while (lru_lock_[set].test_and_set(std::memory_order_acquire)) ; // spin
    lru_way_[set] = way;
    lru_lock_[set].clear(std::memory_order_release);
}

inline std::size_t Cache::select_victim(std::size_t set) noexcept
{
    // ranges pipeline: 0..ways-1 | filter(!mru) | take(1)
    auto ways = std::views::iota(std::size_t{0}, ways_);
    auto cand = ways | std::views::filter([&](std::size_t w){return w!=lru_way_[set];});
    return cand.empty() ? 0 : *cand.begin();
}

inline std::optional<std::uint32_t> Cache::load_word(Address addr)
{
    ++stats_.n_cpu_accesses;
    std::size_t set = index(addr);
    std::uint32_t tg = tag(addr);

    auto ways = std::views::iota(std::size_t{0}, ways_);
    if (auto hit = std::ranges::find_if(ways, [&](std::size_t w){
            const CacheLine& cl = data_[slot(set,w)];
            return cl.valid && cl.tag == tg; });
        hit != ways.end())
    {
        ++stats_.n_hits;
        touch_lru(set, *hit);
        const auto& line = data_[slot(set,*hit)];
        return line.words[(addr >> 2) & (line_words-1)];
    }

    // miss
    ++stats_.n_misses;
    std::size_t victim = select_victim(set);
    fill_line(set, victim, addr);
    const auto& line = data_[slot(set,victim)];
    return line.words[(addr >> 2) & (line_words-1)];
}

inline bool Cache::store_word(Address addr, std::uint32_t v)
{
    ++stats_.n_cpu_accesses;
    std::size_t set = index(addr);
    std::uint32_t tg = tag(addr);
    auto ways = std::views::iota(std::size_t{0}, ways_);

    if (auto hit = std::ranges::find_if(ways, [&](std::size_t w){
            CacheLine& cl = data_[slot(set,w)];
            if (cl.valid && cl.tag == tg) { cl.words[(addr>>2)&3]=v; cl.dirty=true; return true; }
            return false; });
        hit != ways.end())
    {
        ++stats_.n_hits;
        touch_lru(set,*hit);
        if (policy_ == WritePolicy::write_through)
            return next_->store_word(addr, v);
        return true;
    }

    // write-miss => write-allocate, then retry as hit
    ++stats_.n_misses;
    std::size_t victim = select_victim(set);
    fill_line(set, victim, addr);
    return store_word(addr, v);
}

inline void Cache::fill_line(std::size_t set, std::size_t way, Address addr)
{
    std::size_t sl = slot(set, way);
    CacheLine& cl = data_[sl];

    // write-back dirty victim
    if (cl.valid && cl.dirty)
        for (std::size_t i=0;i<line_words;++i)
            next_->store_word(static_cast<std::uint32_t>(((cl.tag<<6)|set)<<line_shift | (i<<2)), cl.words[i]);

    stats_.n_evictions += cl.valid;
    Address base = addr & ~( (1u<<line_shift)-1 );
    for (std::size_t i=0;i<line_words;++i)
        cl.words[i] = next_->load_word(static_cast<std::uint32_t>(base | (i<<2))).value_or(0);

    cl.tag = tag(addr);
    cl.valid = true;
    cl.dirty = false;
    touch_lru(set, way);
}

} // namespace rv
