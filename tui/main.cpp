#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "simulator_state.h"

using namespace ftxui;

// ── formatting helpers ────────────────────────────────────────────────────────

static std::string hex_addr(int addr) {
    std::ostringstream ss;
    ss << "0x" << std::hex << std::uppercase
       << std::setw(5) << std::setfill('0') << addr;
    return ss.str();
}

// ── panel renderers ───────────────────────────────────────────────────────────

static Element render_header(const SimulatorState& sim) {
    auto pids = sim.get_manager().get_pids();
    std::string pid_str = pids.empty() ? "none"
        : "PID " + std::to_string(sim.get_viewed_pid())
          + " / " + std::to_string(pids.size());

    return hbox({
        text("  os-sim") | bold,
        text("  Virtual Memory Simulator") | dim,
        filler(),
        text("Policy: ") | dim,
        text(sim.get_policy_name()) | bold | color(Color::Cyan),
        text("  Frames: ") | dim,
        text(std::to_string(sim.get_num_frames())) | bold,
        text("  Viewing: ") | dim,
        text(pid_str) | bold | color(Color::Yellow),
        text("  "),
    }) | color(Color::White) | bgcolor(Color::Blue);
}

static Element render_frame_pool(const SimulatorState& sim) {
    const auto& pool = sim.get_manager().get_frame_pool();
    const int cap  = pool.get_capacity();
    const int cols = 8;

    std::vector<Element> rows;
    for (int r = 0; r * cols < cap; ++r) {
        std::vector<Element> cells;
        for (int c = 0; c < cols && r * cols + c < cap; ++c) {
            int idx = r * cols + c;
            const Frame& f = pool.peek_frame(idx);
            std::string label = " " + std::string(idx < 10 ? " " : "")
                              + std::to_string(idx) + " ";
            Element cell;
            if (!f.in_use) {
                cell = text(label) | dim | border;
            } else if (f.ref_count > 1) {
                cell = text(label) | bold | color(Color::Yellow) | border;
            } else {
                cell = text(label) | bold | color(Color::Green) | border;
            }
            cells.push_back(std::move(cell));
        }
        rows.push_back(hbox(std::move(cells)));
    }

    rows.push_back(separator());
    rows.push_back(hbox({
        text("  "),
        text("□") | dim,        text(" free  ") | dim,
        text("■") | color(Color::Green),  text(" used  ") | dim,
        text("◉") | color(Color::Yellow), text(" shared (CoW)") | dim,
    }));

    return window(text(" Frame Pool "), vbox(std::move(rows)));
}

static Element render_page_table(const SimulatorState& sim) {
    int pid = sim.get_viewed_pid();
    std::string title = " Page Table"
        + (pid >= 0 ? " — PID " + std::to_string(pid) : "") + " ";

    auto hdr = hbox({
        text("  VPN  ") | bold,
        text(" Frame ") | bold,
        text(" Dirty ") | bold,
        text(" Write ") | bold,
    }) | bgcolor(Color::GrayDark);

    std::vector<Element> rows = { hdr };

    if (pid < 0) {
        rows.push_back(text("  (no process)") | dim | hcenter);
    } else {
        const auto& raw = sim.get_manager().get_process(pid)
                              .get_page_table().get_entries();

        std::vector<std::pair<int, PageTableEntry>> sorted(raw.begin(), raw.end());
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b){ return a.first < b.first; });

        bool any = false;
        for (const auto& [vpn, e] : sorted) {
            if (!e.valid) continue;
            any = true;
            rows.push_back(hbox({
                text("  " + std::to_string(vpn))
                    | color(Color::Cyan) | size(WIDTH, EQUAL, 7),
                text(std::to_string(e.frame_index))
                    | size(WIDTH, EQUAL, 7),
                text(e.dirty    ? "  *  " : "  -  ")
                    | color(e.dirty    ? Color::Red    : Color::GrayLight),
                text(e.writable ? "  rw " : "  r  ")
                    | color(e.writable ? Color::Green  : Color::Yellow),
            }));
        }
        if (!any)
            rows.push_back(text("  (no pages loaded)") | dim | hcenter);
    }

    return window(text(title), vbox(std::move(rows)));
}

static Element render_tlb(const SimulatorState& sim) {
    int pid = sim.get_viewed_pid();
    std::string title = " TLB"
        + (pid >= 0 ? " — PID " + std::to_string(pid) : "") + " ";

    auto hdr = hbox({
        text("  VPN  ") | bold,
        text(" Frame ") | bold,
        text(" Writable ") | bold,
    }) | bgcolor(Color::GrayDark);

    std::vector<Element> rows = { hdr };

    if (pid < 0) {
        rows.push_back(text("  (no process)") | dim | hcenter);
    } else {
        const auto& entries = sim.get_manager().get_process(pid)
                                  .get_tlb().get_entries();
        bool any = false;
        for (const auto& e : entries) {
            if (!e.valid) continue;
            any = true;
            rows.push_back(hbox({
                text("  " + std::to_string(e.vpn))
                    | color(Color::Cyan) | size(WIDTH, EQUAL, 7),
                text(std::to_string(e.frame_index))
                    | size(WIDTH, EQUAL, 7),
                text(e.writable ? "  yes" : "  no ")
                    | color(e.writable ? Color::Green : Color::Yellow),
            }));
        }
        if (!any)
            rows.push_back(text("  (empty)") | dim | hcenter);
    }

    return window(text(title), vbox(std::move(rows)));
}

static Element render_log(const SimulatorState& sim) {
    const auto& events = sim.get_log();
    std::vector<Element> lines;

    int start = std::max(0, static_cast<int>(events.size()) - 8);
    for (int i = start; i < static_cast<int>(events.size()); ++i) {
        const auto& e = events[i];

        std::string tag;
        Color       tag_color;
        if      (e.cow_copy)                { tag = "CoW "; tag_color = Color::Magenta; }
        else if (e.page_fault && e.eviction){ tag = "EVCT"; tag_color = Color::Red;     }
        else if (e.page_fault)              { tag = "FALT"; tag_color = Color::Yellow;  }
        else                                { tag = "HIT "; tag_color = Color::Green;   }

        lines.push_back(hbox({
            text("[" + tag + "]")  | bold | color(tag_color),
            text(" P" + std::to_string(e.pid) + " "),
            text(e.is_write ? "W" : "R")
                | color(e.is_write ? Color::Red : Color::Cyan),
            text(" vpn=" + std::to_string(e.vpn)
                + "  " + hex_addr(e.virtual_address)) | dim,
        }));
    }

    if (lines.empty())
        lines.push_back(text("  (no accesses yet)") | dim);

    return window(text(" Access Log "), vbox(std::move(lines)));
}

static Element render_metrics(const SimulatorState& sim) {
    const auto& m = sim.get_manager().get_metrics();

    auto stat = [](const std::string& label, std::string val) {
        return hbox({
            text("  " + label) | dim,
            filler(),
            text(val + "  ") | bold,
        });
    };

    int tlb_total = m.tlb_hits + m.tlb_misses;
    std::string hit_rate = tlb_total > 0
        ? std::to_string(100 * m.tlb_hits / tlb_total) + "%"
        : "—";

    return window(
        text(" Metrics "),
        vbox({
            stat("Page Faults:", std::to_string(m.page_faults)),
            stat("Hits:        ", std::to_string(m.hits)),
            stat("Evictions:   ", std::to_string(m.evictions)),
            separator(),
            stat("TLB Hits:    ", std::to_string(m.tlb_hits)),
            stat("TLB Misses:  ", std::to_string(m.tlb_misses)),
            stat("Hit Rate:    ", hit_rate),
            separator(),
            stat("CoW Forks:   ", std::to_string(m.cow_forks)),
            stat("CoW Copies:  ", std::to_string(m.cow_copies)),
            stat("Avoided:     ", std::to_string(m.cow_copies_avoided)),
        })
    );
}

static Element render_controls() {
    auto key = [](const std::string& k, const std::string& desc) {
        return hbox({
            text("[" + k + "]") | bold | color(Color::Yellow),
            text(" " + desc + "  ") | dim,
        });
    };
    return hbox({
        text("  "),
        key("a", "access"),
        key("f", "fork"),
        key("r", "reset"),
        key("Tab", "next PID"),
        key("q", "quit"),
    });
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
    SimulatorState sim(PolicyType::LRU, 16);
    sim.create_process(0);

    auto screen = ScreenInteractive::Fullscreen();

    auto root = Renderer([&] {
        return vbox({
            render_header(sim),
            hbox({
                vbox({ render_frame_pool(sim) }) | flex,
                separator(),
                vbox({
                    render_page_table(sim),
                    render_tlb(sim),
                }) | flex,
            }) | flex,
            separator(),
            hbox({
                render_log(sim) | flex,
                separator(),
                render_metrics(sim) | size(WIDTH, GREATER_THAN, 32),
            }) | size(HEIGHT, LESS_THAN, 13),
            separator(),
            render_controls(),
        });
    });

    auto with_quit = CatchEvent(root, [&](Event e) {
        if (e == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    screen.Loop(with_quit);
}
