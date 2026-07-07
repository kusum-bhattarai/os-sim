#include "working_set.h"
#include <stdexcept>

void WorkingSetPolicy::on_load(int frame_index, int /*vpn*/){
    last_access[frame_index] = ++clock;
}

void WorkingSetPolicy::on_access(int frame_index){
    if (last_access.count(frame_index) == 0){
        throw std::runtime_error("Accessing frame that isn't loaded");
    }
    last_access[frame_index] = ++clock;
}

int WorkingSetPolicy::evict(){
    if (last_access.empty()){
        throw std::runtime_error("No frames exist");
    }
    // oldest frame first — an expired frame if one exists, else LRU fallback
    int victim = -1;
    long long oldest = 0;
    for (const auto& [frame, time] : last_access){
        if (victim == -1 || time < oldest){
            victim = frame;
            oldest = time;
        }
    }
    last_access.erase(victim);
    return victim;
}

void WorkingSetPolicy::reset(){
    last_access.clear();
    clock = 0;
}

std::vector<int> WorkingSetPolicy::expired_frames() const {
    std::vector<int> expired;
    for (const auto& [frame, time] : last_access){
        if (clock - time >= window){
            expired.push_back(frame);
        }
    }
    return expired;
}

int WorkingSetPolicy::working_set_size() const {
    int n = 0;
    for (const auto& [frame, time] : last_access){
        if (clock - time < window){
            n++;
        }
    }
    return n;
}
