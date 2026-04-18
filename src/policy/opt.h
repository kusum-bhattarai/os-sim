#pragma once
#include "replacement_policy.h"
#include <vector>
#include <unordered_map>

class OPT: public ReplacementPolicy {
private:
    std::vector<int> access_sequence;  // full sequence passed in construction
    int curr_pos;   // curr position in the access sequence
    std::unordered_map<int, int> frame_to_vpn;  // frame_index ->vpn

public:
    OPT(const std::vector<int>& access_seq): access_sequence(access_seq), curr_pos(0) {}
    void on_load(int frame_index, int vpn) override;
    void on_access(int frame_index) override;
    int evict() override;
    void reset() override;
};