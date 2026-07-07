#pragma once
#include <set>
#include <unordered_map>
#include <vector>

// power-of-two page-level allocator: blocks split on demand and merge with
// their buddy on free
class BuddyAllocator {
private:
    int total_pages;
    int max_order;
    std::vector<std::set<int>> free_lists;   // order -> base page indexes
    std::unordered_map<int, int> allocated;  // base -> order

    static bool is_power_of_two(int n) { return n > 0 && (n & (n - 1)) == 0; }

public:
    explicit BuddyAllocator(int num_pages = 128);

    // allocate num_pages rounded up to a power of two; returns the base page
    // index, or -1 when fragmentation leaves no block large enough
    int allocate(int num_pages);
    void free(int base);

    // smallest order whose block holds num_pages
    static int order_for(int num_pages);

    int get_total_pages() const { return total_pages; }
    int get_max_order() const { return max_order; }
    int free_count(int order) const { return static_cast<int>(free_lists[order].size()); }

    int free_pages() const {
        int n = 0;
        for (int k = 0; k <= max_order; k++)
            n += free_count(k) * (1 << k);
        return n;
    }

    int largest_free_block() const {
        for (int k = max_order; k >= 0; k--)
            if (!free_lists[k].empty()) return 1 << k;
        return 0;
    }
};
