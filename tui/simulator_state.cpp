#include "simulator_state.h"
#include "policy/fifo.h"
#include "policy/lru.h"
#include "policy/clock.h"
#include "constants.h"
#include <algorithm>

std::unique_ptr<ReplacementPolicy> SimulatorState::make_policy(PolicyType type) {
    switch (type) {
        case PolicyType::FIFO:  return std::make_unique<FIFO>();
        case PolicyType::CLOCK: return std::make_unique<Clock>();
        case PolicyType::LRU:
        default:                return std::make_unique<LRU>();
    }
}

SimulatorState::SimulatorState(PolicyType policy_type, int num_frames)
    : policy_type(policy_type), num_frames(num_frames)
{
    manager = std::make_unique<MemoryManager>(make_policy(policy_type).release(), num_frames);
}

std::string SimulatorState::get_policy_name() const {
    switch (policy_type) {
        case PolicyType::FIFO:  return "FIFO";
        case PolicyType::CLOCK: return "CLOCK";
        case PolicyType::LRU:   return "LRU";
        default:                return "?";
    }
}

void SimulatorState::create_process(int pid) {
    manager->create_process(pid);
    if (viewed_pid == -1) viewed_pid = pid;
}

AccessEvent SimulatorState::step(int pid, int virtual_address, bool is_write) {
    const Metrics before = manager->get_metrics();
    manager->access(pid, virtual_address, is_write);
    const Metrics& after = manager->get_metrics();

    AccessEvent evt;
    evt.pid             = pid;
    evt.virtual_address = virtual_address;
    evt.vpn             = virtual_address / PAGE_SIZE;
    evt.is_write        = is_write;
    evt.tlb_hit         = (after.tlb_hits    > before.tlb_hits);
    evt.page_fault      = (after.page_faults > before.page_faults);
    evt.eviction        = (after.evictions   > before.evictions);
    evt.cow_copy        = (after.cow_copies  > before.cow_copies);

    log.push_back(evt);
    return evt;
}

void SimulatorState::fork(int parent_pid, int child_pid) {
    manager->fork_process(parent_pid, child_pid);
    // ensure child is visible in the process list
    if (viewed_pid == -1) viewed_pid = child_pid;
}

void SimulatorState::reset(PolicyType new_policy_type, int new_num_frames) {
    policy_type = new_policy_type;
    num_frames  = new_num_frames;
    manager     = std::make_unique<MemoryManager>(make_policy(policy_type).release(), num_frames);
    log.clear();
    viewed_pid  = -1;
}

void SimulatorState::cycle_viewed_pid() {
    auto pids = manager->get_pids();
    if (pids.empty()) return;
    auto it = std::find(pids.begin(), pids.end(), viewed_pid);
    if (it == pids.end() || std::next(it) == pids.end()) {
        viewed_pid = pids.front();
    } else {
        viewed_pid = *std::next(it);
    }
}
