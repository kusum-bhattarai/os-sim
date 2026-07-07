#include <cassert>
#include <iostream>
#include "../src/alloc/slab_allocator.h"
#include "../src/constants.h"

void test_size_class_round_up() {
    assert(SlabAllocator::size_class_for(1)   == 0);
    assert(SlabAllocator::size_class_for(16)  == 0);
    assert(SlabAllocator::size_class_for(17)  == 1);
    assert(SlabAllocator::size_class_for(200) == 4);
    assert(SlabAllocator::size_class_for(256) == 4);
    assert(SlabAllocator::size_class_for(257) == -1);
    assert(SlabAllocator::size_class_for(0)   == -1);

    std::cout << "PASS: test_size_class_round_up\n";
}

void test_allocations_share_slab() {
    FramePool pool(4);
    SlabAllocator slab(pool);

    int a = slab.allocate(16);
    int b = slab.allocate(16);

    assert(a / (int)PAGE_SIZE == b / (int)PAGE_SIZE); // same frame
    assert(a != b);
    assert(slab.slab_count(0) == 1);
    assert(slab.frames_used() == 1);
    assert(slab.objects_in_use(0) == 2);

    std::cout << "PASS: test_allocations_share_slab\n";
}

void test_address_aligned_to_class_size() {
    FramePool pool(4);
    SlabAllocator slab(pool);

    int a = slab.allocate(20); // class 32
    assert(a % 32 == 0);

    std::cout << "PASS: test_address_aligned_to_class_size\n";
}

void test_slab_grows_when_full() {
    FramePool pool(4);
    SlabAllocator slab(pool);

    int per_slab = PAGE_SIZE / 16;
    for (int i = 0; i < per_slab; i++)
        slab.allocate(16);
    assert(slab.slab_count(0) == 1);

    slab.allocate(16); // 65th object
    assert(slab.slab_count(0) == 2);
    assert(slab.frames_used() == 2);

    std::cout << "PASS: test_slab_grows_when_full\n";
}

void test_free_reuses_slot_lifo() {
    FramePool pool(4);
    SlabAllocator slab(pool);

    slab.allocate(16);
    int b = slab.allocate(16);
    slab.free(b);
    int c = slab.allocate(16);

    assert(c == b);
    assert(slab.objects_in_use(0) == 2);

    std::cout << "PASS: test_free_reuses_slot_lifo\n";
}

void test_empty_slab_returns_frame() {
    FramePool pool(1);
    SlabAllocator slab(pool);

    int a = slab.allocate(64);
    assert(slab.frames_used() == 1);
    slab.free(a);
    assert(slab.frames_used() == 0);
    assert(slab.slab_count(2) == 0);

    // the only frame is free again
    int frame = pool.allocate();
    assert(frame == 0);

    std::cout << "PASS: test_empty_slab_returns_frame\n";
}

void test_distinct_classes_use_distinct_slabs() {
    FramePool pool(4);
    SlabAllocator slab(pool);

    int a = slab.allocate(16);
    int b = slab.allocate(256);

    assert(a / (int)PAGE_SIZE != b / (int)PAGE_SIZE);
    assert(slab.slab_count(0) == 1);
    assert(slab.slab_count(4) == 1);

    std::cout << "PASS: test_distinct_classes_use_distinct_slabs\n";
}

void test_oversize_throws() {
    FramePool pool(4);
    SlabAllocator slab(pool);

    bool threw = false;
    try { slab.allocate(300); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);

    std::cout << "PASS: test_oversize_throws\n";
}

void test_double_free_throws() {
    FramePool pool(4);
    SlabAllocator slab(pool);

    int a = slab.allocate(16);
    slab.allocate(16); // keep the slab alive after the first free
    slab.free(a);

    bool threw = false;
    try { slab.free(a); } catch (const std::runtime_error&) { threw = true; }
    assert(threw);

    std::cout << "PASS: test_double_free_throws\n";
}

void test_fragmentation_stats() {
    FramePool pool(4);
    SlabAllocator slab(pool);

    slab.allocate(20); // class 32
    slab.allocate(100); // class 128

    assert(slab.bytes_requested() == 120);
    assert(slab.bytes_reserved() == 160);

    std::cout << "PASS: test_fragmentation_stats\n";
}

int main() {
    test_size_class_round_up();
    test_allocations_share_slab();
    test_address_aligned_to_class_size();
    test_slab_grows_when_full();
    test_free_reuses_slot_lifo();
    test_empty_slab_returns_frame();
    test_distinct_classes_use_distinct_slabs();
    test_oversize_throws();
    test_double_free_throws();
    test_fragmentation_stats();

    std::cout << "\nAll SlabAllocator tests passed.\n";
    return 0;
}
