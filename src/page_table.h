#pragma once
#include <array>
#include <unordered_map>
#include "constants.h"

struct PageTableEntry{
    bool valid;
    bool dirty;
    bool referenced;
    bool writable;
    int frame_index;
    PageTableEntry() : valid(false), dirty(false), referenced(false), writable(false), frame_index(-1) {}
};

class PageTable {
private:
    std::unordered_map<int, PageTableEntry> entries; // key: virtual page number
public:
    PageTable() = default;
    // add or update an entry
    void insert(int vpn, int frame_index);

    // to look for an entry by vpn
    PageTableEntry* lookup(int vpn);

    // to make an entry invalid (e.g. on page eviction)
    void invalidate(int vpn);

    // translate a virtual address to a physical address, returns -1 if not valid
    int translate(int virtual_address);
};