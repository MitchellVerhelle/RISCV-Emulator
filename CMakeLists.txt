cmake_minimum_required(VERSION 3.22)
project(RISCVCpp LANGUAGES CXX VERSION 0.1)

option(ENABLE_PTHREADS "Build WebAssembly with pthread support" ON)

#-------------------------------------------------------------------
#  Parallel STL / TBB / OpenMP support (macOS Homebrew layout)
#-------------------------------------------------------------------
if (NOT EMSCRIPTEN)
    list(APPEND CMAKE_PREFIX_PATH
    "/opt/homebrew/opt/tbb/lib/cmake/TBB"
    "/opt/homebrew/opt/libomp"
    "/opt/homebrew/Cellar/onedpl/2022.8.0/lib/cmake/oneDPL"
    )

    find_package(TBB REQUIRED)
    find_package(OpenMP REQUIRED)
    find_package(oneDPL REQUIRED)
endif()

# ------------------------------------------------------------------
#  CTRE
# ------------------------------------------------------------------
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND NOT EMSCRIPTEN)
  set(CMAKE_TOOLCHAIN_FILE
      "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
endif()

if(NOT DEFINED CMAKE_PREFIX_PATH AND NOT EMSCRIPTEN)
  set(CMAKE_PREFIX_PATH
      "$ENV{HOME}/vcpkg/installed/arm64-osx"
      CACHE STRING "Vcpkg prefix path")
endif()

# -----------------------------------------------------------
#  Global compiler settings (apply to both native & emcc)
# -----------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
if (NOT EMSCRIPTEN)
    set(CMAKE_CXX_COMPILER "/opt/homebrew/Cellar/llvm/20.1.2/bin/clang++")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)

if (NOT EMSCRIPTEN)
    find_package(ctre CONFIG REQUIRED)
endif()

# -----------------------------------------------------------
#  Source files (one static library)
# -----------------------------------------------------------
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM SRC
     ${CMAKE_CURRENT_SOURCE_DIR}/src/sdl_frontend.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/emulator.cpp)
add_library(riscvcpp STATIC ${SRC})
target_include_directories(riscvcpp PUBLIC include)

if (EMSCRIPTEN AND ENABLE_PTHREADS)
    set(PTHREAD_FLAGS
        -pthread
        -sUSE_PTHREADS=1
        -sPTHREAD_POOL_SIZE=4)
    target_compile_options(riscvcpp PRIVATE ${PTHREAD_FLAGS})
    target_link_options   (riscvcpp PRIVATE ${PTHREAD_FLAGS})
else()
    set(PTHREAD_FLAGS "")
endif()

# give OneDPL headers to anyone who uses riscvcpp
target_include_directories(riscvcpp PUBLIC
  /opt/homebrew/Cellar/onedpl/2022.8.0/include
)

# optimize with libc++ and O3 for all downstream targets
target_compile_options(riscvcpp PUBLIC
  $<$<NOT:$<BOOL:${EMSCRIPTEN}>>:-stdlib=libc++>
  -O3
)

# --- link CTRE -----------------------------------
target_link_libraries(riscvcpp
    PUBLIC
      # CTRE (vcpkg) or single-header for WASM
      $<$<NOT:$<BOOL:${EMSCRIPTEN}>>:ctre::ctre>

      # C++ threads (only if we found it)
      $<$<AND:$<NOT:$<BOOL:${EMSCRIPTEN}>>,$<TARGET_EXISTS:Threads::Threads>>:Threads::Threads>

      # TBB, OpenMP, oneDPL (only on native)
      $<$<NOT:$<BOOL:${EMSCRIPTEN}>>:TBB::tbb>
      $<$<NOT:$<BOOL:${EMSCRIPTEN}>>:OpenMP::OpenMP_CXX>
      $<$<NOT:$<BOOL:${EMSCRIPTEN}>>:oneDPL>
)

if (EMSCRIPTEN)
    include(FetchContent)
    FetchContent_Declare(
        ctre
        GIT_REPOSITORY https://github.com/hanickadot/compile-time-regular-expressions.git
        GIT_TAG        main          # use 'main' branch instead of missing tag
    )
    FetchContent_Populate(ctre)    # clone only, no sub-build
    target_include_directories(riscvcpp PUBLIC
        ${ctre_SOURCE_DIR}/single-header
    )
endif()

# Thread option for native builds
option(USE_THREADS "Enable thread-safe cache lines" ON)
if (USE_THREADS AND NOT EMSCRIPTEN)
    find_package(Threads REQUIRED)
    target_link_libraries(riscvcpp PUBLIC Threads::Threads)
endif()

# ===================================================================
#            TARGETS THAT DEPEND ON THE TOOLCHAIN
# ===================================================================

# -------------------------------------------------------------------
#  1. Native command-line demo  (built when **NOT** using emcmake)
# -------------------------------------------------------------------
if (NOT EMSCRIPTEN)
    add_executable(test_riscv          main.cpp)
    add_executable(cache_stats_demo    examples/cache_stats_demo.cpp)
    add_executable(parallel_stress     examples/parallel_stress.cpp)

    target_link_libraries(test_riscv       PRIVATE riscvcpp)
    target_link_libraries(cache_stats_demo PRIVATE riscvcpp)
    target_link_libraries(parallel_stress  PRIVATE riscvcpp)


# -------------------------------------------------------------------
#  2. Emscripten / SDL2 browser front-end  (built when EMSCRIPTEN)
# -------------------------------------------------------------------
else()
    # Emscripten already sets CMAKE_(C)XX_COMPILER to emcc / em++
    set(CMAKE_EXECUTABLE_SUFFIX ".html")        # produce rv_game.html

    # rv_game executable
    add_executable(rv_game
        src/sdl_frontend.cpp
        src/emulator.cpp)

    target_link_libraries(rv_game PRIVATE riscvcpp)

    target_compile_options(rv_game PRIVATE
        -sUSE_SDL=2
        -sEXCEPTION_CATCHING_ALLOWED=all
        ${PTHREAD_FLAGS})

    target_link_options(rv_game PRIVATE
        -sUSE_SDL=2
        -sEXCEPTION_CATCHING_ALLOWED=all
        -sASYNCIFY
        -sALLOW_MEMORY_GROWTH
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
        -sEXPORTED_FUNCTIONS=['_main']
        ${PTHREAD_FLAGS})
endif()


#Native build:
# cmake -S . -B build
# cmake --build build            # -> build/test_riscv, build/cache_stats_demo, build/parallel_stress
# ./build/test_riscv
# ./build/cache_stats_demo
# ./build/parallel_stress


#WASM build:
# Activate your emsdk first:   source ~/emsdk/emsdk_env.sh

# emcmake cmake -S . -B wasm
# cmake --build wasm             # -> wasm/rv_game.html + rv_game.wasm
# python3 -m http.server -d wasm # open http://localhost:8000/rv_game.html