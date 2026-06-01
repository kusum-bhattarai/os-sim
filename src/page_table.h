#pragma once
#include <unordered_map>
#include "page_table_iface.h"
#include "constants.h"

class FlatPageTable : public IPageTable {
private:
    std::unordered_map<int, PageTableEntry> entries;
public:
    FlatPageTable() = default;
    void insert(int vpn, int frame_index) override;
    PageTableEntry* lookup(int vpn) override;
    void invalidate(int vpn) override;
    int translate(int virtual_address) override;
    std::unordered_map<int, PageTableEntry> get_entries() const override { return entries; }
};
