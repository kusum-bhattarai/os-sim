#include "page_table_hashed.h"
#include "constants.h"

void HashedPageTable::insert(int vpn, int frame_index) {
    auto& chain = buckets[bucket_index(vpn)];
    for (auto& node : chain) {
        if (node.vpn == vpn) {
            node.entry = PageTableEntry{};
            node.entry.valid = true;
            node.entry.frame_index = frame_index;
            return;
        }
    }
    chain.emplace_front(vpn);
    chain.front().entry.valid = true;
    chain.front().entry.frame_index = frame_index;
}

PageTableEntry* HashedPageTable::lookup(int vpn) {
    auto& chain = buckets[bucket_index(vpn)];
    lookups++;
    for (auto& node : chain) {
        probes++;
        if (node.vpn == vpn) {
            return &node.entry;
        }
    }
    return nullptr;
}

void HashedPageTable::invalidate(int vpn) {
    PageTableEntry* entry = lookup(vpn);
    if (entry != nullptr) {
        entry->valid = false;
    }
}

int HashedPageTable::translate(int virtual_address) {
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

std::unordered_map<int, PageTableEntry> HashedPageTable::get_entries() const {
    std::unordered_map<int, PageTableEntry> result;
    for (int b = 0; b < HASHED_BUCKETS; b++) {
        for (const auto& node : buckets[b]) {
            result[node.vpn] = node.entry;
        }
    }
    return result;
}
