#include "exp_utils.h"
#include "memory_manager.h"
#include "policy/lru.h"
#include <vector>

// 20-page working set accessed in a tight loop.
// Enough frames to avoid page faults — only TLB size is the variable.
static constexpr int WORKING_SET = 20;
static constexpr int ROUNDS      = 10;   // 200 total accesses
static constexpr int FRAMES      = 64;

int main() {
    print_exp_header(
        "Experiment G: TLB Hit Rate vs TLB Size",
        "20-page working set, 200 accesses, 64 frames (no faults)"
    );
    print_tlb_size_header();

    std::vector<int> seq;
    for (int r = 0; r < ROUNDS; r++)
        for (int p = 0; p < WORKING_SET; p++) seq.push_back(p);

    for (size_t tlb_size : {4u, 8u, 16u, 32u, 64u}) {
        MemoryManager mm(new LRU(), FRAMES, tlb_size);
        mm.create_process(1);
        for (int vpn : seq) mm.access(1, vpn * PAGE_SIZE, false);
        print_tlb_size_row(tlb_size, mm.get_metrics());
    }

    print_exp_footer();
    return 0;
}
