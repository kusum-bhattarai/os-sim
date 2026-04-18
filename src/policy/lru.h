#pragma once
#include "replacement_policy.h"
#include <unordered_map>
#include <list>

class LRU: public ReplacementPolicy {
private:
    std::list<int> order;  // front = LRU, back = MRU
    std::unordered_map<int, std::list<int>::iterator> frame_positions;  // frame_index -> position in the list

public:
    void on_load(int frame_index, int vpn) override;
    void on_access(int frame_index) override;
    int evict() override;
    void reset() override;
};