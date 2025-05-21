cmake_minimum_required(VERSION 3.22)
project(RISCVCpp LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_COMPILER "/opt/homebrew/Cellar/llvm/20.1.2/bin/clang++")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------------------------
#  Build options
# --------------------------------------------------------------------
option(USE_THREADS "Enable thread-safe cache lines" ON)

# --------------------------------------------------------------------
#  Library
# --------------------------------------------------------------------
file(GLOB SRC CONFIGURE_DEPENDS "src/*.cpp")
add_library(riscvcpp STATIC ${SRC})
target_include_directories(riscvcpp PUBLIC include)
target_compile_options(riscvcpp PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

if (USE_THREADS)
    find_package(Threads REQUIRED)
    target_link_libraries(riscvcpp PUBLIC Threads::Threads)
endif()

# --------------------------------------------------------------------
#  Tiny demo executable
# --------------------------------------------------------------------
add_executable(test_riscv main.cpp)
target_link_libraries(test_riscv PRIVATE riscvcpp)

# cmake -S . -B build
# cmake --build ./build