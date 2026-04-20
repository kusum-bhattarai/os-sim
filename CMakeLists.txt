cmake_minimum_required(VERSION 3.15)
project(os-sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- source files (add to this list as you build more phases) ---
set(SOURCES
    src/frame_pool.cpp
    src/page_table.cpp
    src/policy/fifo.cpp
    src/policy/lru.cpp
    src/policy/opt.cpp
    src/memory_manager.cpp
    src/metrics.cpp
    )

# --- test executables ---
add_executable(test_frame_pool
    tests/test_frame_pool.cpp
    ${SOURCES}
)

add_executable(test_page_table
    tests/test_page_table.cpp
    ${SOURCES}
)

add_executable(test_fifo
    tests/test_fifo.cpp
    ${SOURCES}
)

add_executable(test_lru
    tests/test_lru.cpp
    ${SOURCES}
)

add_executable(test_opt
    tests/test_opt.cpp
    ${SOURCES}
)

add_executable(test_memory_manager
    tests/test_memory_manager.cpp
    ${SOURCES}
)

add_executable(test_cow
    tests/test_cow.cpp
    ${SOURCES}
)

target_include_directories(test_frame_pool PRIVATE src)
target_include_directories(test_page_table PRIVATE src)
target_include_directories(test_fifo PRIVATE src)
target_include_directories(test_lru PRIVATE src)
target_include_directories(test_opt PRIVATE src)
target_include_directories(test_cow PRIVATE src)

# --- experiment executables ---
add_executable(exp_a experiments/exp_a_algorithm_comparison.cpp ${SOURCES})
add_executable(exp_b experiments/exp_b_frame_sensitivity.cpp ${SOURCES})
add_executable(exp_c experiments/exp_c_cow_read_heavy.cpp ${SOURCES})
add_executable(exp_d experiments/exp_d_cow_write_heavy.cpp ${SOURCES})
add_executable(exp_e experiments/exp_e_write_timing.cpp ${SOURCES})

target_include_directories(exp_a PRIVATE src experiments)
target_include_directories(exp_b PRIVATE src experiments)
target_include_directories(exp_c PRIVATE src experiments)
target_include_directories(exp_d PRIVATE src experiments)
target_include_directories(exp_e PRIVATE src experiments)