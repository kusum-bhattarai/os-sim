#include "comparison.h"
#include "memory_manager.h"
#include "constants.h"

// ---------------------------------------------------------------------------
// Internal helper — one isolated run for a single (policy, frame-count) pair.
// ---------------------------------------------------------------------------

static PolicyResult run_one(
    const std::vector<int>& vpns, int pid, bool is_write,
    PolicyType ptype, int num_frames)
{
    auto policy = SimulatorState::make_policy(ptype);
    MemoryManager mm(policy.release(), num_frames);
    mm.create_process(pid);
    for (int vpn : vpns)
        mm.access(pid, static_cast<int>(vpn * PAGE_SIZE), is_write);
    const Metrics& m = mm.get_metrics();

    std::string name;
    switch (ptype) {
        case PolicyType::FIFO:  name = "FIFO";  break;
        case PolicyType::LRU:   name = "LRU";   break;
        case PolicyType::CLOCK: name = "CLOCK"; break;
    }
    return { name, ptype, m.page_faults, m.tlb_hits, m.evictions, num_frames };
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::vector<PolicyResult> run_comparison(
    const std::vector<int>& vpns, int pid, bool is_write, int num_frames)
{
    return {
        run_one(vpns, pid, is_write, PolicyType::FIFO,  num_frames),
        run_one(vpns, pid, is_write, PolicyType::LRU,   num_frames),
        run_one(vpns, pid, is_write, PolicyType::CLOCK, num_frames),
    };
}

std::vector<PolicyResult> run_frame_sweep(
    const std::vector<int>& vpns, int pid, bool is_write,
    int frames_min, int frames_max)
{
    std::vector<PolicyResult> results;
    results.reserve(static_cast<size_t>((frames_max - frames_min + 1) * 3));
    for (int f = frames_min; f <= frames_max; ++f)
        for (PolicyType p : {PolicyType::FIFO, PolicyType::LRU, PolicyType::CLOCK})
            results.push_back(run_one(vpns, pid, is_write, p, f));
    return results;
}
