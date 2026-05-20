#pragma once
#include <string>
#include <vector>
#include "simulator_state.h"

struct PolicyResult {
    std::string policy_name;
    PolicyType  policy_type;
    int         faults;
    int         tlb_hits;
    int         evictions;
    int         num_frames;
};

// Run vpns against FIFO, LRU, and CLOCK with the given frame count.
// Each policy runs in a completely isolated MemoryManager; sim is never touched.
std::vector<PolicyResult> run_comparison(
    const std::vector<int>& vpns, int pid, bool is_write, int num_frames);

// Run vpns across frames_min..frames_max for all three policies.
// Results are ordered: for each frame count f, FIFO then LRU then CLOCK.
std::vector<PolicyResult> run_frame_sweep(
    const std::vector<int>& vpns, int pid, bool is_write,
    int frames_min, int frames_max);
