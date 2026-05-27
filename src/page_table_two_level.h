#pragma once
#include <array>
#include <memory>
#include <unordered_map>
#include "page_table_iface.h"

// 10-bit VPN split as 5+5: bits 9-5 → L1 index, bits 4-0 → L2 index
static constexpr int L1_BITS = 5;
static constexpr int L2_BITS = 5;
static constexpr int L1_SIZE = 1 << L1_BITS;
static constexpr int L2_SIZE = 1 << L2_BITS;

struct L2Table {
    std::array<PageTableEntry, L2_SIZE> entries;
    std::array<bool, L2_SIZE> present;
    L2Table() : present{} {}
};

class TwoLevelPageTable : public IPageTable {
    std::array<std::unique_ptr<L2Table>, L1_SIZE> l1;
public:
    void insert(int vpn, int frame_index) override;
    PageTableEntry* lookup(int vpn) override;
    void invalidate(int vpn) override;
    int translate(int virtual_address) override;
    std::unordered_map<int, PageTableEntry> get_entries() const override;

    int l2_table_count() const {
        int count = 0;
        for (const auto& slot : l1) if (slot != nullptr) count++;
        return count;
    }

private:
    static int l1_index(int vpn) { return vpn >> L2_BITS; }
    static int l2_index(int vpn) { return vpn & (L2_SIZE - 1); }
};
