#include <cassert>
#include <iostream>
#include "../src/alloc/buddy_allocator.h"

void test_order_for_rounds_up() {
    assert(BuddyAllocator::order_for(1) == 0);
    assert(BuddyAllocator::order_for(2) == 1);
    assert(BuddyAllocator::order_for(3) == 2);
    assert(BuddyAllocator::order_for(4) == 2);
    assert(BuddyAllocator::order_for(5) == 3);
    assert(BuddyAllocator::order_for(0) == -1);

    std::cout << "PASS: test_order_for_rounds_up\n";
}

void test_constructor_requires_power_of_two() {
    bool threw = false;
    try { BuddyAllocator buddy(24); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);

    std::cout << "PASS: test_constructor_requires_power_of_two\n";
}

void test_first_allocation_splits_down() {
    BuddyAllocator buddy(16);
    int base = buddy.allocate(1);

    assert(base == 0);
    assert(buddy.free_count(0) == 1); // page 1
    assert(buddy.free_count(1) == 1); // pages 2-3
    assert(buddy.free_count(2) == 1); // pages 4-7
    assert(buddy.free_count(3) == 1); // pages 8-15
    assert(buddy.free_pages() == 15);

    std::cout << "PASS: test_first_allocation_splits_down\n";
}

void test_adjacent_allocations_are_buddies() {
    BuddyAllocator buddy(16);
    int a = buddy.allocate(1);
    int b = buddy.allocate(1);

    assert(a == 0);
    assert(b == 1); // reuses the split-off buddy

    std::cout << "PASS: test_adjacent_allocations_are_buddies\n";
}

void test_free_coalesces_back_to_full_block() {
    BuddyAllocator buddy(16);
    int a = buddy.allocate(1);
    int b = buddy.allocate(1);
    buddy.free(a);
    buddy.free(b);

    assert(buddy.free_pages() == 16);
    assert(buddy.largest_free_block() == 16);
    assert(buddy.free_count(4) == 1);

    std::cout << "PASS: test_free_coalesces_back_to_full_block\n";
}

void test_allocation_rounds_to_power_of_two() {
    BuddyAllocator buddy(16);
    buddy.allocate(3); // rounds to 4 pages

    assert(buddy.free_pages() == 12);

    std::cout << "PASS: test_allocation_rounds_to_power_of_two\n";
}

void test_fragmentation_blocks_large_allocation() {
    BuddyAllocator buddy(16);
    int bases[16];
    for (int i = 0; i < 16; i++)
        bases[i] = buddy.allocate(1);
    for (int i = 0; i < 16; i += 2)
        buddy.free(bases[i]); // free every other page

    assert(buddy.free_pages() == 8);
    assert(buddy.largest_free_block() == 1);
    assert(buddy.allocate(2) == -1); // half the pool is free but unusable

    for (int i = 1; i < 16; i += 2)
        buddy.free(bases[i]);
    assert(buddy.largest_free_block() == 16); // full coalesce
    assert(buddy.allocate(2) == 0);

    std::cout << "PASS: test_fragmentation_blocks_large_allocation\n";
}

void test_oversize_returns_minus_one() {
    BuddyAllocator buddy(16);
    assert(buddy.allocate(32) == -1);

    bool threw = false;
    try { buddy.allocate(0); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);

    std::cout << "PASS: test_oversize_returns_minus_one\n";
}

void test_invalid_free_throws() {
    BuddyAllocator buddy(16);
    int a = buddy.allocate(1);
    buddy.free(a);

    bool threw = false;
    try { buddy.free(a); } catch (const std::runtime_error&) { threw = true; }
    assert(threw);

    threw = false;
    try { buddy.free(5); } catch (const std::runtime_error&) { threw = true; }
    assert(threw);

    std::cout << "PASS: test_invalid_free_throws\n";
}

void test_exhaustion_and_reuse() {
    BuddyAllocator buddy(8);
    assert(buddy.allocate(8) == 0);
    assert(buddy.allocate(1) == -1);

    buddy.free(0);
    assert(buddy.allocate(4) == 0);
    assert(buddy.allocate(4) == 4);

    std::cout << "PASS: test_exhaustion_and_reuse\n";
}

int main() {
    test_order_for_rounds_up();
    test_constructor_requires_power_of_two();
    test_first_allocation_splits_down();
    test_adjacent_allocations_are_buddies();
    test_free_coalesces_back_to_full_block();
    test_allocation_rounds_to_power_of_two();
    test_fragmentation_blocks_large_allocation();
    test_oversize_returns_minus_one();
    test_invalid_free_throws();
    test_exhaustion_and_reuse();

    std::cout << "\nAll BuddyAllocator tests passed.\n";
    return 0;
}
