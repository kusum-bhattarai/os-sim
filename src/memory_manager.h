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
    std::unordered_map<int,std::pair<int,int>> frame_to_process_vpn; // frame_index -> {pid, vpn}

public:
    MemoryManager(ReplacementPolicy* policy, int num_frames = NUM_FRAMES): policy(policy), frame_pool(num_frames) {}

    void create_process(int pid);   // creates a new process with an empty page table
    void access(int pid, int virtual_address, bool is_write);  // simulates a memory access, updates metrics, and handles page faults
    void print_metrics() const {metrics.print();}
    void reset_metrics() { metrics.reset(); }
};