#include "mmio_window.hpp"
#include "cache.hpp"
#include "hash_table.hpp"
#include "riscv.hpp"
#include "demo_loop.hpp"

#include <SDL.h>
#include <emscripten.h>

using namespace rv;

extern "C" void start_game();

extern "C" void start_game()                // <── exported to JS
{
    demo::install_main_loop();              // run the demo loop
}

/* ------------------------------------------------------------------ */
/*  Global handles (frontend only)                                    */
/* ------------------------------------------------------------------ */
static MmioWindow* io      = nullptr;   // provided by emulator.cpp
static RiscV*      cpu     = nullptr;

namespace rv {
    void build_system(MmioWindow*& io_out,
                      RiscV*&      cpu_out);          // from emulator.cpp
}

/* ------------------------------------------------------------------ */
/*  SDL + Emscripten glue                                             */
/* ------------------------------------------------------------------ */
static SDL_Window*   win  = nullptr;
static SDL_Renderer* ren  = nullptr;
static SDL_Texture*  tex  = nullptr;

void main_loop()
{
    /* 1. Input ---------------------------------------------------- */
    SDL_Event e;
    io->gpio_in = 0;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) emscripten_cancel_main_loop();
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            bool down = e.type == SDL_KEYDOWN;
            auto bit  = [&](int b){ return down? (1u<<b) : 0u; };
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:  io->gpio_in |= bit(0); break;
                case SDLK_RIGHT: io->gpio_in |= bit(1); break;
                case SDLK_UP:    io->gpio_in |= bit(2); break;
                case SDLK_DOWN:  io->gpio_in |= bit(3); break;
                case SDLK_SPACE: io->gpio_in |= bit(4); break;
            }
        }
    }

    /* 2. Run guest CPU for N instructions ------------------------ */
    for (int i = 0; i < 10'000; ++i) cpu->step();

    /* 3. Blit framebuffer ---------------------------------------- */
    SDL_UpdateTexture(tex, nullptr, io->framebuffer, 128);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    SDL_RenderPresent(ren);
}

int main()
{
    /* Build emulator core */
    build_system(io, cpu);                // fills global pointers

    /* SDL video */
    SDL_Init(SDL_INIT_VIDEO);
    win = SDL_CreateWindow("RV‑Game",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           512, 512, 0);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB332,
                            SDL_TEXTUREACCESS_STREAMING, 128, 128);

#ifdef __EMSCRIPTEN__
    // start_game();
    // emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (true) main_loop();             // ~60 fps native
#endif
    return 0;
}
