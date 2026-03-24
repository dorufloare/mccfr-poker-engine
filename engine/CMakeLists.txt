cmake_minimum_required(VERSION 3.16)
project(PokerEngine LANGUAGES CXX)

# --- C++ settings ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional optimization flags for low latency
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -march=native -Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/O2 /W4)
endif()

# --- Include directories ---
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/io
)

# --- Source files ---
file(GLOB ENGINE_SOURCES
    src/*.cpp
    src/bots/*.cpp
    io/*.cpp
)

# --- Main executable ---
add_executable(poker_engine ${ENGINE_SOURCES})

# --- Test executable (all engine sources except main.cpp) ---
file(GLOB TEST_ENGINE_SOURCES
    src/*.cpp
    src/bots/*.cpp
    io/*.cpp
)
list(FILTER TEST_ENGINE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

add_executable(poker_tests
    tests/test_evaluation.cpp
    ${TEST_ENGINE_SOURCES}
)
target_include_directories(poker_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/io
)
