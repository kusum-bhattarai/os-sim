#include "tlb.h"

int TLB::lookup(int vpn) {
    for (auto& entry : entries) {
        if (entry.valid && entry.vpn == vpn) {
            entry.lru_tick = tick++;  // update LRU info on hit
            return entry.frame_index;
        }
    }
    return -1; // miss
}

void TLB::insert(int vpn, int frame_index, bool writable){
    // check for a valid entry first
    for (auto& entry: entries) {
        if (!entry.valid) {
            entry = {true, vpn, frame_index, writable, tick++};
            return;
        } else if (entry.vpn == vpn) { // overwrite existing entry for same VPN
            entry.frame_index = frame_index;
            entry.writable = writable;
            entry.lru_tick = tick++;
            return;
        }

        // if TLB is full, evict the LRU entry
        auto lru_entry = std::min_element(entries.begin(), entries.end(), [](const TLBEntry& a, const TLBEntry& b) {
            return a.lru_tick < b.lru_tick;
        });
        *lru_entry = {true, vpn, frame_index, writable, tick++};
    }
}

void TLB::invalidate(int vpn) {
    for (auto& entry : entries) {
        if (entry.valid && entry.vpn == vpn) {
            entry.valid = false;
            return;
        }
    }
}

void TLB::flush() {
    for (auto& entry : entries) {
        entry.valid = false;
    }
}