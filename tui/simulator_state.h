#pragma once
#include <memory>
#include <string>
#include <vector>
#include "memory_manager.h"
#include "policy/replacement_policy.h"

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

    void        create_process(int pid);
    AccessEvent step(int pid, int virtual_address, bool is_write);
    void        fork(int parent_pid, int child_pid);
    void        reset(PolicyType policy_type, int num_frames);

    const MemoryManager&             get_manager()     const { return *manager; }
    const std::vector<AccessEvent>&  get_log()         const { return log; }
    PolicyType                       get_policy_type() const { return policy_type; }
    std::string                      get_policy_name() const;
    int                              get_num_frames()  const { return num_frames; }

    int  get_viewed_pid() const { return viewed_pid; }
    void cycle_viewed_pid();

private:
    PolicyType                      policy_type;
    int                             num_frames;
    std::unique_ptr<MemoryManager>  manager;
    std::vector<AccessEvent>        log;
    int                             viewed_pid = -1;

    static std::unique_ptr<ReplacementPolicy> make_policy(PolicyType type);
};
