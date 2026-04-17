cmake_minimum_required(VERSION 3.15)
project(os-sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- source files (add to this list as you build more phases) ---
set(SOURCES
    src/frame_pool.cpp
)

# --- test executables ---
add_executable(test_frame_pool
    tests/test_frame_pool.cpp
    ${SOURCES}
)

target_include_directories(test_frame_pool PRIVATE src)