#include <algorithm>
#include <iomanip>
#include <optional>
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

static int parse_address(const std::string& s) {
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        return std::stoi(s, nullptr, 16);
    return std::stoi(s);
}

static Element event_tag(const AccessEvent& e) {
    std::string label;
    Color       col;
    if      (e.cow_copy)                 { label = "CoW "; col = Color::Magenta; }
    else if (e.page_fault && e.eviction) { label = "EVCT"; col = Color::Red;     }
    else if (e.page_fault)               { label = "FALT"; col = Color::Yellow;  }
    else                                 { label = "HIT "; col = Color::Green;   }
    return text("[" + label + "]") | bold | color(col);
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
        text("□") | dim,       text(" free  ") | dim,
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
                    | color(e.dirty    ? Color::Red   : Color::GrayLight),
                text(e.writable ? "  rw " : "  r  ")
                    | color(e.writable ? Color::Green : Color::Yellow),
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
        lines.push_back(hbox({
            event_tag(e),
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
        key("n", "new proc"),
        key("f", "fork"),
        key("c", "config"),
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

    // ── Config modal state ───────────────────────────────────────────────────
    bool show_config = false;
    int  algo_selected = static_cast<int>(sim.get_policy_type());
    std::string frames_str = std::to_string(sim.get_num_frames());
    std::string config_error;

    std::vector<std::string> algo_labels = {"FIFO", "LRU", "CLOCK"};
    auto algo_radio   = Radiobox(&algo_labels, &algo_selected);
    auto frames_input = Input(&frames_str, "count");

    auto apply_btn = Button("  Apply  ", [&] {
        int f = 0;
        try { f = std::stoi(frames_str); } catch (...) {}
        if (f < 1 || f > 256) { config_error = "Frames must be 1-256"; return; }
        sim.reset(static_cast<PolicyType>(algo_selected), f);
        sim.create_process(0);
        show_config = false;
        config_error.clear();
    });

    auto cfg_cancel = Button(" Cancel ", [&] {
        show_config = false;
        config_error.clear();
    });

    auto config_body = Container::Vertical({
        algo_radio,
        frames_input,
        Container::Horizontal({apply_btn, cfg_cancel}),
    });

    auto config_modal = Renderer(config_body, [&] {
        std::vector<Element> rows = {
            text(" Configuration ") | bold | hcenter,
            separator(),
            text("  Algorithm") | dim,
            algo_radio->Render(),
            separator(),
            hbox({text("  Frames: "), frames_input->Render() | size(WIDTH, EQUAL, 8)}),
        };
        if (!config_error.empty())
            rows.push_back(text("  " + config_error) | color(Color::Red));
        rows.push_back(separator());
        rows.push_back(
            hbox({apply_btn->Render(), text("  "), cfg_cancel->Render()}) | hcenter);
        return vbox(std::move(rows))
            | border | size(WIDTH, EQUAL, 34) | size(HEIGHT, EQUAL, 14);
    });

    // ── New-process modal state ──────────────────────────────────────────────
    bool show_new_proc = false;
    std::string new_pid_str;
    std::string new_proc_error;

    auto pid_input  = Input(&new_pid_str, "e.g. 1");

    auto create_btn = Button("  Create  ", [&] {
        int pid = -1;
        try { pid = std::stoi(new_pid_str); } catch (...) {}
        if (pid < 0) { new_proc_error = "PID must be >= 0"; return; }
        try {
            sim.create_process(pid);
            show_new_proc = false;
            new_proc_error.clear();
        } catch (const std::exception& ex) {
            new_proc_error = ex.what();
        }
    });

    auto np_cancel = Button(" Cancel ", [&] {
        show_new_proc = false;
        new_proc_error.clear();
    });

    auto np_body = Container::Vertical({
        pid_input,
        Container::Horizontal({create_btn, np_cancel}),
    });

    auto np_modal = Renderer(np_body, [&] {
        std::vector<Element> rows = {
            text(" New Process ") | bold | hcenter,
            separator(),
            hbox({text("  PID: "), pid_input->Render() | size(WIDTH, EQUAL, 10)}),
        };
        if (!new_proc_error.empty())
            rows.push_back(text("  " + new_proc_error) | color(Color::Red));
        rows.push_back(separator());
        rows.push_back(
            hbox({create_btn->Render(), text("  "), np_cancel->Render()}) | hcenter);
        return vbox(std::move(rows))
            | border | size(WIDTH, EQUAL, 34) | size(HEIGHT, EQUAL, 9);
    });

    // ── Access modal state ───────────────────────────────────────────────────
    bool show_access = false;
    std::string access_pid_str;
    std::string access_addr_str;
    int access_mode = 0; // 0 = Read, 1 = Write
    std::string access_error;
    std::optional<AccessEvent> last_access;

    // execute one step — shared by button and on_enter
    auto do_access = [&] {
        int pid = -1, addr = -1;
        try { pid = std::stoi(access_pid_str); } catch (...) {}
        if (pid < 0) { access_error = "Invalid PID"; return; }

        try { addr = parse_address(access_addr_str); } catch (...) {}
        if (addr < 0) { access_error = "Invalid address"; return; }

        try {
            last_access  = sim.step(pid, addr, access_mode == 1);
            access_error.clear();
        } catch (const std::exception& ex) {
            access_error = ex.what();
        }
    };

    auto access_pid_input = Input(&access_pid_str, "PID");

    InputOption addr_opt;
    addr_opt.multiline = false;
    addr_opt.on_enter  = [&] { do_access(); };
    auto access_addr_input = Input(&access_addr_str, "decimal or 0x…", addr_opt);

    std::vector<std::string> mode_labels = {"Read", "Write"};
    auto access_mode_radio = Radiobox(&mode_labels, &access_mode);

    auto exec_btn  = Button("  Step  ", [&] { do_access(); });
    auto acc_close = Button(" Close ", [&] {
        show_access  = false;
        access_error.clear();
        last_access.reset();
    });

    auto access_body = Container::Vertical({
        access_pid_input,
        access_addr_input,
        access_mode_radio,
        Container::Horizontal({exec_btn, acc_close}),
    });

    auto access_modal = Renderer(access_body, [&] {
        std::vector<Element> rows = {
            text(" Memory Access ") | bold | hcenter,
            separator(),
            hbox({text("  PID:     "), access_pid_input->Render()  | size(WIDTH, EQUAL, 14)}),
            hbox({text("  Address: "), access_addr_input->Render() | size(WIDTH, EQUAL, 14)}),
            hbox({text("  Mode:    "), access_mode_radio->Render()}),
            separator(),
        };

        // result of last step
        if (last_access.has_value()) {
            const auto& e = *last_access;
            const auto& pt_entries = sim.get_manager().get_process(e.pid)
                                         .get_page_table().get_entries();
            int frame = -1;
            if (auto it = pt_entries.find(e.vpn); it != pt_entries.end())
                frame = it->second.frame_index;

            rows.push_back(hbox({
                text("  Result: "),
                event_tag(e),
                text("  vpn=" + std::to_string(e.vpn)
                    + "  frame=" + (frame >= 0 ? std::to_string(frame) : "—")) | dim,
            }));
        } else {
            rows.push_back(text("  Result: —") | dim);
        }

        if (!access_error.empty())
            rows.push_back(text("  Error:  " + access_error) | color(Color::Red));

        rows.push_back(separator());
        rows.push_back(
            hbox({exec_btn->Render(), text("  "), acc_close->Render()}) | hcenter);
        rows.push_back(text("  Enter on address field also steps") | dim);

        return vbox(std::move(rows))
            | border | size(WIDTH, EQUAL, 42) | size(HEIGHT, EQUAL, 14);
    });

    // ── Main view ────────────────────────────────────────────────────────────
    auto main_view = Renderer([&] {
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

    // ── Compose: chain three modals over the main view ───────────────────────
    auto app = Modal(
        Modal(
            Modal(main_view, config_modal, &show_config),
            np_modal, &show_new_proc
        ),
        access_modal, &show_access
    );

    // ── Global event handling ────────────────────────────────────────────────
    auto with_events = CatchEvent(app, [&](Event e) {
        if (e == Event::Escape) {
            if (show_access)   { show_access = false; access_error.clear(); last_access.reset(); return true; }
            if (show_new_proc) { show_new_proc = false; new_proc_error.clear(); return true; }
            if (show_config)   { show_config   = false; config_error.clear();   return true; }
        }
        if (show_config || show_new_proc || show_access) return false;

        if (e == Event::Character('q')) { screen.ExitLoopClosure()(); return true; }
        if (e == Event::Character('a')) {
            access_pid_str  = sim.get_viewed_pid() >= 0
                                  ? std::to_string(sim.get_viewed_pid()) : "0";
            access_addr_str = "";
            access_error.clear();
            last_access.reset();
            show_access     = true;
            return true;
        }
        if (e == Event::Character('c')) {
            algo_selected = static_cast<int>(sim.get_policy_type());
            frames_str    = std::to_string(sim.get_num_frames());
            config_error.clear();
            show_config   = true;
            return true;
        }
        if (e == Event::Character('n')) {
            new_pid_str    = "";
            new_proc_error.clear();
            show_new_proc  = true;
            return true;
        }
        if (e == Event::Tab) { sim.cycle_viewed_pid(); return true; }
        return false;
    });

    screen.Loop(with_events);
}
