#include "exp_utils.h"
#include "memory_manager.h"
#include "policy/lru.h"

static const std::vector<int> SEQUENCE = {1,2,3,4,1,2,5,1,2,3,4,5};

int main() {
    print_exp_header(
        "Experiment B: Frame Count Sensitivity (LRU)",
        "Sequence: [1,2,3,4,1,2,5,1,2,3,4,5]"
    );
    print_frame_table_header();

    for (int frames = 1; frames <= 7; ++frames) {
        MemoryManager mm(new LRU(), frames);
        mm.create_process(1);
        for (int page : SEQUENCE)
            mm.access(1, page * PAGE_SIZE, false);
        print_frame_row(frames, mm.get_metrics());
    }

    print_exp_footer();
    return 0;
}
