#include <cassert>
#include <iostream>
#include <unordered_map>
#include "../src/policy/lru.h"

void test_lru_evicts_least_recently_used() {
    LRU lru;
    lru.on_load(0, 1);
    lru.on_load(1, 2);
    lru.on_load(2, 3);
    lru.on_access(0);      // 0 is now most recently used
    lru.on_access(1);      // 1 is now most recently used
    assert(lru.evict() == 2); // 2 was least recently used
    std::cout << "PASS: test_lru_evicts_least_recently_used\n";
}

void test_lru_access_updates_order() {
    LRU lru;
    lru.on_load(0, 1);
    lru.on_load(1, 2);
    lru.on_load(2, 3);
    lru.on_access(0);      // make 0 most recent
    assert(lru.evict() == 1); // 1 is now LRU
    std::cout << "PASS: test_lru_access_updates_order\n";
}

void test_lru_evict_empty_throws() {
    LRU lru;
    try {
        lru.evict();
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_lru_evict_empty_throws\n";
}

void test_lru_access_unknown_frame_throws() {
    LRU lru;
    try {
        lru.on_access(99);
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_lru_access_unknown_frame_throws\n";
}

void test_lru_reset_clears_state() {
    LRU lru;
    lru.on_load(0, 1);
    lru.on_load(1, 2);
    lru.reset();
    try {
        lru.evict();
        assert(false && "should have thrown after reset");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_lru_reset_clears_state\n";
}

void test_lru_fault_count_known_sequence() {
    // sequence: [1,2,3,4,1,2,5,1,2,3,4,5], 3 frames
    // expected faults: 8
    std::vector<int> sequence = {1,2,3,4,1,2,5,1,2,3,4,5};
    LRU lru;
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
                lru.on_load(next_frame, page);
                next_frame++;
            } else {
                int victim_frame = lru.evict();
                int evicted_page = frame_to_page[victim_frame];
                page_to_frame.erase(evicted_page);
                page_to_frame[page] = victim_frame;
                frame_to_page[victim_frame] = page;
                lru.on_load(victim_frame, page);
            }
        } else {
            lru.on_access(page_to_frame[page]);
        }
    }
    assert(faults == 10);
    std::cout << "PASS: test_lru_fault_count_known_sequence\n";
}

int main() {
    test_lru_evicts_least_recently_used();
    test_lru_access_updates_order();
    test_lru_evict_empty_throws();
    test_lru_access_unknown_frame_throws();
    test_lru_reset_clears_state();
    test_lru_fault_count_known_sequence();

    std::cout << "\nAll LRU tests passed.\n";
    return 0;
}