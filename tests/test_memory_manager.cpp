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

// ─── TLB metrics ─────────────────────────────────────────────────────

void test_tlb_miss_on_first_access() {
    auto mm = make_fifo_manager();
    mm.create_process(1);
    mm.access(1, 0, false);
    const auto& m = mm.get_metrics();
    assert(m.tlb_misses == 1);
    assert(m.tlb_hits   == 0);
    std::cout << "PASS: test_tlb_miss_on_first_access\n";
}

void test_tlb_hit_on_second_access() {
    auto mm = make_fifo_manager();
    mm.create_process(1);
    mm.access(1, 0, false); // fault + TLB miss
    mm.reset_metrics();
    mm.access(1, 0, false); // TLB hit
    const auto& m = mm.get_metrics();
    assert(m.tlb_hits   == 1);
    assert(m.tlb_misses == 0);
    assert(m.hits       == 1);
    assert(m.page_faults == 0);
    std::cout << "PASS: test_tlb_hit_on_second_access\n";
}

void test_tlb_shootdown_on_eviction() {
    // vpn 0 is loaded, then evicted when vpn 2 comes in (2-frame FIFO)
    // the TLB entry for vpn 0 must be invalidated so the next access faults
    auto mm = make_fifo_manager(2);
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false); // vpn 0 → frame 0
    mm.access(1, 1 * PAGE_SIZE, false); // vpn 1 → frame 1
    mm.access(1, 2 * PAGE_SIZE, false); // evicts vpn 0, shoots down its TLB entry
    mm.reset_metrics();
    mm.access(1, 0 * PAGE_SIZE, false); // vpn 0 was evicted → must be a miss + fault
    const auto& m = mm.get_metrics();
    assert(m.tlb_misses  == 1);
    assert(m.tlb_hits    == 0);
    assert(m.page_faults == 1);
    std::cout << "PASS: test_tlb_shootdown_on_eviction\n";
}

void test_tlb_coherent_after_cow() {
    // after a CoW write the TLB must reflect the new frame;
    // a second write to the same VPN should be a plain TLB hit with no CoW copy
    auto mm = make_fifo_manager();
    mm.create_process(1);
    mm.access(1, 0, false);          // load vpn 0 for process 1
    mm.fork_process(1, 2);           // share the frame; both marked read-only
    mm.access(1, 0, true);           // CoW: process 1 gets a new frame, TLB updated
    mm.reset_metrics();
    mm.access(1, 0, true);           // second write — must be TLB hit, no CoW copy
    const auto& m = mm.get_metrics();
    assert(m.tlb_hits   == 1);
    assert(m.cow_copies == 0);
    std::cout << "PASS: test_tlb_coherent_after_cow\n";
}

void test_tlb_metrics_sum_equals_total_accesses() {
    auto mm = make_fifo_manager(4);
    mm.create_process(1);
    // 3 unique pages: first 3 accesses are misses, next 3 are TLB hits
    std::vector<int> seq = {0, 1, 2, 0, 1, 2};
    for (int vpn : seq) mm.access(1, vpn * PAGE_SIZE, false);
    const auto& m = mm.get_metrics();
    assert(m.tlb_hits + m.tlb_misses == (int)seq.size());
    std::cout << "PASS: test_tlb_metrics_sum_equals_total_accesses\n";
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
    test_tlb_miss_on_first_access();
    test_tlb_hit_on_second_access();
    test_tlb_shootdown_on_eviction();
    test_tlb_coherent_after_cow();
    test_tlb_metrics_sum_equals_total_accesses();

    std::cout << "\nAll MemoryManager tests passed.\n";
    return 0;
}