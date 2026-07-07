#pragma once
#include <unordered_map>

struct PageTableEntry {
    bool valid;
    bool dirty;
    bool referenced;
    bool writable;
    int frame_index;
    PageTableEntry() : valid(false), dirty(false), referenced(false), writable(false), frame_index(-1) {}
};

enum class PageTableType { FLAT, TWO_LEVEL, HASHED, INVERTED };

class IPageTable {
public:
    virtual ~IPageTable() = default;
    virtual void insert(int vpn, int frame_index) = 0;
    virtual PageTableEntry* lookup(int vpn) = 0;
    virtual void invalidate(int vpn) = 0;
    virtual int translate(int virtual_address) = 0;
    virtual std::unordered_map<int, PageTableEntry> get_entries() const = 0;
};
