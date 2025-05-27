#include "mmio_window.hpp"
#include "cache.hpp"
#include "hash_table.hpp"
#include "riscv.hpp"
#include "text/bitmap_font.hpp"

#include <SDL.h>
#include <algorithm>    // for std::fill_n
#include <cstdlib>      // for std::exit
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif

using namespace rv;

// —————————————————————————————————————————————————————————————
//  Globals shared between host & emulator
// —————————————————————————————————————————————————————————————
static SDL_Window*   win = nullptr;
static SDL_Renderer* ren = nullptr;
static SDL_Texture*  tex = nullptr;

// These get filled by build_system()
static MmioWindow* io  = nullptr;
static RiscV*      cpu = nullptr;

// menu state
static bool started = false;

namespace rv {
    void build_system(MmioWindow*& io_out, RiscV*& cpu_out);
}

// forward to your emulator’s per-frame loop (in the same file below)
static void main_loop();

// —————————————————————————————————————————————————————————————
//  Draw the start-screen menu into the 128×128 frame-buffer
// —————————————————————————————————————————————————————————————
static void draw_start_menu()
{
    // 1) background = dark blue (RGB332 = 0x10)
    std::fill_n(io->framebuffer, 128*128, std::uint8_t(0x10));

    // 2) white button rectangle at (28,56) size 72×16
    for (int y = 56; y < 72; ++y)
        for (int x = 28; x < 100; ++x)
            io->framebuffer[y*128 + x] = 0xFF;

    // 3) label "START" (black) centered in the button
    text::BitmapFont::drawText(
        io->framebuffer, 128, 128,
        48, 60,           // x,y in logical 128×128 space
        "START", 0x00     // color = black
    );
}

// —————————————————————————————————————————————————————————————
//  Pre-game menu loop: waits for mouse click on the START button
// —————————————————————————————————————————————————————————————
static void menu_loop()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#else
            std::exit(0);
#endif
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN) {
            // map 768×768 window → 128×128 logical coords
            int mx = e.button.x * 128 / 768;
            int my = e.button.y * 128 / 768;
            // if inside our button…
            if (mx >= 28 && mx < 100 && my >= 56 && my < 72) {
                started = true;
#ifdef __EMSCRIPTEN__
                emscripten_cancel_main_loop();
#endif
                // build and point to the emulator core
                build_system(io, cpu);
#ifdef __EMSCRIPTEN__
                // switch into the emulator’s main_loop for browser
                emscripten_set_main_loop(main_loop, 0, 1);
#endif
                return;
            }
        }
    }

    // draw/redraw menu each frame
    draw_start_menu();
    SDL_UpdateTexture(tex, nullptr, io->framebuffer, 128);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    SDL_RenderPresent(ren);
}

// —————————————————————————————————————————————————————————————
//  The real emulator loop: identical to before
// —————————————————————————————————————————————————————————————
static void main_loop()
{
    SDL_Event e;
    io->gpio_in = 0;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#else
            std::exit(0);
#endif
        }
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            bool down = (e.type == SDL_KEYDOWN);
            auto bit = [&](int b){ return down ? (1u<<b) : 0u; };
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:  io->gpio_in |= bit(0); break;
                case SDLK_RIGHT: io->gpio_in |= bit(1); break;
                case SDLK_UP:    io->gpio_in |= bit(2); break;
                case SDLK_DOWN:  io->gpio_in |= bit(3); break;
                case SDLK_SPACE: io->gpio_in |= bit(4); break;
            }
        }
    }

    // run the guest CPU a bunch of instructions
    for (int i = 0; i < 10'000; ++i)
        cpu->step();

    // blit frame-buffer to screen
    SDL_UpdateTexture(tex, nullptr, io->framebuffer, 128);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    SDL_RenderPresent(ren);
}

// —————————————————————————————————————————————————————————————
//  Entry point
// —————————————————————————————————————————————————————————————
int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    win = SDL_CreateWindow("RV-Game",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           768, 768,
                           SDL_WINDOW_RESIZABLE);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    // SDL_RenderSetLogicalSize(ren, 128, 128);
    tex = SDL_CreateTexture(ren,
                            SDL_PIXELFORMAT_RGB332,
                            SDL_TEXTUREACCESS_STREAMING,
                            128, 128);

#ifdef __EMSCRIPTEN__
    // run the menu until START is clicked
    emscripten_set_main_loop(menu_loop, 0, 1);
#else
    // native: spin the menu loop
    while (!started) {
        menu_loop();
    }
    // then drop into the emulator
    while (true) {
        main_loop();
        SDL_Delay(16);
    }
#endif

    return 0;
}
