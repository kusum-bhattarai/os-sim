#include "memory_manager.h"
#include <algorithm>
#include <stdexcept>

const Process& MemoryManager::get_process(int pid) const {
    auto it = processes.find(pid);
    if (it == processes.end()) {
        throw std::runtime_error("Process does not exist");
    }
    return *it->second;
}

std::vector<int> MemoryManager::get_pids() const {
    std::vector<int> pids;
    pids.reserve(processes.size());
    for (const auto& [pid, _] : processes) {
        pids.push_back(pid);
    }
    std::sort(pids.begin(), pids.end());
    return pids;
}

std::vector<std::pair<int,int>> MemoryManager::get_frame_owners(int frame_index) const {
    auto it = frame_to_owners.find(frame_index);
    if (it == frame_to_owners.end()) return {};
    return it->second;
}

void MemoryManager::create_process(int pid){
    if (processes.count(pid) > 0){
        throw std::runtime_error("Process with this PID already exists");
    }
    processes[pid] = std::make_unique<Process>(pid, tlb_size);
}

void MemoryManager::access(int pid, int virtual_address, bool is_write){
    if (processes.count(pid) == 0){
        throw std::runtime_error("Process does not exist");
    }
    Process& proc = *processes[pid];
    IPageTable& pt = proc.get_page_table();
    int vpn = virtual_address / PAGE_SIZE;

    // TLB lookup first
    int tlb_result = proc.get_tlb().lookup(vpn);
    if (tlb_result != -1) {
        // TLB hit — skip page table entirely
        metrics.tlb_hits++;
        metrics.hits++;
        PageTableEntry* entry = pt.lookup(vpn);
        entry->referenced = true;

        if (!is_write && !entry->writable) {
            int ref = frame_pool.get_frame(entry->frame_index).ref_count;
            if (ref > 1) metrics.cow_copies_avoided++;
        }

        if (is_write) {
            if (!entry->writable) {
                handle_cow(pid, vpn, entry);
                proc.get_tlb().insert(vpn, entry->frame_index, true); // update TLB after CoW
                metrics.cow_copies++;
            }
            entry->dirty = true;
        }
        policy->on_access(entry->frame_index);
        return; // done, skip page table
    }

    // TLB miss: fall through to page table
    metrics.tlb_misses++;
    int physical_address = pt.translate(virtual_address);

    if (physical_address == -1){
        throw std::runtime_error("Invalid virtual address");
    } else if (physical_address == -2){
        // page fault handling
        metrics.page_faults++;
        int frame_index;
        try {
            frame_index = frame_pool.allocate();
        } catch (const std::runtime_error&) {
            int victim_frame = policy->evict();
            for (auto& [owner_pid, owner_vpn] : frame_to_owners[victim_frame]) {
                processes[owner_pid]->get_page_table().invalidate(owner_vpn);
                processes[owner_pid]->get_tlb().invalidate(owner_vpn); // invalidate TLB too
            }
            frame_to_owners[victim_frame].clear();
            frame_index = victim_frame;
            metrics.evictions++;
        }
        pt.insert(vpn, frame_index);
        pt.lookup(vpn)->writable = true;
        frame_to_owners[frame_index] = {{pid, vpn}};
        policy->on_load(frame_index, vpn);
        proc.get_tlb().insert(vpn, frame_index, true); // load into TLB
    } else {
        // page hit via page table
        metrics.hits++;
        PageTableEntry* entry = pt.lookup(vpn);
        entry->referenced = true;

        if (!is_write && !entry->writable) {
            int ref = frame_pool.get_frame(entry->frame_index).ref_count;
            if (ref > 1) metrics.cow_copies_avoided++;
        }

        if (is_write) {
            if (!entry->writable) {
                handle_cow(pid, vpn, entry);
                metrics.cow_copies++;
            }
            entry->dirty = true;
        }
        policy->on_access(entry->frame_index);
        proc.get_tlb().insert(vpn, entry->frame_index, entry->writable); // load into TLB
    }
}
void MemoryManager::fork_process(int parent_pid, int child_pid){
    if (processes.count(parent_pid) == 0){
        throw std::runtime_error("Parent process does not exist");
    } else if (processes.count(child_pid) > 0){
        throw std::runtime_error("Child PID already exists");
    } else {
        Process& parent = *processes[parent_pid];
        auto child = std::make_unique<Process>(child_pid);
        IPageTable& parent_pt = parent.get_page_table();
        IPageTable& child_pt = child->get_page_table();

        // copy page table entries and increment frame ref counts
        for (const auto& [vpn, entry] : parent_pt.get_entries()){
            if (entry.valid){
                child_pt.insert(vpn, entry.frame_index);
                child_pt.lookup(vpn)->writable = false; // mark CoW
                parent_pt.lookup(vpn)->writable = false; // mark CoW
                frame_pool.increment_ref(entry.frame_index);
                frame_to_owners[entry.frame_index].push_back({child_pid, vpn});
            }
        }
        processes[child_pid] = std::move(child);
        metrics.cow_forks++;
    }
}

void MemoryManager::handle_cow(int pid, int vpn, PageTableEntry* entry){
    if (entry->writable){
        return; // already writable, no need to copy
    }
    int old_frame = entry->frame_index;
    int new_frame = frame_pool.allocate();
    policy->on_load(new_frame, vpn); // register new frame with replacement policy
    // simulate copying data by just updating ownership and ref counts
    frame_pool.decrement_ref(old_frame); // decrement ref for old frame
    entry->frame_index = new_frame; // point to new frame
    entry->writable = true; // new frame is writable

    // update ownership for the new frame
    auto& owners = frame_to_owners[old_frame];
    auto it = std::find_if(owners.begin(), owners.end(), [&](const std::pair<int,int>& owner){
        return owner.first == pid && owner.second == vpn;
    });
    if (it != owners.end()){
        owners.erase(it); // remove old ownership
        if (owners.size() == 1){
            // if only one owner left, can mark as writable
            auto [remaining_pid, remaining_vpn] = owners[0];
            processes[remaining_pid]->get_page_table().lookup(remaining_vpn)->writable = true;
        }
    }
    frame_to_owners[new_frame] = {{pid, vpn}}; // add new ownership
}