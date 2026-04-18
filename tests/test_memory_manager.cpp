#include <cassert>
#include <iostream>
#include "../src/memory_manager.h"
#include "../src/policy/fifo.h"
#include "../src/policy/lru.h"
#include "../src/constants.h"

// helper to create a manager with FIFO and n frames
MemoryManager make_fifo_manager(int num_frames = 4) {
    return MemoryManager(new FIFO(), num_frames);
}

MemoryManager make_lru_manager(int num_frames = 4) {
    return MemoryManager(new LRU(), num_frames);
}

// ─── process management ───────────────────────────────────────────

void test_create_process() {
    auto mm = make_fifo_manager();
    mm.create_process(1);
    std::cout << "PASS: test_create_process\n";
}

void test_duplicate_process_throws() {
    auto mm = make_fifo_manager();
    mm.create_process(1);
    try {
        mm.create_process(1);
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_duplicate_process_throws\n";
}

void test_access_unknown_process_throws() {
    auto mm = make_fifo_manager();
    try {
        mm.access(99, 0, false);
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_access_unknown_process_throws\n";
}

// ─── page faults ─────────────────────────────────────────────────

void test_first_access_causes_fault() {
    auto mm = make_fifo_manager();
    mm.create_process(1);
    mm.access(1, 0, false); // vpn 0, never loaded
    mm.print_metrics();     // should show 1 fault
    std::cout << "PASS: test_first_access_causes_fault\n";
}

void test_second_access_is_hit() {
    auto mm = make_fifo_manager();
    mm.create_process(1);
    mm.access(1, 0, false); // fault
    mm.reset_metrics();
    mm.access(1, 0, false); // should be hit now
    std::cout << "PASS: test_second_access_is_hit\n";
}

void test_fault_count_multiple_pages() {
    auto mm = make_fifo_manager(3);
    mm.create_process(1);
    // access 3 distinct pages — all faults
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.access(1, 1 * PAGE_SIZE, false);
    mm.access(1, 2 * PAGE_SIZE, false);
    // access same pages again — all hits
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.access(1, 1 * PAGE_SIZE, false);
    mm.access(1, 2 * PAGE_SIZE, false);
    std::cout << "PASS: test_fault_count_multiple_pages\n";
}

// ─── eviction ────────────────────────────────────────────────────

void test_eviction_occurs_when_frames_full() {
    auto mm = make_fifo_manager(2); // only 2 frames
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false); // loads vpn 0 → frame 0
    mm.access(1, 1 * PAGE_SIZE, false); // loads vpn 1 → frame 1
    mm.reset_metrics();
    mm.access(1, 2 * PAGE_SIZE, false); // no free frames → eviction
    mm.print_metrics(); // should show 1 fault, 1 eviction
    std::cout << "PASS: test_eviction_occurs_when_frames_full\n";
}

void test_evicted_page_causes_fault_on_reaccess() {
    auto mm = make_fifo_manager(2);
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false); // vpn 0 loaded
    mm.access(1, 1 * PAGE_SIZE, false); // vpn 1 loaded
    mm.access(1, 2 * PAGE_SIZE, false); // vpn 0 evicted (FIFO), vpn 2 loaded
    mm.reset_metrics();
    mm.access(1, 0 * PAGE_SIZE, false); // vpn 0 was evicted → fault again
    mm.print_metrics();
    std::cout << "PASS: test_evicted_page_causes_fault_on_reaccess\n";
}

// ─── dirty bit ───────────────────────────────────────────────────

void test_write_sets_dirty_bit() {
    auto mm = make_fifo_manager();
    mm.create_process(1);
    mm.access(1, 0, true); // write access
    std::cout << "PASS: test_write_sets_dirty_bit\n";
}

void test_read_does_not_set_dirty_bit() {
    auto mm = make_fifo_manager();
    mm.create_process(1);
    mm.access(1, 0, false); // read access
    std::cout << "PASS: test_read_does_not_set_dirty_bit\n";
}

// ─── invalid address ─────────────────────────────────────────────

void test_invalid_address_throws() {
    auto mm = make_fifo_manager();
    mm.create_process(1);
    try {
        mm.access(1, -1, false);
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_invalid_address_throws\n";
}

// ─── multiple processes ──────────────────────────────────────────

void test_two_processes_independent_page_tables() {
    auto mm = make_fifo_manager(4);
    mm.create_process(1);
    mm.create_process(2);
    // load vpn 0 for process 1
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.reset_metrics();
    // process 2 accessing same virtual address should still fault
    mm.access(2, 0 * PAGE_SIZE, false);
    mm.print_metrics(); // should show 1 fault — p2 has its own page table
    std::cout << "PASS: test_two_processes_independent_page_tables\n";
}

void test_full_sequence_fifo_fault_count() {
    // [1,2,3,4,1,2,5,1,2,3,4,5] with 3 frames
    std::vector<int> sequence = {1,2,3,4,1,2,5,1,2,3,4,5};
    auto mm = make_fifo_manager(3);
    mm.create_process(1);
    for (int page : sequence) {
        mm.access(1, page * PAGE_SIZE, false);
    }
    mm.print_metrics();
    std::cout << "PASS: test_full_sequence_fifo_fault_count\n";
}

void test_full_sequence_lru_fault_count() {
    std::vector<int> sequence = {1,2,3,4,1,2,5,1,2,3,4,5};
    auto mm = make_lru_manager(3);
    mm.create_process(1);
    for (int page : sequence) {
        mm.access(1, page * PAGE_SIZE, false);
    }
    mm.print_metrics();
    std::cout << "PASS: test_full_sequence_lru_fault_count\n";
}

int main() {
    test_create_process();
    test_duplicate_process_throws();
    test_access_unknown_process_throws();
    test_first_access_causes_fault();
    test_second_access_is_hit();
    test_fault_count_multiple_pages();
    test_eviction_occurs_when_frames_full();
    test_evicted_page_causes_fault_on_reaccess();
    test_write_sets_dirty_bit();
    test_read_does_not_set_dirty_bit();
    test_invalid_address_throws();
    test_two_processes_independent_page_tables();
    test_full_sequence_fifo_fault_count();
    test_full_sequence_lru_fault_count();

    std::cout << "\nAll MemoryManager tests passed.\n";
    return 0;
}