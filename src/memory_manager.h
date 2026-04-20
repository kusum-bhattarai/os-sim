#pragma once
#include <unordered_map>
#include <memory>
#include "page_table.h"
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
    std::unordered_map<int, std::vector<std::pair<int,int>>> frame_to_owners;  // frame_index -> list of {pid, vpn} owning this frame (for CoW)

public:
    MemoryManager(ReplacementPolicy* policy, int num_frames = NUM_FRAMES): policy(policy), frame_pool(num_frames) {}

    void create_process(int pid);   // creates a new process with an empty page table
    void access(int pid, int virtual_address, bool is_write);  // simulates a memory access, updates metrics, and handles page faults
    void print_metrics() const {metrics.print();}
    const Metrics& get_metrics() const { return metrics;}
    void reset_metrics() { metrics.reset(); }
    void fork_process(int parent_pid, int child_pid); // creates a new process with a copy of the parent's page table (for CoW implementation)
    void handle_cow(int pid, int vpn, PageTableEntry* entry);
};