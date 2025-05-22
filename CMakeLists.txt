cmake_minimum_required(VERSION 3.22)
project(RISCVCpp LANGUAGES CXX VERSION 0.1)

# -----------------------------------------------------------
#  Global compiler settings (apply to both native & emcc)
# -----------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
if (NOT EMSCRIPTEN)
    set(CMAKE_CXX_COMPILER "/opt/homebrew/Cellar/llvm/20.1.2/bin/clang++")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)

# -----------------------------------------------------------
#  Source files (one static library)
# -----------------------------------------------------------
file(GLOB SRC CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM SRC
     ${CMAKE_CURRENT_SOURCE_DIR}/src/sdl_frontend.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/emulator.cpp)
add_library(riscvcpp STATIC ${SRC})
target_include_directories(riscvcpp PUBLIC include)

# Thread option for native builds
option(USE_THREADS "Enable thread‑safe cache lines" ON)
if (USE_THREADS AND NOT EMSCRIPTEN)
    find_package(Threads REQUIRED)
    target_link_libraries(riscvcpp PUBLIC Threads::Threads)
endif()

# ===================================================================
#            TARGETS THAT DEPEND ON THE TOOLCHAIN
# ===================================================================

# -------------------------------------------------------------------
#  1. Native command‑line demo  (built when **NOT** using emcmake)
# -------------------------------------------------------------------
if (NOT EMSCRIPTEN)
    add_executable(test_riscv main.cpp)
    target_link_libraries(test_riscv PRIVATE riscvcpp)

# -------------------------------------------------------------------
#  2. Emscripten / SDL2 browser front‑end  (built when EMSCRIPTEN)
# -------------------------------------------------------------------
else()
    # Emscripten already sets CMAKE_(C)XX_COMPILER to emcc / em++
    set(CMAKE_EXECUTABLE_SUFFIX ".html")        # produce rv_game.html

    add_executable(rv_game
        src/sdl_frontend.cpp
        src/emulator.cpp)

    target_link_libraries(rv_game PRIVATE riscvcpp)

    # SDL2 flags must be on **both** compile and link commands
    target_compile_options(rv_game PRIVATE
        "-sUSE_SDL=2"
        "-sEXCEPTION_CATCHING_ALLOWED=all")
    target_link_options(rv_game PRIVATE
        "-sUSE_SDL=2"
        "-sEXCEPTION_CATCHING_ALLOWED=all"
        "-sASYNCIFY"
        "-sALLOW_MEMORY_GROWTH"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
        "-sEXPORTED_FUNCTIONS=['_start_game']")
endif()


#Native build:
# cmake -S . -B build
# cmake --build build        # → build/test_riscv
# ./build/test_riscv


#WASM build:
# Activate your emsdk first:   source ~/emsdk/emsdk_env.sh

# emcmake cmake -S . -B wasm
# cmake --build wasm          # → wasm/rv_game.html + rv_game.wasm
# python3 -m http.server -d wasm      # open http://localhost:8000/rv_game.html