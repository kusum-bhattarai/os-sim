#pragma once
#include <unordered_map>
#include <vector>
#include "page_table_iface.h"
#include "constants.h"

// one global table indexed by physical frame: which (pid, vpn) owns each frame
struct InvertedEntry {
    int pid;
    int vpn;
    PageTableEntry entry;
    InvertedEntry() : pid(-1), vpn(-1) {}
};

class InvertedPageTable {
    std::vector<InvertedEntry> table;
    std::unordered_map<long long, int> index; // (pid, vpn) -> frame
    long long indexed_probes = 0;
    long long linear_probes = 0;
    long long lookup_count = 0;

    static long long key(int pid, int vpn) {
        return (static_cast<long long>(pid) << 32) | static_cast<unsigned int>(vpn);
    }

public:
    InvertedPageTable(int num_frames = NUM_FRAMES);

    void insert(int pid, int vpn, int frame_index);
    PageTableEntry* lookup(int pid, int vpn);        // via hash index
    PageTableEntry* linear_lookup(int pid, int vpn); // scan every frame
    void invalidate(int pid, int vpn);
    int translate(int pid, int virtual_address);
    std::unordered_map<int, PageTableEntry> entries_for(int pid) const;

    int frame_count() const { return static_cast<int>(table.size()); }
    const InvertedEntry& entry_at(int frame_index) const { return table.at(frame_index); }

    long long total_indexed_probes() const { return indexed_probes; }
    long long total_linear_probes() const { return linear_probes; }
    long long total_lookups() const { return lookup_count; }
    void reset_probe_stats() { indexed_probes = 0; linear_probes = 0; lookup_count = 0; }
};

// per-process adapter so the shared inverted table fits the IPageTable interface
class InvertedPageTableView : public IPageTable {
    InvertedPageTable* table;
    int pid;
public:
    InvertedPageTableView(InvertedPageTable* table, int pid) : table(table), pid(pid) {}
    void insert(int vpn, int frame_index) override { table->insert(pid, vpn, frame_index); }
    PageTableEntry* lookup(int vpn) override { return table->lookup(pid, vpn); }
    void invalidate(int vpn) override { table->invalidate(pid, vpn); }
    int translate(int virtual_address) override { return table->translate(pid, virtual_address); }
    std::unordered_map<int, PageTableEntry> get_entries() const override { return table->entries_for(pid); }
};
