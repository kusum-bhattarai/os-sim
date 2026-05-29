#pragma once
#include <memory>
#include <string>
#include <vector>
#include "memory_manager.h"
#include "policy/replacement_policy.h"

class Clock; // forward declaration — full type only needed in simulator_state.cpp

enum class PolicyType { FIFO, LRU, CLOCK };

struct AccessEvent {
    int  pid;
    int  virtual_address;
    int  vpn;
    bool is_write;
    bool tlb_hit;
    bool page_fault;
    bool eviction;
    bool cow_copy;
};

class SimulatorState {
public:
    SimulatorState(PolicyType policy_type, int num_frames);

    void        create_process(int pid, PageTableType pt_type = PageTableType::FLAT);
    AccessEvent step(int pid, int virtual_address, bool is_write);
    void        fork(int parent_pid, int child_pid);
    void        reset(PolicyType policy_type, int num_frames);

    const MemoryManager&             get_manager()      const { return *manager; }
    const std::vector<AccessEvent>&  get_log()          const { return log; }
    PolicyType                       get_policy_type()  const { return policy_type; }
    std::string                      get_policy_name()  const;
    int                              get_num_frames()   const { return num_frames; }
    // nullptr when policy is not CLOCK
    const Clock*                     get_clock_policy() const { return clock_policy; }

    int  get_viewed_pid() const { return viewed_pid; }
    void cycle_viewed_pid();

    // Public so isolated managers (e.g. comparison runs) can reuse the same
    // policy factory without duplicating the switch.
    static std::unique_ptr<ReplacementPolicy> make_policy(PolicyType type);

private:
    PolicyType                      policy_type;
    int                             num_frames;
    std::unique_ptr<MemoryManager>  manager;
    std::vector<AccessEvent>        log;
    int                             viewed_pid  = -1;
    Clock*                          clock_policy = nullptr; // non-owning
};
