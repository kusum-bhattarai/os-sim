#include "exp_utils.h"
#include "memory_manager.h"
#include "policy/lru.h"
#include <random>
#include <vector>

// Enough frames to eliminate page faults — isolates TLB behaviour from paging.
static constexpr int FRAMES   = 32;
static constexpr int ACCESSES = 128;

static void run(MemoryManager& mm, const std::vector<int>& seq) {
    for (int vpn : seq) mm.access(1, vpn * PAGE_SIZE, false);
}

int main() {
    print_exp_header(
        "Experiment F: TLB Hit Rate vs Access Locality",
        "128 accesses, 32 frames (no page faults), TLB size 16"
    );
    print_tlb_pattern_header();

    // Hot working set: 8 pages repeated 16 times.
    // After the first 8 cold misses the TLB is warm for the rest.
    {
        MemoryManager mm(new LRU(), FRAMES);
        mm.create_process(1);
        std::vector<int> seq;
        for (int round = 0; round < 16; round++)
            for (int p = 0; p < 8; p++) seq.push_back(p);
        run(mm, seq);
        print_tlb_pattern_row("Hot working set", mm.get_metrics());
    }

    // Sequential scan: 32 pages in order, repeated 4 times.
    // TLB holds 16 entries; a 32-page scan evicts half on each pass,
    // so only the tail of each pass hits on the next.
    {
        MemoryManager mm(new LRU(), FRAMES);
        mm.create_process(1);
        std::vector<int> seq;
        for (int round = 0; round < 4; round++)
            for (int p = 0; p < 32; p++) seq.push_back(p);
        run(mm, seq);
        print_tlb_pattern_row("Sequential scan", mm.get_metrics());
    }

    // Random: uniform draw from [0, 1023].
    // With 1024 possible pages and a 16-entry TLB the collision probability
    // is ~1.6%, so nearly every access is a cold miss.
    {
        MemoryManager mm(new LRU(), FRAMES);
        mm.create_process(1);
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> dist(0, 1023);
        std::vector<int> seq;
        for (int i = 0; i < ACCESSES; i++) seq.push_back(dist(rng));
        run(mm, seq);
        print_tlb_pattern_row("Random", mm.get_metrics());
    }

    print_exp_footer();
    return 0;
}
