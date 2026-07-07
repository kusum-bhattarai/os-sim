#include "buddy_allocator.h"
#include <algorithm>
#include <stdexcept>

BuddyAllocator::BuddyAllocator(int num_pages) : total_pages(num_pages) {
    if (!is_power_of_two(num_pages)) {
        throw std::invalid_argument("Page count must be a power of two");
    }
    max_order = 0;
    while ((1 << max_order) < num_pages) max_order++;
    free_lists.resize(max_order + 1);
    free_lists[max_order].insert(0);
}

int BuddyAllocator::order_for(int num_pages) {
    if (num_pages <= 0) {
        return -1;
    }
    int order = 0;
    while ((1 << order) < num_pages) order++;
    return order;
}

int BuddyAllocator::allocate(int num_pages) {
    int order = order_for(num_pages);
    if (order == -1) {
        throw std::invalid_argument("Page count must be positive");
    }
    if (order > max_order) {
        return -1;
    }
    int k = order;
    while (k <= max_order && free_lists[k].empty()) k++;
    if (k > max_order) {
        return -1;
    }
    int base = *free_lists[k].begin();
    free_lists[k].erase(free_lists[k].begin());
    while (k > order) {
        k--;
        free_lists[k].insert(base + (1 << k)); // release the upper half
    }
    allocated[base] = order;
    return base;
}

void BuddyAllocator::free(int base) {
    auto it = allocated.find(base);
    if (it == allocated.end()) {
        throw std::runtime_error("Block was not allocated");
    }
    int order = it->second;
    allocated.erase(it);
    while (order < max_order) {
        int buddy = base ^ (1 << order);
        auto& list = free_lists[order];
        auto buddy_it = list.find(buddy);
        if (buddy_it == list.end()) {
            break; // buddy still allocated or split — stop merging
        }
        list.erase(buddy_it);
        base = std::min(base, buddy);
        order++;
    }
    free_lists[order].insert(base);
}
