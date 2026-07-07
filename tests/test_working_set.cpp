#include <cassert>
#include <iostream>
#include "../src/policy/working_set.h"
#include "../src/policy/fifo.h"
#include "../src/memory_manager.h"
#include "../src/constants.h"

void test_working_set_size_within_window() {
    WorkingSetPolicy ws(4);
    ws.on_load(0, 0);
    ws.on_load(1, 1);
    ws.on_load(2, 2);

    assert(ws.working_set_size() == 3);
    assert(ws.expired_frames().empty());

    std::cout << "PASS: test_working_set_size_within_window\n";
}

void test_frames_expire_outside_window() {
    WorkingSetPolicy ws(3);
    ws.on_load(0, 0);              // t=1
    ws.on_load(1, 1);              // t=2
    for (int i = 0; i < 3; i++)
        ws.on_access(1);           // t=3,4,5 — frame 0 ages out

    auto expired = ws.expired_frames();
    assert(expired.size() == 1);
    assert(expired[0] == 0);
    assert(ws.working_set_size() == 1);

    std::cout << "PASS: test_frames_expire_outside_window\n";
}

void test_evict_picks_oldest() {
    WorkingSetPolicy ws(2);
    ws.on_load(0, 0);
    ws.on_load(1, 1);
    ws.on_access(0); // frame 1 is now oldest

    assert(ws.evict() == 1);
    assert(ws.evict() == 0);

    bool threw = false;
    try { ws.evict(); } catch (const std::runtime_error&) { threw = true; }
    assert(threw);

    std::cout << "PASS: test_evict_picks_oldest\n";
}

void test_reaccess_keeps_frame_in_working_set() {
    WorkingSetPolicy ws(3);
    ws.on_load(0, 0);
    ws.on_load(1, 1);
    for (int i = 0; i < 4; i++) {
        ws.on_access(0);
        ws.on_access(1);
    }

    assert(ws.expired_frames().empty());
    assert(ws.working_set_size() == 2);

    std::cout << "PASS: test_reaccess_keeps_frame_in_working_set\n";
}

void test_trim_frees_expired_frames() {
    MemoryManager mm(new WorkingSetPolicy(4), 8);
    mm.create_process(1);

    mm.access(1, 0 * PAGE_SIZE, false);
    mm.access(1, 1 * PAGE_SIZE, false);
    for (int i = 0; i < 4; i++)
        mm.access(1, 2 * PAGE_SIZE, false); // pages 0 and 1 age out

    int trimmed = mm.trim_working_set();
    assert(trimmed == 2);
    assert(mm.get_metrics().ws_trims == 2);

    // trimmed pages fault again even though frames were never exhausted
    int faults_before = mm.get_metrics().page_faults;
    mm.access(1, 0 * PAGE_SIZE, false);
    assert(mm.get_metrics().page_faults == faults_before + 1);

    std::cout << "PASS: test_trim_frees_expired_frames\n";
}

void test_trim_noop_when_all_in_window() {
    MemoryManager mm(new WorkingSetPolicy(16), 8);
    mm.create_process(1);
    mm.access(1, 0 * PAGE_SIZE, false);
    mm.access(1, 1 * PAGE_SIZE, false);

    assert(mm.trim_working_set() == 0);
    assert(mm.get_metrics().ws_trims == 0);

    std::cout << "PASS: test_trim_noop_when_all_in_window\n";
}

void test_trim_noop_with_other_policy() {
    MemoryManager mm(new FIFO(), 4);
    mm.create_process(1);
    for (int vpn = 0; vpn < 8; vpn++)
        mm.access(1, vpn * PAGE_SIZE, false);

    assert(mm.trim_working_set() == 0);

    std::cout << "PASS: test_trim_noop_with_other_policy\n";
}

void test_eviction_under_pressure_prefers_expired() {
    MemoryManager mm(new WorkingSetPolicy(2), 2);
    mm.create_process(1);

    mm.access(1, 0 * PAGE_SIZE, false);
    mm.access(1, 1 * PAGE_SIZE, false);
    mm.access(1, 1 * PAGE_SIZE, false); // page 0 is oldest
    mm.access(1, 2 * PAGE_SIZE, false); // evicts page 0's frame

    assert(mm.get_metrics().evictions == 1);
    int faults_before = mm.get_metrics().page_faults;
    mm.access(1, 1 * PAGE_SIZE, false); // still resident
    assert(mm.get_metrics().page_faults == faults_before);

    std::cout << "PASS: test_eviction_under_pressure_prefers_expired\n";
}

int main() {
    test_working_set_size_within_window();
    test_frames_expire_outside_window();
    test_evict_picks_oldest();
    test_reaccess_keeps_frame_in_working_set();
    test_trim_frees_expired_frames();
    test_trim_noop_when_all_in_window();
    test_trim_noop_with_other_policy();
    test_eviction_under_pressure_prefers_expired();

    std::cout << "\nAll WorkingSetPolicy tests passed.\n";
    return 0;
}
