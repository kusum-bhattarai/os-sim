#include "page_table_two_level.h"
#include "constants.h"

void TwoLevelPageTable::insert(int vpn, int frame_index) {
    int i1 = l1_index(vpn);
    int i2 = l2_index(vpn);
    if (l1[i1] == nullptr) {
        l1[i1] = std::make_unique<L2Table>();
    }
    l1[i1]->entries[i2] = PageTableEntry{};
    l1[i1]->entries[i2].valid = true;
    l1[i1]->entries[i2].frame_index = frame_index;
    l1[i1]->present[i2] = true;
}

PageTableEntry* TwoLevelPageTable::lookup(int vpn) {
    int i1 = l1_index(vpn);
    int i2 = l2_index(vpn);
    if (l1[i1] == nullptr || !l1[i1]->present[i2]) {
        return nullptr;
    }
    return &l1[i1]->entries[i2];
}

void TwoLevelPageTable::invalidate(int vpn) {
    int i1 = l1_index(vpn);
    int i2 = l2_index(vpn);
    if (l1[i1] == nullptr || !l1[i1]->present[i2]) {
        return;
    }
    l1[i1]->entries[i2].valid = false;
}

int TwoLevelPageTable::translate(int virtual_address) {
    if (virtual_address < 0 || static_cast<size_t>(virtual_address) >= VIRTUAL_ADDRESS_SPACE_SIZE) {
        return -1;
    }
    int vpn = virtual_address / PAGE_SIZE;
    int offset = virtual_address % PAGE_SIZE;
    PageTableEntry* entry = lookup(vpn);
    if (entry != nullptr && entry->valid) {
        return entry->frame_index * PAGE_SIZE + offset;
    }
    return -2;
}

std::unordered_map<int, PageTableEntry> TwoLevelPageTable::get_entries() const {
    std::unordered_map<int, PageTableEntry> result;
    for (int i1 = 0; i1 < L1_SIZE; i1++) {
        if (l1[i1] == nullptr) continue;
        for (int i2 = 0; i2 < L2_SIZE; i2++) {
            if (l1[i1]->present[i2]) {
                int vpn = (i1 << L2_BITS) | i2;
                result[vpn] = l1[i1]->entries[i2];
            }
        }
    }
    return result;
}
