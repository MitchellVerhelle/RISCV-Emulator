#include "graphics.hpp"

namespace gfx {

/* 8×8 fixed font: only space + A‑Z (define the rest as needed) */
static constexpr std::uint8_t font[27][8] = {
    {0,0,0,0,0,0,0,0},                       // space
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0}, // A
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0}, // B
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0}, // C
    /* … add more letters … */
};

static void put_char(std::uint8_t* fb, char c, int px, int py)
{
    int idx = (c==' ')?0 : (c-'A'+1);
    for (int y=0; y<8; ++y) {
        std::uint8_t row = font[idx][y];
        for (int x=0; x<8; ++x)
            if (row & (1<<(7-x)))
                fb[(py+y)*128 + (px+x)] = 0xFF;      // white
    }
}

void draw_title_screen(std::uint8_t* fb)
{
    /* blue gradient background */
    for (int y=0; y<128; ++y)
        for (int x=0; x<128; ++x)
            fb[y*128+x] = static_cast<std::uint8_t>(32 + y);  // RGB332

    const char* txt = "RISC‑V  GAME";
    for (int i=0; txt[i]; ++i)
        put_char(fb, txt[i], 32 + i*9, 50);
}

} // namespace gfx
