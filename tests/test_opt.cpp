#include <cassert>
#include <iostream>
#include <unordered_map>
#include "../src/policy/opt.h"

void test_opt_evicts_farthest_next_use() {
    std::vector<int> seq = {1, 2, 3, 1};
    OPT opt(seq);
    opt.on_load(0, 1); opt.on_access(0); // access 1
    opt.on_load(1, 2); opt.on_access(1); // access 2
    opt.on_load(2, 3); opt.on_access(2); // access 3
    // next accesses: 1 is used at pos 3, 2 and 3 never again
    // should evict frame with vpn 2 or 3 (never used) — frame 1 or 2
    int victim = opt.evict();
    assert(victim == 1 || victim == 2); // either never-used page
    std::cout << "PASS: test_opt_evicts_farthest_next_use\n";
}

void test_opt_evict_empty_throws() {
    std::vector<int> seq = {1, 2};
    OPT opt(seq);
    try {
        opt.evict();
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_opt_evict_empty_throws\n";
}

void test_opt_reset_clears_state() {
    std::vector<int> seq = {1, 2, 3};
    OPT opt(seq);
    opt.on_load(0, 1);
    opt.reset();
    try {
        opt.evict();
        assert(false && "should have thrown after reset");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_opt_reset_clears_state\n";
}

void test_opt_fault_count_known_sequence() {
    // sequence: [1,2,3,4,1,2,5,1,2,3,4,5], 3 frames
    // expected faults: 7
    std::vector<int> sequence = {1,2,3,4,1,2,5,1,2,3,4,5};
    OPT opt(sequence);
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
                opt.on_load(next_frame, page);
                next_frame++;
            } else {
                int victim_frame = opt.evict();
                int evicted_page = frame_to_page[victim_frame];
                page_to_frame.erase(evicted_page);
                page_to_frame[page] = victim_frame;
                frame_to_page[victim_frame] = page;
                opt.on_load(victim_frame, page);
            }
        }
        opt.on_access(page_to_frame[page]); // advance curr_pos once per reference
    }
    assert(faults == 7);
    std::cout << "PASS: test_opt_fault_count_known_sequence\n";
}

int main() {
    test_opt_evicts_farthest_next_use();
    test_opt_evict_empty_throws();
    test_opt_reset_clears_state();
    test_opt_fault_count_known_sequence();

    std::cout << "\nAll OPT tests passed.\n";
    return 0;
}