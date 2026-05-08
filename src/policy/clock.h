#pragma once
#include "replacement_policy.h"
#include <vector>
#include <unordered_map>

class Clock : public ReplacementPolicy {
    struct Entry {
        int frame_index;
        bool referenced = false;
    };

    std::vector<Entry> frames;
    size_t hand = 0; // points to the next candidate for eviction
    std::unordered_map<int, size_t> frame_to_index; // frame_index -> index in frames vector

public:
    void on_load(int frame_index, int vpn) override;
    void on_access(int frame_index) override;
    int evict() override;
    void reset() override;
};