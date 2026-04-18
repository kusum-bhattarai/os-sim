#include <cassert>
#include <iostream>
#include "../src/policy/fifo.h"

void test_fifo_basic_eviction_order() {
    FIFO fifo;
    fifo.on_load(0, 1);
    fifo.on_load(1, 2);
    fifo.on_load(2, 3);
    assert(fifo.evict() == 0); // first loaded is first evicted
    std::cout << "PASS: test_fifo_basic_eviction_order\n";
}

void test_fifo_evict_after_reload() {
    FIFO fifo;
    fifo.on_load(0, 1);
    fifo.on_load(1, 2);
    fifo.on_load(2, 3);
    fifo.evict();           // evicts frame 0
    fifo.on_load(0, 4);    // reload frame 0 with new page
    assert(fifo.evict() == 1); // frame 1 is now oldest
    std::cout << "PASS: test_fifo_evict_after_reload\n";
}

void test_fifo_access_does_not_affect_order() {
    FIFO fifo;
    fifo.on_load(0, 1);
    fifo.on_load(1, 2);
    fifo.on_load(2, 3);
    fifo.on_access(0);     // accessing frame 0 should not change eviction order
    assert(fifo.evict() == 0);
    std::cout << "PASS: test_fifo_access_does_not_affect_order\n";
}

void test_fifo_evict_empty_throws() {
    FIFO fifo;
    try {
        fifo.evict();
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_fifo_evict_empty_throws\n";
}

void test_fifo_reset_clears_state() {
    FIFO fifo;
    fifo.on_load(0, 1);
    fifo.on_load(1, 2);
    fifo.reset();
    try {
        fifo.evict();
        assert(false && "should have thrown after reset");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_fifo_reset_clears_state\n";
}

void test_fifo_fault_count_known_sequence() {
    // sequence: [1,2,3,4,1,2,5,1,2,3,4,5], 3 frames
    // expected faults: 9
    std::vector<int> sequence = {1,2,3,4,1,2,5,1,2,3,4,5};
    FIFO fifo;
    std::unordered_map<int, int> page_to_frame;
    std::unordered_map<int, int> frame_to_page;
    int next_frame = 0;
    int faults = 0;

    for (int page : sequence) {
        if (page_to_frame.find(page) == page_to_frame.end()) {
            faults++;
            if (next_frame < 3) {
                page_to_frame[page] = next_frame;
                frame_to_page[next_frame] = page;
                fifo.on_load(next_frame, page);
                next_frame++;
            } else {
                int victim_frame = fifo.evict();
                int evicted_page = frame_to_page[victim_frame];
                page_to_frame.erase(evicted_page);
                page_to_frame[page] = victim_frame;
                frame_to_page[victim_frame] = page;
                fifo.on_load(victim_frame, page);
            }
        } else {
            fifo.on_access(page_to_frame[page]);
        }
    }
    assert(faults == 9);
    std::cout << "PASS: test_fifo_fault_count_known_sequence\n";
}

int main() {
    test_fifo_basic_eviction_order();
    test_fifo_evict_after_reload();
    test_fifo_access_does_not_affect_order();
    test_fifo_evict_empty_throws();
    test_fifo_reset_clears_state();
    test_fifo_fault_count_known_sequence();

    std::cout << "\nAll FIFO tests passed.\n";
    return 0;
}