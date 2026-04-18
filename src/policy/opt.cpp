#include "opt.h"
#include <stdexcept>
#include <climits>

void OPT::on_load(int frame_index, int vpn){
    frame_to_vpn[frame_index] = vpn;
}

void OPT::on_access(int frame_index){
    // increment curr_pos 
    if (curr_pos < static_cast<int>(access_sequence.size())){
        curr_pos++;
    }
}

int OPT::evict(){
    if (frame_to_vpn.empty()){
        throw std::runtime_error("No frames exist");
    }
    int max_distance = -1;
    int victim = -1;
    for (const auto& [frame_index, vpn] : frame_to_vpn){
        // check if vpn is accessed in the future
        auto it = std::find(access_sequence.begin() + curr_pos, access_sequence.end(), vpn);
        int distance;
        if (it == access_sequence.end()){
            distance = INT_MAX;  // not accessed again, so max distance
        } else {
            // if accessed again, keep track of the farthest distance
            distance = std::distance(access_sequence.begin() + curr_pos, it);
        }
        if (distance > max_distance){
            max_distance = distance;
            victim = frame_index;
        }
    }
    return victim;
}

void OPT::reset(){
    curr_pos = 0;
    frame_to_vpn.clear();
}