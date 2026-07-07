#include "page_table_inverted.h"

InvertedPageTable::InvertedPageTable(int num_frames) : table(num_frames) {}

void InvertedPageTable::insert(int pid, int vpn, int frame_index) {
    InvertedEntry& slot = table.at(frame_index);
    if (slot.pid != -1) {
        index.erase(key(slot.pid, slot.vpn)); // evict the previous occupant's mapping
    }
    auto it = index.find(key(pid, vpn));
    if (it != index.end() && it->second != frame_index) {
        table[it->second] = InvertedEntry{}; // (pid, vpn) moved frames — clear the old slot
    }
    slot = InvertedEntry{};
    slot.pid = pid;
    slot.vpn = vpn;
    slot.entry.valid = true;
    slot.entry.frame_index = frame_index;
    index[key(pid, vpn)] = frame_index;
}

PageTableEntry* InvertedPageTable::lookup(int pid, int vpn) {
    lookup_count++;
    indexed_probes++;
    auto it = index.find(key(pid, vpn));
    if (it == index.end()) {
        return nullptr;
    }
    return &table[it->second].entry;
}

PageTableEntry* InvertedPageTable::linear_lookup(int pid, int vpn) {
    for (auto& slot : table) {
        linear_probes++;
        if (slot.pid == pid && slot.vpn == vpn && slot.entry.valid) {
            return &slot.entry;
        }
    }
    return nullptr;
}

void InvertedPageTable::invalidate(int pid, int vpn) {
    auto it = index.find(key(pid, vpn));
    if (it == index.end()) {
        return;
    }
    table[it->second].entry.valid = false;
    index.erase(it);
}

int InvertedPageTable::translate(int pid, int virtual_address) {
    if (virtual_address < 0 || static_cast<size_t>(virtual_address) >= VIRTUAL_ADDRESS_SPACE_SIZE) {
        return -1;
    }
    int vpn = virtual_address / PAGE_SIZE;
    int offset = virtual_address % PAGE_SIZE;
    PageTableEntry* entry = lookup(pid, vpn);
    if (entry != nullptr && entry->valid) {
        return entry->frame_index * PAGE_SIZE + offset;
    }
    return -2;
}

std::unordered_map<int, PageTableEntry> InvertedPageTable::entries_for(int pid) const {
    std::unordered_map<int, PageTableEntry> result;
    for (const auto& slot : table) {
        if (slot.pid == pid) {
            result[slot.vpn] = slot.entry;
        }
    }
    return result;
}
