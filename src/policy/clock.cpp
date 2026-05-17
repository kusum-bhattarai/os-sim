#include "clock.h"
#include <stdexcept>

void Clock::on_load(int frame_index, int vpn) {
    if (frame_to_index.count(frame_index) == 0) {
        frames.push_back({frame_index, true});
        frame_to_index[frame_index] = frames.size() - 1;
    } else {
        size_t idx = frame_to_index[frame_index];
        frames[idx].referenced = true;
    }
}

void Clock::on_access(int frame_index) {
    if (frame_to_index.count(frame_index) > 0) {
        size_t idx = frame_to_index[frame_index];
        frames[idx].referenced = true;
    }
}

int Clock::evict() {
    if (frames.empty()) {
        throw std::runtime_error("No frames to evict");
    }
    while (true) {
        Entry& entry = frames[hand];
        if (!entry.referenced) {
            int victim_frame = entry.frame_index;
            hand = (hand + 1) % frames.size();
            return victim_frame;
            // slot stays in place — on_load will reuse it via frame_to_index
        } else {
            entry.referenced = false; // second chance
            hand = (hand + 1) % frames.size();
        }
    }
}

void Clock::reset() {
    frames.clear();
    frame_to_index.clear();
    hand = 0;
}