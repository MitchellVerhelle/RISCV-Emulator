#include "mmio_window.hpp"

namespace rv {

std::optional<std::uint32_t>
MmioWindow::load_word(std::uint32_t a)
{
    if (a == 0x2000'2000) return gpio_in;
    return next_->load_word(a);                  // delegate
}

bool MmioWindow::store_word(std::uint32_t a, std::uint32_t v)
{
    if (a >= 0x2000'0000 && a < 0x2000'2000) {   // frame‑buffer
        framebuffer[a - 0x2000'0000] = static_cast<std::uint8_t>(v);
        return true;
    }
    if (a == 0x2000'2004) {                      // audio
        audio_note = static_cast<std::uint8_t>(v);
        return true;
    }
    return next_->store_word(a, v);              // delegate
}

} // namespace rv
