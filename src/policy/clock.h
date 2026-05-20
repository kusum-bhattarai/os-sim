#pragma once
#include "replacement_policy.h"
#include <vector>
#include <unordered_map>

class Clock : public ReplacementPolicy {
public:
    struct Entry {
        int  frame_index;
        bool referenced = false;
    };

    void on_load(int frame_index, int vpn) override;
    void on_access(int frame_index) override;
    int  evict() override;
    void reset() override;

    const std::vector<Entry>& get_entries() const { return frames; }
    size_t                    get_hand()    const { return hand; }

private:
    std::vector<Entry>              frames;
    size_t                          hand = 0;
    std::unordered_map<int, size_t> frame_to_index;
};