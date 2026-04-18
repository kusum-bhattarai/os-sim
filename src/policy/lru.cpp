#include "lru.h"
#include <stdexcept>

void LRU::on_load(int frame_index, int /*vpn*/){
    if (frame_positions.count(frame_index) == 0){
        order.push_back(frame_index);
        frame_positions[frame_index] = std::prev(order.end());
    } else {
        // if frame already exists, access and move to back
        on_access(frame_index);
    }
}

void LRU::on_access(int frame_index){
    if (frame_positions.count(frame_index) == 0){
        throw std::runtime_error("Accessing frame that isn't loaded");
    } else {
        // move accessed to the back of the list
        order.splice(order.end(), order, frame_positions[frame_index]);
        frame_positions[frame_index] = std::prev(order.end());
    }
}

int LRU::evict(){
    if (order.empty()){
        throw std::runtime_error("No frames exist");
    } 
    int victim = order.front();
    order.pop_front();
    frame_positions.erase(victim);
    return victim;
}

void LRU::reset(){
    order.clear();
    frame_positions.clear();
}