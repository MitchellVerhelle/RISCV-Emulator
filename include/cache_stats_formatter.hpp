#pragma once
#include <format>
#include <string_view>
#include "cache_stats.hpp"

/*
Add a formatter *into namespace std*.
Supports:
    "{}"      ->  single-line (same as CacheStats::pretty())
    "{:full}" ->  multi-line, nicely indented block
Any other specifier triggers a std::format_error.
*/
template <>
struct std::formatter<rv::CacheStats, char>
{
    // store which style the user asked for
    enum class style { single, full } sty{style::single};

    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            if (*it == ':' ) ++it; // ignore :
            const std::string_view tag{ it, ctx.end() };
            if (tag.starts_with("full")) { sty = style::full; it += 4; }
            else throw std::format_error("unknown format for CacheStats");
        }
        return it; // points at '}'
    }

    template <class FmtCtx>
    auto format(const rv::CacheStats& cs, FmtCtx& ctx) const
    {
        using std::format;
        switch (sty) {
        case style::single:
            return format_to(ctx.out(), "Hits {:8}, Misses {:8}  "
                                         "HR {:5.2f}%  MR {:5.2f}%",
                              cs.n_hits.load(), cs.n_misses.load(),
                              cs.hit_rate()*100.0, cs.miss_rate()*100.0);

        case style::full:
            return format_to(ctx.out(),
R"(Cache statistics
    CPU accesses : {0:10}
    Hits         : {1:10}
    Misses       : {2:10}
    Evictions    : {3:10}
    Hit rate     : {4:5.2f} %
    Miss rate    : {5:5.2f} %
)",
                cs.n_cpu_accesses.load(), cs.n_hits.load(),
                cs.n_misses.load(), cs.n_evictions.load(),
                cs.hit_rate()*100.0, cs.miss_rate()*100.0);
        }
        // unreachable, but silences -Wreturn-type
        return format_to(ctx.out(), "");
    }
};
