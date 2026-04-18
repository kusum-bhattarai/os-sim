#include "fifo.h"
#include <stdexcept>

void FIFO::on_load(int frame_index, int /*vpn*/){
    load_order.push(frame_index);
}

void FIFO::on_access(int frame_index){
    // nothing to do after access
}

int FIFO::evict(){
    if (load_order.empty()){
        throw std::runtime_error("No frames exist");
    }
    int victim = load_order.front();
    load_order.pop();
    return victim;
}

void FIFO::reset(){
    std::queue<int> empty;
    std::swap(load_order, empty);
}