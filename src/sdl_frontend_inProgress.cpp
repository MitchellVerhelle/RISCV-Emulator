
#define SDL_MAIN_HANDLED
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif
#include <cstdlib>
#include <iostream>
#include "emulator.hpp"

using namespace rv;

static MmioWindow* io   = nullptr;
static RiscV*      cpu  = nullptr;

#ifdef __EMSCRIPTEN__
static SDL_Renderer* g_renderer = nullptr;
static SDL_Texture*  g_texture  = nullptr;

static void frame()            // called ~60 fps by browser
{
    /* FUTURE GOALS: cpu->step() N times per frame if you want live emulation */

    SDL_UpdateTexture(g_texture, nullptr, io->framebuffer, 128);
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, nullptr, nullptr);
    SDL_RenderPresent(g_renderer);

    /* basic quit handling: if user closed tab → main loop auto-ends       */
}
#endif

int main() {
    build_system(io, cpu);
    if (!io || !io->framebuffer) {
        std::cerr << "Failed to initialise RISC-V system\n";
        return EXIT_FAILURE;
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << '\n';
        return EXIT_FAILURE;
    }

    SDL_Window*   win = SDL_CreateWindow("RV-Game",
                                         SDL_WINDOWPOS_CENTERED,
                                         SDL_WINDOWPOS_CENTERED,
                                         786, 786, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(ren, 128, 128);

    SDL_Texture* tex = SDL_CreateTexture(ren,
                                         SDL_PIXELFORMAT_RGB332,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         128, 128);

    // 3. Main loop – blit FB to the screen every ~16 ms
#ifdef __EMSCRIPTEN__
    /* hand control to the browser */
    g_renderer = ren;
    g_texture  = tex;
    emscripten_set_main_loop(frame, 0, /*simulate_infinite_loop=*/true);
#else
    /* native desktop: traditional loop                                   */
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
        }
        /* render */
        SDL_UpdateTexture(tex, nullptr, io->framebuffer, 128);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);
        SDL_Delay(16);          // ~60 fps
    }
#endif

    // 4. Clean-up
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}