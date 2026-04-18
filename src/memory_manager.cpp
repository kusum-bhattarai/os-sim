#include "memory_manager.h"
#include <stdexcept>

void MemoryManager::create_process(int pid){
    if (processes.count(pid) > 0){
        throw std::runtime_error("Process with this PID already exists");
    }
    processes[pid] = std::make_unique<Process>(pid);
}

void MemoryManager::access(int pid, int virtual_address, bool is_write){
    if (processes.count(pid) == 0){
        throw std::runtime_error("Process does not exist");
    }
    Process& proc = *processes[pid];
    PageTable& pt = proc.get_page_table();
    int vpn = virtual_address / PAGE_SIZE;
    // int offset = virtual_address % PAGE_SIZE;
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
            // no free frames, need to evict
            int victim_frame = policy->evict();
            auto [victim_pid, victim_vpn] = frame_to_process_vpn[victim_frame];
            Process& victim_proc = *processes[victim_pid];
            victim_proc.get_page_table().invalidate(victim_vpn);
            frame_index = victim_frame;
            metrics.evictions++;
        } 
        pt.insert(vpn, frame_index);
        frame_to_process_vpn[frame_index] = {pid, vpn};
        policy->on_load(frame_index, vpn);
    } else {
        // page hit
        metrics.hits++;
        PageTableEntry* entry = pt.lookup(vpn);
        if (is_write){
            entry->dirty = true;
        }
        entry->referenced = true;
        policy->on_access(entry->frame_index);
    }
}