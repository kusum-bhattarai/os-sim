#include "page_table.h"

void PageTable::insert(int vpn, int frame_index){
    entries[vpn] = PageTableEntry{};
    entries[vpn].valid = true;
    entries[vpn].frame_index = frame_index;
}

PageTableEntry* PageTable::lookup(int vpn){
    auto it = entries.find(vpn);
    if (it != entries.end()){
        return &it->second;
    }
    return nullptr;
}

void PageTable::invalidate(int vpn){
    auto it = entries.find(vpn);
    if (it != entries.end()){
        it->second.valid = false;
    }
}

int PageTable::translate(int virtual_address){
    if (virtual_address < 0 || static_cast<size_t>(virtual_address) >= VIRTUAL_ADDRESS_SPACE_SIZE) {
        return -1; // signal invalid address (out of bounds)
    }
    int vpn = virtual_address / PAGE_SIZE;
    int offset = virtual_address % PAGE_SIZE;
    auto it = entries.find(vpn);
    if (it != entries.end() && it->second.valid){
        return it->second.frame_index * PAGE_SIZE + offset;
    }
    return -2; // page fault (missing or invalid page)
}