#pragma once
#include <array>
#include "constants.h"

struct TLBEntry {
    bool valid       = false;
    int  vpn         = -1;
    int  frame_index = -1;
    bool writable    = false;
    int  lru_tick    = 0;   // for LRU eviction within TLB
};

class TLB {
    int tick = 0;          // global access counter for LRU
    std::array<TLBEntry, TLB_SIZE> entries;
public:
    // returns frame_index on hit, -1 on miss
    int lookup(int vpn);

    // inserts or overwrites; evicts LRU entry if full
    void insert(int vpn, int frame_index, bool writable);

    // invalidates a single VPN (e.g., TLB shootdown)
    void invalidate(int vpn);

    // wipes all entries (e.g., context switch without ASID)
    void flush();
};