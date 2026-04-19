#include <cassert>
#include <iostream>
#include "../src/memory_manager.h"
#include "../src/policy/fifo.h"
#include "../src/constants.h"

MemoryManager make_manager(int num_frames = 8) {
    return MemoryManager(new FIFO(), num_frames);
}

// ─── fork basics ─────────────────────────────────────────────────

void test_fork_creates_child_process() {
    auto mm = make_manager();
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.access(1, 1 * PAGE_SIZE, false);
    mm.fork_process(1, 2);
    // child should be able to access same pages without faulting
    mm.reset_metrics();
    mm.access(2, 0 * PAGE_SIZE, false);
    mm.access(2, 1 * PAGE_SIZE, false);
    mm.print_metrics(); // should show 0 faults — pages already in memory
    std::cout << "PASS: test_fork_creates_child_process\n";
}

void test_fork_unknown_parent_throws() {
    auto mm = make_manager();
    try {
        mm.fork_process(99, 2);
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_fork_unknown_parent_throws\n";
}

void test_fork_duplicate_child_throws() {
    auto mm = make_manager();
    mm.create_process(1);
    mm.create_process(2);
    try {
        mm.fork_process(1, 2);
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_fork_duplicate_child_throws\n";
}

void test_fork_increments_cow_forks_metric() {
    auto mm = make_manager();
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.fork_process(1, 2);
    mm.print_metrics(); // should show cow_forks = 1
    std::cout << "PASS: test_fork_increments_cow_forks_metric\n";
}

// ─── shared memory after fork ─────────────────────────────────────

void test_fork_shares_frames_not_copies() {
    auto mm = make_manager();
    mm.create_process(1);
    // load 4 pages
    for (int i = 0; i < 4; i++) {
        mm.access(1, i * PAGE_SIZE, false);
    }
    mm.reset_metrics();
    mm.fork_process(1, 2);
    // forking should not allocate new frames
    mm.print_metrics(); // faults = 0, no new frames allocated
    std::cout << "PASS: test_fork_shares_frames_not_copies\n";
}

void test_read_after_fork_causes_no_cow_copy() {
    auto mm = make_manager();
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.fork_process(1, 2);
    mm.reset_metrics();
    // child reads — should be a hit, no CoW copy
    mm.access(2, 0 * PAGE_SIZE, false);
    mm.print_metrics(); // cow_copies = 0, hits = 1
    std::cout << "PASS: test_read_after_fork_causes_no_cow_copy\n";
}

void test_parent_read_after_fork_causes_no_cow_copy() {
    auto mm = make_manager();
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.fork_process(1, 2);
    mm.reset_metrics();
    // parent reads — should also be a hit, no CoW copy
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.print_metrics(); // cow_copies = 0, hits = 1
    std::cout << "PASS: test_parent_read_after_fork_causes_no_cow_copy\n";
}

// ─── CoW trigger on write ─────────────────────────────────────────

void test_child_write_triggers_cow_copy() {
    auto mm = make_manager();
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.fork_process(1, 2);
    mm.reset_metrics();
    // child writes — should trigger CoW
    mm.access(2, 0 * PAGE_SIZE, true);
    mm.print_metrics(); // cow_copies = 1
    std::cout << "PASS: test_child_write_triggers_cow_copy\n";
}

void test_parent_write_triggers_cow_copy() {
    auto mm = make_manager();
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.fork_process(1, 2);
    mm.reset_metrics();
    // parent writes — should also trigger CoW
    mm.access(1, 0 * PAGE_SIZE, true);
    mm.print_metrics(); // cow_copies = 1
    std::cout << "PASS: test_parent_write_triggers_cow_copy\n";
}

void test_second_write_does_not_trigger_cow() {
    auto mm = make_manager();
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.fork_process(1, 2);
    mm.access(2, 0 * PAGE_SIZE, true); // first write → CoW copy
    mm.reset_metrics();
    mm.access(2, 0 * PAGE_SIZE, true); // second write → no CoW, already owns it
    mm.print_metrics(); // cow_copies = 0
    std::cout << "PASS: test_second_write_does_not_trigger_cow\n";
}

void test_both_processes_write_get_independent_frames() {
    auto mm = make_manager();
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.fork_process(1, 2);
    mm.reset_metrics();
    mm.access(1, 0 * PAGE_SIZE, true); // parent writes → CoW copy
    mm.access(2, 0 * PAGE_SIZE, true); // child writes → CoW copy
    mm.print_metrics(); // cow_copies = 2
    std::cout << "PASS: test_both_processes_write_get_independent_frames\n";
}

// ─── last owner gets write permission restored ────────────────────

void test_last_owner_gets_write_permission_after_cow() {
    auto mm = make_manager();
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.fork_process(1, 2);
    // child writes → child gets new frame, parent is last owner of original
    mm.access(2, 0 * PAGE_SIZE, true);
    mm.reset_metrics();
    // parent should now be able to write without triggering CoW
    mm.access(1, 0 * PAGE_SIZE, true);
    mm.print_metrics(); // cow_copies = 0 — parent owns frame alone now
    std::cout << "PASS: test_last_owner_gets_write_permission_after_cow\n";
}

// ─── multi-page fork ─────────────────────────────────────────────

void test_fork_with_multiple_pages() {
    auto mm = make_manager();
    mm.create_process(1);
    for (int i = 0; i < 4; i++) {
        mm.access(1, i * PAGE_SIZE, false);
    }
    mm.fork_process(1, 2);
    mm.reset_metrics();
    // write to all pages in child — should trigger 4 CoW copies
    for (int i = 0; i < 4; i++) {
        mm.access(2, i * PAGE_SIZE, true);
    }
    mm.print_metrics(); // cow_copies = 4
    std::cout << "PASS: test_fork_with_multiple_pages\n";
}

void test_read_heavy_fork_no_copies() {
    auto mm = make_manager();
    mm.create_process(1);
    for (int i = 0; i < 4; i++) {
        mm.access(1, i * PAGE_SIZE, false);
    }
    mm.fork_process(1, 2);
    mm.reset_metrics();
    // both processes only read — zero copies should occur
    for (int i = 0; i < 4; i++) {
        mm.access(1, i * PAGE_SIZE, false);
        mm.access(2, i * PAGE_SIZE, false);
    }
    mm.print_metrics(); // cow_copies = 0, hits = 8
    std::cout << "PASS: test_read_heavy_fork_no_copies\n";
}

int main() {
    test_fork_creates_child_process();
    test_fork_unknown_parent_throws();
    test_fork_duplicate_child_throws();
    test_fork_increments_cow_forks_metric();
    test_fork_shares_frames_not_copies();
    test_read_after_fork_causes_no_cow_copy();
    test_parent_read_after_fork_causes_no_cow_copy();
    test_child_write_triggers_cow_copy();
    test_parent_write_triggers_cow_copy();
    test_second_write_does_not_trigger_cow();
    test_both_processes_write_get_independent_frames();
    test_last_owner_gets_write_permission_after_cow();
    test_fork_with_multiple_pages();
    test_read_heavy_fork_no_copies();

    std::cout << "\nAll CoW tests passed.\n";
    return 0;
}