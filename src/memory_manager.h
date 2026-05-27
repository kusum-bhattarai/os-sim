#pragma once
#include <unordered_map>
#include <memory>
#include "page_table_iface.h"
#include "frame_pool.h"
#include "metrics.h"
#include "policy/replacement_policy.h"
#include "process.h"

class MemoryManager{
private:
    FramePool frame_pool;
    Metrics metrics;
    std::unique_ptr<ReplacementPolicy> policy;
    std::unordered_map<int,std::unique_ptr<Process>> processes; // pid -> process
    std::unordered_map<int, std::vector<std::pair<int,int>>> frame_to_owners;
    size_t tlb_size;

public:
    MemoryManager(ReplacementPolicy* policy, int num_frames = NUM_FRAMES, size_t tlb_size = TLB_SIZE)
        : policy(policy), frame_pool(num_frames), tlb_size(tlb_size) {}

    void create_process(int pid, PageTableType pt_type = PageTableType::FLAT);
    void access(int pid, int virtual_address, bool is_write);
    void fork_process(int parent_pid, int child_pid);
    void handle_cow(int pid, int vpn, PageTableEntry* entry);

    void print_metrics() const { metrics.print(); }
    void reset_metrics() { metrics.reset(); }
    const Metrics& get_metrics() const { return metrics; }

    // read-only introspection for visualization
    const FramePool& get_frame_pool() const { return frame_pool; }
    const Process&   get_process(int pid) const;
    std::vector<int> get_pids() const;
    std::vector<std::pair<int,int>> get_frame_owners(int frame_index) const;
};