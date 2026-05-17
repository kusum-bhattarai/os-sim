#include <cassert>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "../src/policy/clock.h"

// ─── eviction ────────────────────────────────────────────────────────

void test_clock_evict_empty_throws() {
    Clock clock;
    try {
        clock.evict();
        assert(false && "should have thrown");
    } catch (const std::runtime_error&) {}
    std::cout << "PASS: test_clock_evict_empty_throws\n";
}

void test_clock_second_chance() {
    // On load, the reference bit is set. The first evict() sweeps through
    // clearing all bits before evicting, giving each frame one second chance.
    // A frame reloaded after eviction has its bit set again and is skipped
    // on the next evict — the next unreferenced frame is taken instead.
    Clock clock;
    clock.on_load(0, 1);
    clock.on_load(1, 2);
    clock.on_load(2, 3);

    assert(clock.evict() == 0); // full sweep clears all bits, evicts frame 0

    clock.on_load(0, 4);        // frame 0 reloaded: reference bit set again
    assert(clock.evict() == 1); // hand at 1, frame 1 has no reference bit → evicted
                                // frame 0 survives its second chance
    std::cout << "PASS: test_clock_second_chance\n";
}

void test_clock_access_protects_frame() {
    // on_access sets the reference bit. A frame that is accessed after the
    // sweep clears its bit gets a new second chance on the next eviction.
    Clock clock;
    clock.on_load(0, 1);
    clock.on_load(1, 2);
    clock.on_load(2, 3);

    clock.evict();              // sweep clears all bits, evicts 0, hand → 1
    clock.on_load(0, 4);        // frame 0 reloaded: ref=true
    clock.on_access(1);         // frame 1 referenced again after its bit was cleared

    // hand=1: frame 1 ref=true → cleared, advance
    // hand=2: frame 2 ref=false → evicted
    assert(clock.evict() == 2);
    std::cout << "PASS: test_clock_access_protects_frame\n";
}

void test_clock_unaccessed_frame_eventually_evicted() {
    // A frame that is never accessed again loses its reference bit on the
    // first sweep and is evicted on the second pass through that position.
    Clock clock;
    clock.on_load(0, 1);
    clock.on_load(1, 2);
    clock.on_load(2, 3);

    clock.evict();              // sweep, evicts 0, hand → 1; frames 1,2 cleared
    clock.on_load(0, 4);        // reload frame 0: ref=true
    clock.evict();              // frame 1 ref=false → evicted, hand → 2
    clock.on_load(1, 5);        // reload frame 1: ref=true
    clock.evict();              // frame 2 ref=false → evicted, hand → 0
    clock.on_load(2, 6);        // reload frame 2: ref=true

    // All three frames now have ref=true (from reload). A full sweep clears
    // all, then evicts frame 0 (hand wraps back to it first).
    assert(clock.evict() == 0);
    std::cout << "PASS: test_clock_unaccessed_frame_eventually_evicted\n";
}

// ─── reset ───────────────────────────────────────────────────────────

void test_clock_reset_clears_state() {
    Clock clock;
    clock.on_load(0, 1);
    clock.on_load(1, 2);
    clock.reset();
    try {
        clock.evict();
        assert(false && "should have thrown after reset");
    } catch (const std::runtime_error&) {}
    // usable again after reset
    clock.on_load(0, 3);
    clock.evict();
    std::cout << "PASS: test_clock_reset_clears_state\n";
}

// ─── fault count ─────────────────────────────────────────────────────

void test_clock_fault_count_known_sequence() {
    // [1,2,3,4,1,2,5,1,2,3,4,5] with 3 frames → 9 faults
    std::vector<int> sequence = {1,2,3,4,1,2,5,1,2,3,4,5};
    Clock clock;
    std::unordered_map<int,int> page_to_frame, frame_to_page;
    int next_frame = 0, faults = 0;

    for (int page : sequence) {
        if (page_to_frame.find(page) == page_to_frame.end()) {
            faults++;
            if (next_frame < 3) {
                page_to_frame[page] = next_frame;
                frame_to_page[next_frame] = page;
                clock.on_load(next_frame++, page);
            } else {
                int vf = clock.evict();
                page_to_frame.erase(frame_to_page[vf]);
                page_to_frame[page] = vf;
                frame_to_page[vf] = page;
                clock.on_load(vf, page);
            }
        } else {
            clock.on_access(page_to_frame[page]);
        }
    }
    assert(faults == 9);
    std::cout << "PASS: test_clock_fault_count_known_sequence\n";
}

int main() {
    test_clock_evict_empty_throws();
    test_clock_second_chance();
    test_clock_access_protects_frame();
    test_clock_unaccessed_frame_eventually_evicted();
    test_clock_reset_clears_state();
    test_clock_fault_count_known_sequence();

    std::cout << "\nAll CLOCK tests passed.\n";
    return 0;
}
