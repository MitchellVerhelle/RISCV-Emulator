#pragma once
#include <emscripten.h>
#include <cstdio>

namespace demo {

inline void tick(void* /*userdata*/)
{
    static int frame = 0;
    if ((frame++ & 63) == 0)                  // print once per 64 frames
        std::puts("demo main‑loop is alive");
}

inline void install_main_loop()
{
    // 0 = use requestAnimationFrame, 1 = setTimeout; RAF is smoother.
    const int fps_hint  = 0;                  // ignored in RAF mode
    const bool simulate_infinite_loop = true; // keeps main() from returning

    emscripten_set_main_loop_arg(&tick,
                                 /*arg=*/nullptr,
                                 fps_hint,
                                 simulate_infinite_loop);

    // (optional) tune timing AFTER the loop exists
    emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
}

} // namespace demo