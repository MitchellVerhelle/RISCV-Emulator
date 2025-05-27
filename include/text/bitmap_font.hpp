#pragma once
#include <cstdint>
#include <array>
#include <string_view>

namespace text {

/*
A fixed‐size 8-by-8 bitmap glyph.
*/
struct Glyph {
    std::uint8_t width;  // 8
    std::uint8_t height; // 8
    std::array<std::uint8_t,8> bitmap; // one byte per row of pixels
};

/*
Simple 8-by-8 monospaced bitmap font.
*/
class BitmapFont {
public:
    static constexpr int charWidth  = 8;
    static constexpr int charHeight = 8;

    /*
    Return the glyph for `c`, or a blank glyph if unknown.
    */
    static const Glyph& getGlyph(char c) noexcept;

    /*
    Draw `text` at (x,y) into `fb`, an 8-bit per-pixel buffer of size `fbWidth-by-fbHeight`. 
    Pixels are set to `color` when the glyph bit is 1.
    */
    static void drawText(std::uint8_t*    fb,
                         std::size_t      fbWidth,
                         std::size_t      fbHeight,
                         int              x,
                         int              y,
                         std::string_view text,
                         std::uint8_t     color = 0xFF) noexcept;
};

} // namespace text
