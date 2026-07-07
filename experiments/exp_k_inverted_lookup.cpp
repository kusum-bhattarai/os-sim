#include "exp_utils.h"
#include "page_table_inverted.h"
#include "constants.h"
#include <iostream>
#include <iomanip>

static constexpr int PID = 1;

struct Result {
    int frames;
    double avg_linear;
    double avg_indexed;
};

static Result run(int frames) {
    InvertedPageTable ipt(frames);
    for (int f = 0; f < frames; f++)
        ipt.insert(PID, f, f); // vpn f lives in frame f — table fully occupied

    ipt.reset_probe_stats();
    for (int f = 0; f < frames; f++)
        ipt.linear_lookup(PID, f);
    double avg_linear = static_cast<double>(ipt.total_linear_probes()) / frames;

    ipt.reset_probe_stats();
    for (int f = 0; f < frames; f++)
        ipt.lookup(PID, f);
    double avg_indexed = static_cast<double>(ipt.total_indexed_probes()) / frames;

    return { frames, avg_linear, avg_indexed };
}

int main() {
    print_exp_header(
        "Experiment K: Inverted Page Table Lookup Cost",
        "forward lookup (VPN -> frame), full table"
    );

    std::cout << std::right << std::setw(8)  << "Frames"
                            << std::setw(13) << "Table slots"
                            << std::setw(15) << "Linear probes"
                            << std::setw(10) << "Indexed" << "\n";
    print_separator('-');

    for (int frames : {16, 64, 256, 1024}) {
        Result r = run(frames);
        std::cout << std::right << std::setw(8)  << r.frames
                                << std::setw(13) << r.frames
                                << std::setw(15) << std::fixed << std::setprecision(1) << r.avg_linear
                                << std::setw(10) << std::setprecision(1) << r.avg_indexed << "\n";
    }

    std::cout << "\nThe table has one slot per physical frame regardless of the\n";
    std::cout << (VIRTUAL_ADDRESS_SPACE_SIZE / PAGE_SIZE) << "-page virtual address space. Without the hash index a\n";
    std::cout << "forward lookup scans half the table on average; the index\n";
    std::cout << "makes it one probe at the cost of a second structure.\n";
    print_exp_footer();
    return 0;
}
