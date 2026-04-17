#include <cassert>
#include <iostream>
#include <stdexcept>
#include "../src/frame_pool.h"

void test_allocate_returns_valid_index() {
    FramePool pool;
    int idx = pool.allocate();
    assert(idx >= 0 && idx < NUM_FRAMES);
    std::cout << "PASS: test_allocate_returns_valid_index\n";
}

void test_allocated_frame_is_in_use() {
    FramePool pool;
    int idx = pool.allocate();
    assert(pool.get_frame(idx).in_use == true);
    std::cout << "PASS: test_allocated_frame_is_in_use\n";
}

void test_allocated_frame_ref_count_is_one() {
    FramePool pool;
    int idx = pool.allocate();
    assert(pool.get_frame(idx).ref_count == 1);
    std::cout << "PASS: test_allocated_frame_ref_count_is_one\n";
}

void test_allocate_all_frames_throws() {
    FramePool pool;
    for (int i = 0; i < NUM_FRAMES; ++i) pool.allocate();
    try {
        pool.allocate();
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_allocate_all_frames_throws\n";
}

void test_free_allows_reallocation() {
    FramePool pool;
    int idx = pool.allocate();
    pool.deallocate(idx);
    int new_idx = pool.allocate();
    assert(new_idx >= 0 && new_idx < NUM_FRAMES);
    assert(pool.get_frame(new_idx).in_use == true);
    std::cout << "PASS: test_free_allows_reallocation\n";
}

void test_double_free_throws() {
    FramePool pool;
    int idx = pool.allocate();
    pool.deallocate(idx);
    try {
        pool.deallocate(idx);
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_double_free_throws\n";
}

void test_get_frame_out_of_bounds_throws() {
    FramePool pool;
    try {
        pool.get_frame(-1);
        assert(false && "should have thrown");
    } catch (const std::out_of_range&) {}
    try {
        pool.get_frame(NUM_FRAMES);
        assert(false && "should have thrown");
    } catch (const std::out_of_range&) {}
    std::cout << "PASS: test_get_frame_out_of_bounds_throws\n";
}

void test_get_frame_on_freed_frame_throws() {
    FramePool pool;
    int idx = pool.allocate();
    pool.deallocate(idx);
    try {
        pool.get_frame(idx);
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_get_frame_on_freed_frame_throws\n";
}

void test_freed_frame_data_is_zeroed() {
    FramePool pool;
    int idx = pool.allocate();

    // write something to the frame
    Frame& f = pool.get_frame(idx);
    f.data.fill(std::byte{0xFF});

    pool.deallocate(idx);

    // reallocate and check data is zeroed
    int new_idx = pool.allocate();
    Frame& f2 = pool.get_frame(new_idx);
    for (const auto& byte : f2.data) {
        assert(byte == std::byte{0});
    }
    std::cout << "PASS: test_freed_frame_data_is_zeroed\n";
}

int main() {
    test_allocate_returns_valid_index();
    test_allocated_frame_is_in_use();
    test_allocated_frame_ref_count_is_one();
    test_allocate_all_frames_throws();
    test_free_allows_reallocation();
    test_double_free_throws();
    test_get_frame_out_of_bounds_throws();
    test_get_frame_on_freed_frame_throws();
    test_freed_frame_data_is_zeroed();

    std::cout << "\nAll tests passed.\n";
    return 0;
}