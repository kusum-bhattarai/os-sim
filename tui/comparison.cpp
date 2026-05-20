#include "comparison.h"
#include "memory_manager.h"
#include "constants.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

// ---------------------------------------------------------------------------
// Internal helper — one isolated run for a single (policy, frame-count) pair.
// ---------------------------------------------------------------------------

static PolicyResult run_one(
    const std::vector<int>& vpns, int pid, bool is_write,
    PolicyType ptype, int num_frames)
{
    auto policy = SimulatorState::make_policy(ptype);
    MemoryManager mm(policy.release(), num_frames);
    mm.create_process(pid);
    for (int vpn : vpns)
        mm.access(pid, static_cast<int>(vpn * PAGE_SIZE), is_write);
    const Metrics& m = mm.get_metrics();

    std::string name;
    switch (ptype) {
        case PolicyType::FIFO:  name = "FIFO";  break;
        case PolicyType::LRU:   name = "LRU";   break;
        case PolicyType::CLOCK: name = "CLOCK"; break;
    }
    return { name, ptype, m.page_faults, m.tlb_hits, m.evictions, num_frames };
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::vector<PolicyResult> run_comparison(
    const std::vector<int>& vpns, int pid, bool is_write, int num_frames)
{
    return {
        run_one(vpns, pid, is_write, PolicyType::FIFO,  num_frames),
        run_one(vpns, pid, is_write, PolicyType::LRU,   num_frames),
        run_one(vpns, pid, is_write, PolicyType::CLOCK, num_frames),
    };
}

std::vector<PolicyResult> run_frame_sweep(
    const std::vector<int>& vpns, int pid, bool is_write,
    int frames_min, int frames_max)
{
    std::vector<PolicyResult> results;
    results.reserve(static_cast<size_t>((frames_max - frames_min + 1) * 3));
    for (int f = frames_min; f <= frames_max; ++f)
        for (PolicyType p : {PolicyType::FIFO, PolicyType::LRU, PolicyType::CLOCK})
            results.push_back(run_one(vpns, pid, is_write, p, f));
    return results;
}

// ---------------------------------------------------------------------------
// FTXUI table renderers
// ---------------------------------------------------------------------------

ftxui::Element render_comparison_table(const std::vector<PolicyResult>& results) {
    auto col = [](const std::string& s, int w, Color c) {
        return text(s) | color(c) | size(WIDTH, EQUAL, w);
    };

    auto hdr = hbox({
        text(" Algorithm") | bold | size(WIDTH, EQUAL, 11),
        text(" Frames")    | bold | size(WIDTH, EQUAL, 8),
        text(" Faults")    | bold | size(WIDTH, EQUAL, 9),
        text(" TLB Hits")  | bold | size(WIDTH, EQUAL, 10),
        text(" Evictions") | bold | size(WIDTH, EQUAL, 11),
    }) | bgcolor(Color::GrayDark);

    std::vector<Element> rows = { hdr, separator() };
    for (const auto& r : results) {
        rows.push_back(hbox({
            text(" " + r.policy_name)
                | bold | color(Color::Cyan) | size(WIDTH, EQUAL, 11),
            col(" " + std::to_string(r.num_frames), 8,  Color::White),
            col(" " + std::to_string(r.faults),     9,  r.faults > 0 ? Color::Red : Color::Green),
            col(" " + std::to_string(r.tlb_hits),   10, Color::Green),
            col(" " + std::to_string(r.evictions),  11, Color::Yellow),
        }));
    }
    return vbox(std::move(rows));
}

ftxui::Element render_sweep_table(const std::vector<PolicyResult>& results,
                                   int frames_min, int frames_max) {
    auto hdr = hbox({
        text(" Frames")       | bold | size(WIDTH, EQUAL, 9),
        text(" FIFO faults")  | bold | color(Color::Red)    | size(WIDTH, EQUAL, 14),
        text(" LRU faults")   | bold | color(Color::Yellow) | size(WIDTH, EQUAL, 13),
        text(" CLOCK faults") | bold | color(Color::Cyan)   | size(WIDTH, EQUAL, 14),
    }) | bgcolor(Color::GrayDark);

    std::vector<Element> rows = { hdr, separator() };

    const int range = frames_max - frames_min + 1;
    for (int i = 0; i < range; ++i) {
        int base = i * 3;
        if (base + 2 >= static_cast<int>(results.size())) break;

        int f    = results[base].num_frames;
        int fifo = results[base].faults;
        int lru  = results[base + 1].faults;
        int clk  = results[base + 2].faults;

        // Belady's anomaly: FIFO fault count increases despite one more frame.
        bool belady = (i > 0 && fifo > results[(i - 1) * 3].faults);

        rows.push_back(hbox({
            text("  " + std::to_string(f)) | size(WIDTH, EQUAL, 9),
            hbox({
                text(" " + std::to_string(fifo)) | color(Color::Red),
                belady ? text(" ▲") | bold | color(Color::Magenta) : text("  "),
            }) | size(WIDTH, EQUAL, 14),
            text(" " + std::to_string(lru))  | color(Color::Yellow) | size(WIDTH, EQUAL, 13),
            text(" " + std::to_string(clk))  | color(Color::Cyan)   | size(WIDTH, EQUAL, 14),
        }));
    }

    rows.push_back(separator());
    rows.push_back(
        text("  ▲ = Belady's anomaly: more frames caused more faults (FIFO only)")
        | dim);

    return vbox(std::move(rows));
}
