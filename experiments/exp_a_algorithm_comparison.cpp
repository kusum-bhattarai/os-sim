#include "exp_utils.h"
#include "memory_manager.h"
#include "policy/fifo.h"
#include "policy/lru.h"
#include "policy/opt.h"

static const std::vector<int> SEQUENCE = {1,2,3,4,1,2,5,1,2,3,4,5};
static constexpr int FRAMES = 3;

static void run(MemoryManager& mm, int pid) {
    for (int page : SEQUENCE)
        mm.access(pid, page * PAGE_SIZE, false);
}

int main() {
    print_exp_header(
        "Experiment A: Algorithm Comparison",
        "Sequence: [1,2,3,4,1,2,5,1,2,3,4,5]  Frames: 3"
    );
    print_algo_table_header();

    {
        MemoryManager mm(new FIFO(), FRAMES);
        mm.create_process(1);
        run(mm, 1);
        print_algo_row("FIFO", mm.get_metrics());
    }
    {
        MemoryManager mm(new LRU(), FRAMES);
        mm.create_process(1);
        run(mm, 1);
        print_algo_row("LRU", mm.get_metrics());
    }
    {
        MemoryManager mm(new OPT(SEQUENCE), FRAMES);
        mm.create_process(1);
        run(mm, 1);
        print_algo_row("OPT", mm.get_metrics());
    }

    print_exp_footer();
    return 0;
}
