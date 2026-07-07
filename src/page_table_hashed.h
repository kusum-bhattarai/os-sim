#pragma once
#include <array>
#include <forward_list>
#include <unordered_map>
#include "page_table_iface.h"

// VPN is hashed into one of 64 buckets; collisions chain within the bucket
static constexpr int HASHED_BUCKETS = 64;

struct HashedNode {
    int vpn;
    PageTableEntry entry;
    HashedNode(int vpn) : vpn(vpn) {}
};

class HashedPageTable : public IPageTable {
    std::array<std::forward_list<HashedNode>, HASHED_BUCKETS> buckets;
    long long probes = 0;
    long long lookups = 0;
public:
    void insert(int vpn, int frame_index) override;
    PageTableEntry* lookup(int vpn) override;
    void invalidate(int vpn) override;
    int translate(int virtual_address) override;
    std::unordered_map<int, PageTableEntry> get_entries() const override;

    int bucket_count() const { return HASHED_BUCKETS; }

    int chain_length(int bucket) const {
        int n = 0;
        for (const auto& node : buckets[bucket]) { (void)node; n++; }
        return n;
    }

    int max_chain_length() const {
        int max = 0;
        for (int b = 0; b < HASHED_BUCKETS; b++)
            if (chain_length(b) > max) max = chain_length(b);
        return max;
    }

    long long total_probes() const { return probes; }
    long long total_lookups() const { return lookups; }
    void reset_probe_stats() { probes = 0; lookups = 0; }

private:
    static int bucket_index(int vpn) { return vpn % HASHED_BUCKETS; }
};
