#pragma once
#include <cstdint>

namespace gfx {

/* 128 × 128, RGB332. Safe to call once at startup. */
void draw_title_screen(std::uint8_t* framebuffer);

} // namespace gfx
