#pragma once
#include "replacement_policy.h"
#include <queue>

class FIFO: public ReplacementPolicy {
private:
    std::queue<int> load_order; // tracks the order of loaded frames

public:
    void on_load(int frame_index, int vpn) override;
    void on_access(int frame_index) override;
    int evict() override;
    void reset() override;
};