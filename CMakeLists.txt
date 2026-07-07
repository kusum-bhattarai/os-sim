cmake_minimum_required(VERSION 3.15)
project(os-sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- core simulator static library ---
add_library(sim_core STATIC
    src/frame_pool.cpp
    src/page_table.cpp
    src/page_table_two_level.cpp
    src/page_table_hashed.cpp
    src/page_table_inverted.cpp
    src/tlb.cpp
    src/policy/clock.cpp
    src/policy/fifo.cpp
    src/policy/lru.cpp
    src/policy/opt.cpp
    src/policy/working_set.cpp
    src/memory_manager.cpp
    src/metrics.cpp
)
target_include_directories(sim_core PUBLIC src)

# --- third-party dependencies ---
include(FetchContent)
FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG v5.0.0
)
FetchContent_MakeAvailable(ftxui)

# --- test executables ---
add_executable(test_frame_pool   tests/test_frame_pool.cpp)
add_executable(test_page_table   tests/test_page_table.cpp)
add_executable(test_fifo         tests/test_fifo.cpp)
add_executable(test_lru          tests/test_lru.cpp)
add_executable(test_opt          tests/test_opt.cpp)
add_executable(test_memory_manager tests/test_memory_manager.cpp)
add_executable(test_cow          tests/test_cow.cpp)
add_executable(test_tlb          tests/test_tlb.cpp)
add_executable(test_clock        tests/test_clock.cpp)
add_executable(test_two_level_page_table tests/test_two_level_page_table.cpp)
add_executable(test_hashed_page_table tests/test_hashed_page_table.cpp)
add_executable(test_inverted_page_table tests/test_inverted_page_table.cpp)
add_executable(test_working_set tests/test_working_set.cpp)

foreach(t IN ITEMS
    test_frame_pool test_page_table test_fifo test_lru test_opt
    test_memory_manager test_cow test_tlb test_clock test_two_level_page_table
    test_hashed_page_table test_inverted_page_table test_working_set)
    target_link_libraries(${t} PRIVATE sim_core)
endforeach()

# --- experiment executables ---
add_executable(exp_a experiments/exp_a_algorithm_comparison.cpp)
add_executable(exp_b experiments/exp_b_frame_sensitivity.cpp)
add_executable(exp_c experiments/exp_c_cow_read_heavy.cpp)
add_executable(exp_d experiments/exp_d_cow_write_heavy.cpp)
add_executable(exp_e experiments/exp_e_write_timing.cpp)
add_executable(exp_f experiments/exp_f_tlb_locality.cpp)
add_executable(exp_g experiments/exp_g_tlb_size_sweep.cpp)
add_executable(exp_h experiments/exp_h_tlb_shootdown_cost.cpp)
add_executable(exp_i experiments/exp_i_page_table_comparison.cpp)
add_executable(exp_j experiments/exp_j_hashed_collisions.cpp)
add_executable(exp_k experiments/exp_k_inverted_lookup.cpp)
add_executable(exp_l experiments/exp_l_working_set.cpp)

foreach(exp IN ITEMS exp_a exp_b exp_c exp_d exp_e exp_f exp_g exp_h exp_i exp_j exp_k exp_l)
    target_link_libraries(${exp} PRIVATE sim_core)
    target_include_directories(${exp} PRIVATE experiments)
endforeach()

# --- TUI executable ---
add_executable(os_sim_tui
    tui/main.cpp
    tui/simulator_state.cpp
    tui/comparison.cpp
)
target_link_libraries(os_sim_tui
    PRIVATE
    sim_core
    ftxui::screen
    ftxui::dom
    ftxui::component
)
