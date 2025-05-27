#pragma once
#include <cstdint>
#include <array>
#include <string_view>

namespace text {

/** A fixed‐size 8×8 bitmap glyph. */
struct Glyph {
    std::uint8_t width;           // always 8
    std::uint8_t height;          // always 8
    std::array<std::uint8_t,8>    // one byte per row of pixels
        bitmap;
};

/** Simple 8×8 monospaced bitmap font. */
class BitmapFont {
public:
    static constexpr int charWidth  = 8;
    static constexpr int charHeight = 8;

    /** Return the glyph for `c`, or a blank glyph if unknown. */
    static constexpr const Glyph& getGlyph(char c) noexcept;

    /** 
     * Draw `text` at (x,y) into `fb`, an 8-bit per-pixel buffer of size
     * `fbWidth×fbHeight`. 
     * Pixels are set to `color` when the glyph bit is 1.
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
