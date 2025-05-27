# Cache & RISC-V Interpreter
### Mitch Verhelle (mverhelle)
### Project for MPCS 51045: Advanced C++, taught by Professor Mike Spertus and TA'd by Yaodan (Kaitlyn) Zhang
### Description
This project was originally an extension to an old project I wrote in C. The project was for a class called CS 3410: Computer System Organization at Cornell University. I wanted to improve the code quality using what I've learned in this class (MPCS 51045: Advanced C++). As I was developing the project, I made improvements to the original project such as CMake, emscripten webassembly, template metaprogramming, an Assembly text parser using CTRE (Compile-Time Regular Expressions) into RISCV 32-bit instructions, and more. I originally wanted to just improve my old code, but after Matt Godbolt's presentation in our last lecture, I was inspired to continue adding to the project until I have a fully functioning RISC-V emulator, with which I can run simple 2D videogames written in Assembly.

## Features
### Memory Features
- **MemoryBus**: Struct intended to be extended by other classes like Cache and HashTable that has three fields, a virtual std::optional<std::uint32_t> load_word, virtual bool store_word, and virtual destructor.
- **Cache**: Simple cache implementation from original project. Improved by adding a Cache class that extends MemoryBus and contains members that makes use of std::optional, unique_ptr, separates implementation from interface, encloses in shared rv namespace.
- **Cache Stats Formatter**: Like lecture 10, creates a std::formatter<rv::CacheStats, char> specialization that makes it easy to print cache stats using std::format.
- **HashTable**: A simple hash table implementation that extends MemoryBus. Uses linear probing for collision. Not thread-safe.
- **ConcurrentHashTable**: Thread-safe hash table. Uses unique_lock and shared_mutex. Used in main.cpp and emulator.cpp for dram. Uses execution policy from oneDPL (oneAPI DPC++ Library) to parallelize the hash table operations.
- **LinkedList**: Copy and move constructible, singly linked list. Not thread-safe. Uses std::unique_ptr for nodes and std::optional return type for find.
- **LockFreeList**: Similar to lock-free-stack from lecture 8, implements a lock-free singly linked list using atomics. Can be tested by running examples/parallel_stress from the CMake build, along with ConcurrentHashTable.
### RISC-V Interpreter Features
- **RISCV Types**: A header file containing relevant types for RISC-V. Constains OpCode enum, sign_extend function, structs for RType, IType, SType, and BType instruction formats, and a using Instr = std::variant<RType,IType,SType,BType> type alias to abstract instructions.
- **RISCV Decode Templates**: A set of template functions to decode RISC-V instructions from a 32-bit instruction word. Uses index_sequence to build decoder table using template partial specialization. Inspired by Matt Godbolt's presentation.
- **RISCV**: Contains essential logic for CPU, like memory, registers, program counter, and step function. Constructor takes MemoryBus (memory).
- **rv_assembler**: Uses CTRE to parse Assembly text into RISC-V instructions (32-bit). Uses CTRE to parse assembly into instructions.
### Emscripten
- **mmio_window**: Memory-mapped I/O window interface for the emulator.
- **text/bitmap_font**: Glyphs for writing text to the screen.
- **sdl_frontend**: SDL2 frontend for the emulator. Uses SDL2 to create a window and render text and graphics. Uses mmio_window to handle memory-mapped I/O.
- **demo_loop**: A simple demo loop that runs the emulator and renders text and graphics to the screen. Uses SDL2 to handle events and render text and graphics. Mostly no longer in use but there for reference.
- **emulator**: Contains the main logic for the emulator. Uses SDL2 to create a window and render text and graphics. Uses mmio_window to handle memory-mapped I/O.

## Running -- Native
- Build with CMake
```bash
# native build:
cmake -S . -B build
cmake --build build        # -> build/test_riscv, build/cache_stats_demo, build/parallel_stress
./build/test_riscv
./build/cache_stats_demo
./build/parallel_stress
```
- **cache_stats_demo**: Tests Cache and CacheStatsFormatter. Prints cache stats using std::format.
- **parallel_stress**: Tests ConcurrentHashTable and LockFreeList.
- **test_riscv**: Built from main.cpp, the entry point for the program. Executes example program that adds numbers to 10 and prints the result. Outputs runtime statistics using chrono and cache stats. Uses the concurrent features like for_each, par, and par_unseq for faster memory load operations.

## Running -- Emscripten
- Install [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) and follow their instructions to set up. (Very smooth!)
- Build with EmCMake
```bash
# activate your emsdk first: (MAC OS if you followed emscripten installation instructions)
source ~/emsdk/emsdk_env.sh

# emscripten build:
emcmake cmake -S . -B wasm
cmake --build wasm             # -> wasm/rv_game.html + rv_game.wasm
python3 -m http.server -d wasm # open http://localhost:8000/rv_game.html
```

## Acknowledgments and Resources
(Links may not work if you don't have access to Cornell's Canvas, but I can show them to you personally if you'd like. Please just ask!)
- [Original Project Instructions -- Cache             (in C)](https://canvas.cornell.edu/courses/54972/assignments/522719?module_item_id=2052989)
- [Original Project Instructions -- RISCV Interpreter (in C)](https://canvas.cornell.edu/courses/54972/assignments/522718?module_item_id=2052986)
- Professor: [Anne Bracy](https://www.engineering.cornell.edu/people/anne-bracy/)
- [RISCV ISA Instruction Manual](https://lf-riscv.atlassian.net/wiki/spaces/HOME/pages/16154769/RISC-V+Technical+Specifications#ISA-Specifications)
- [Textbook: Computer Organization and Design RISC-V Edition David A. Patterson; John L. Hennessy](https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/books/HandP_RISCV.pdf)

(Acknowledgements of the original project.)
Acknowledgments. Concept for this project (Cache & RISCV Interpreter) derived from https://dannyqiu.me/mips-interpreter/. Makefile reading material is from OS textbook OSTEP <http://pages.cs.wisc.edu/~remzi/OSTEP/>. The project is converted from MIPS-based to RISC-V-based by a team of 5 course staff during Fall 2019.



## Setup environment
To set up the environment, you will need to have the following installed:
- [Clang LLVM]()
- [CMake]()
- [vcpkg]()
- [CTRE]()
- [TBB]()
- [oneDPL]()
- [Emscripten]()
- [SDL2]()

This project runs C++23. I am using LLVM version 20.1.2, so anything that or later should work. I am also using CMake version 3.22, so anything that or later should work as well. I used Homebrew to install llvm:
- [Homebrew](https://brew.sh/)

verbatim c_cpp_properties.json
```json
{
    "configurations": [
        {
            "name": "Mac",
            "includePath": [
                "${workspaceFolder}/**",
                "${vcpkgRoot}/arm64-osx/include",
                "${env:HOME}/emsdk/upstream/emscripten/system/include",
                "${env:HOME}/emsdk/upstream/emscripten/system/lib/libcxx/include",
                "${env:HOME}/emsdk/upstream/emscripten/cache/sysroot/include",
                "${env:HOME}/emsdk/upstream/emscripten/cache/sysroot/include/SDL",
                "~/vcpkg/installed/arm64-osx/include",
                "/opt/homebrew/Cellar/onedpl/2022.8.0/include",
                "/opt/homebrew/Cellar/tbb/2022.1.0/include",
                "/opt/homebrew/Cellar/llvm/20.1.2/include/c++/v1"
            ],
            "defines": [
                "__EMSCRIPTEN__"
            ],
            "compilerPath": "/opt/homebrew/Cellar/llvm/20.1.2/bin/clang++",
            "cStandard": "c17",
            "cppStandard": "c++23",
            "intelliSenseMode": "macos-clang-arm64"
        }
    ],
    "version": 4
}
```

verbatim CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.22)
project(RISCVCpp LANGUAGES CXX VERSION 0.1)

#-------------------------------------------------------------------
#  Parallel STL / TBB / OpenMP support (macOS Homebrew layout)
#-------------------------------------------------------------------
list(APPEND CMAKE_PREFIX_PATH
  "/opt/homebrew/opt/tbb/lib/cmake/TBB"
  "/opt/homebrew/opt/libomp"
  "/opt/homebrew/Cellar/onedpl/2022.8.0/lib/cmake/oneDPL"
)

find_package(TBB     REQUIRED)
find_package(OpenMP REQUIRED)
find_package(oneDPL  REQUIRED)

# ------------------------------------------------------------------
#  CTRE (header-only)
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

find_package(ctre CONFIG REQUIRED)

# -----------------------------------------------------------
#  Source files (one static library)
# -----------------------------------------------------------
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM SRC
     ${CMAKE_CURRENT_SOURCE_DIR}/src/sdl_frontend.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/emulator.cpp)
add_library(riscvcpp STATIC ${SRC})
target_include_directories(riscvcpp PUBLIC include)

# give oneDPL headers to the target
target_include_directories(riscvcpp PRIVATE
  /opt/homebrew/Cellar/onedpl/2022.8.0/include
)

# optimize with libc++ and O3 (as in your example)
target_compile_options(riscvcpp PRIVATE
  -stdlib=libc++
  -O3
)

# --- link CTRE -----------------------------------
target_link_libraries(riscvcpp
    PUBLIC
      ctre::ctre
      Threads::Threads
    PRIVATE
      TBB::tbb
      OpenMP::OpenMP_CXX
      oneDPL
)

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
        "-sEXPORTED_FUNCTIONS=['_main']")
endif()


#Native build:
# cmake -S . -B build
# cmake --build build        # -> build/test_riscv
# ./build/test_riscv
# ./build/cache_stats_demo
# ./build/parallel_stress


#WASM build:
# Activate your emsdk first:   source ~/emsdk/emsdk_env.sh

# emcmake cmake -S . -B wasm
# cmake --build wasm          # -> wasm/rv_game.html + rv_game.wasm
# python3 -m http.server -d wasm      # open http://localhost:8000/rv_game.html
```
