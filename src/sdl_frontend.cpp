#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <array>
#include "text/bitmap_font.hpp"

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;

    SDL_Window*   win = SDL_CreateWindow("HELLO SDL",
                                         SDL_WINDOWPOS_CENTERED,
                                         SDL_WINDOWPOS_CENTERED,
                                         512, 512, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, 0);
    SDL_RenderSetLogicalSize(ren, 128, 128);

    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB332,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         128, 128);

    std::array<std::uint8_t,128*128> fb{};
    fb.fill(0x00);   // black

    text::BitmapFont::drawText(fb.data(), 128, 128,
        (128 - 10*text::BitmapFont::charWidth)/2,
        (128 - text::BitmapFont::charHeight)/2,
        "HELLO SDL!", 0xFF);      // white

    SDL_UpdateTexture(tex, nullptr, fb.data(), 128);
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    SDL_RenderPresent(ren);

    SDL_Event e;
    while (SDL_WaitEvent(&e) && e.type != SDL_QUIT) {}

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
