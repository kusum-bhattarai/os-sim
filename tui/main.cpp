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
#include "comparison.h"
#include "policy/clock.h"

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

// Returns parsed VPN list, or empty vector on any parse error.
static std::vector<int> parse_vpn_sequence(const std::string& s) {
    std::vector<int> vpns;
    std::istringstream ss(s);
    std::string token;
    while (ss >> token) {
        try { vpns.push_back(std::stoi(token)); }
        catch (...) { return {}; }
    }
    return vpns;
}

static Element event_tag(const AccessEvent& e) {
    std::string label;
    Color       col;
    if      (e.cow_copy)                 { label = "CoW "; col = Color::Magenta; }
    else if (e.page_fault && e.eviction) { label = "EVCT"; col = Color::Red;     }
    else if (e.page_fault)               { label = "FALT"; col = Color::Yellow;  }
    else if (e.tlb_hit)                  { label = "TLB "; col = Color::Green;   }
    else                                 { label = "PT  "; col = Color::Cyan;    }
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
    const auto& pool  = sim.get_manager().get_frame_pool();
    const int   cap   = pool.get_capacity();
    const int   cols  = 8;
    const Clock* clock = sim.get_clock_policy();

    // Build CLOCK state lookup: physical frame index → (ref bit, is hand)
    std::unordered_map<int, bool> clock_refs;
    int clock_hand_frame = -1;
    if (clock) {
        const auto& entries = clock->get_entries();
        for (const auto& e : entries) clock_refs[e.frame_index] = e.referenced;
        if (!entries.empty())
            clock_hand_frame = entries[clock->get_hand()].frame_index;
    }

    // helper: 4-display-char label for a CLOCK cell (">N ○" / " NN●" etc.)
    auto clock_label = [](bool is_hand, int idx, bool ref) -> std::string {
        std::string p   = is_hand ? ">" : " ";
        std::string bit = ref     ? "●" : "○";
        return idx < 10
            ? p + std::to_string(idx) + " " + bit
            : p + std::to_string(idx)       + bit;
    };

    std::vector<Element> rows;
    for (int r = 0; r * cols < cap; ++r) {
        std::vector<Element> cells;
        for (int c = 0; c < cols && r * cols + c < cap; ++c) {
            int idx = r * cols + c;
            const Frame& f = pool.peek_frame(idx);
            Element cell;

            if (!f.in_use) {
                std::string label = " " + std::string(idx < 10 ? " " : "")
                                  + std::to_string(idx) + " ";
                cell = text(label) | dim | border;

            } else if (clock && clock_refs.count(idx)) {
                bool is_hand = (idx == clock_hand_frame);
                bool ref     = clock_refs.at(idx);
                std::string label = clock_label(is_hand, idx, ref);
                if (is_hand)
                    cell = text(label) | bold | color(Color::White)
                         | bgcolor(Color::Red) | border;
                else if (ref)
                    cell = text(label) | bold | color(Color::Green) | border;
                else
                    cell = text(label) | bold | color(Color::Yellow) | border;

            } else {
                // FIFO / LRU — show ref count for CoW-shared frames
                std::string label;
                if (f.ref_count > 1)
                    label = std::string(idx < 10 ? " " : "") + std::to_string(idx)
                          + "×" + std::to_string(f.ref_count);
                else
                    label = " " + std::string(idx < 10 ? " " : "")
                          + std::to_string(idx) + " ";

                cell = (f.ref_count > 1)
                    ? text(label) | bold | color(Color::Yellow) | border
                    : text(label) | bold | color(Color::Green)  | border;
            }
            cells.push_back(std::move(cell));
        }
        rows.push_back(hbox(std::move(cells)));
    }

    rows.push_back(separator());
    if (clock) {
        rows.push_back(hbox({
            text("  "),
            text(">") | bold | color(Color::White) | bgcolor(Color::Red),
            text(" hand  ") | dim,
            text("●") | color(Color::Green),  text(" ref=1  ") | dim,
            text("○") | color(Color::Yellow), text(" ref=0 (candidate)") | dim,
        }));
    } else {
        rows.push_back(hbox({
            text("  "),
            text("□") | dim,        text(" free  ") | dim,
            text("■") | color(Color::Green),  text(" used  ") | dim,
            text("◉") | color(Color::Yellow), text(" shared (CoW)") | dim,
        }));
    }

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

static constexpr int LOG_VISIBLE = 8;

static Element render_log(const SimulatorState& sim, int scroll_offset) {
    const auto& events = sim.get_log();
    const int total  = static_cast<int>(events.size());
    const int end    = total - scroll_offset;
    const int start  = std::max(0, end - LOG_VISIBLE);

    std::vector<Element> lines;
    for (int i = start; i < end; ++i) {
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

    // title shows position in history; arrow hint appears when not at bottom
    std::string title = " Access Log ";
    if (total > 0)
        title += std::to_string(std::max(1, start + 1)) + "-"
               + std::to_string(std::max(0, end)) + "/"
               + std::to_string(total) + " ";
    if (scroll_offset > 0) title += "↑PgUp  PgDn↓ ";

    return window(text(title), vbox(std::move(lines)));
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
        key("s", "sequence"),
        key("n", "new proc"),
        key("f", "fork"),
        key("p", "presets"),
        key("c", "config"),
        key("r", "reset"),
        key("Tab", "next PID"),
        key("PgUp/Dn", "scroll log"),
        key("q", "quit"),
    });
}

// ── presets ───────────────────────────────────────────────────────────────────

struct Preset {
    std::string                          name;
    std::string                          description;
    PolicyType                           policy;
    int                                  num_frames;
    std::function<void(SimulatorState&)> run;
};

static const std::vector<Preset>& get_presets() {
    static const std::vector<Preset> list = {
        {
            "Temporal Locality",
            "4 frames, 4 pages: one cold-start round then all hits. "
            "Shows the payoff of temporal locality — the working set "
            "fits in memory so every repeat access is free.",
            PolicyType::LRU, 4,
            [](SimulatorState& s) {
                s.create_process(0);
                for (int round = 0; round < 6; ++round)
                    for (int vpn = 0; vpn < 4; ++vpn)
                        s.step(0, vpn * PAGE_SIZE, false);
            }
        },
        {
            "Thrashing",
            "4 frames, 8 pages: the working set is twice the frame count. "
            "Every access evicts a page that will be needed again shortly — "
            "100% fault rate, zero hits.",
            PolicyType::LRU, 4,
            [](SimulatorState& s) {
                s.create_process(0);
                for (int round = 0; round < 3; ++round)
                    for (int vpn = 0; vpn < 8; ++vpn)
                        s.step(0, vpn * PAGE_SIZE, false);
            }
        },
        {
            "CoW: Read-Heavy Fork",
            "PID 0 loads 4 pages then forks to PID 1. Both processes "
            "read all shared pages repeatedly. Zero copies made — the "
            "fork overhead is deferred and never materialises.",
            PolicyType::LRU, 8,
            [](SimulatorState& s) {
                s.create_process(0);
                for (int vpn = 0; vpn < 4; ++vpn)
                    s.step(0, vpn * PAGE_SIZE, false);
                s.fork(0, 1);
                for (int round = 0; round < 4; ++round)
                    for (int vpn = 0; vpn < 4; ++vpn) {
                        s.step(0, vpn * PAGE_SIZE, false);
                        s.step(1, vpn * PAGE_SIZE, false);
                    }
            }
        },
        {
            "CoW: Write-Heavy Fork",
            "PID 0 loads 4 pages then forks to PID 1. Both processes "
            "immediately write to every shared page. Each write triggers "
            "a private copy — fork was expensive.",
            PolicyType::LRU, 8,
            [](SimulatorState& s) {
                s.create_process(0);
                for (int vpn = 0; vpn < 4; ++vpn)
                    s.step(0, vpn * PAGE_SIZE, false);
                s.fork(0, 1);
                for (int vpn = 0; vpn < 4; ++vpn) {
                    s.step(0, vpn * PAGE_SIZE, true);
                    s.step(1, vpn * PAGE_SIZE, true);
                }
            }
        },
        {
            "CLOCK: Second Chance",
            "8 frames, 14 pages. Fills all frames, then re-accesses "
            "even-numbered pages (setting ref bits). New pages trigger "
            "eviction: unreferenced frames go first; referenced ones "
            "survive one sweep before eviction.",
            PolicyType::CLOCK, 8,
            [](SimulatorState& s) {
                s.create_process(0);
                for (int vpn = 0; vpn < 8; ++vpn)
                    s.step(0, vpn * PAGE_SIZE, false);
                for (int vpn : {0, 2, 4, 6})
                    s.step(0, vpn * PAGE_SIZE, false);
                for (int vpn = 8; vpn < 14; ++vpn)
                    s.step(0, vpn * PAGE_SIZE, false);
            }
        },
    };
    return list;
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
    SimulatorState sim(PolicyType::LRU, 16);
    sim.create_process(0);

    auto screen = ScreenInteractive::Fullscreen();
    int log_scroll = 0; // 0 = bottom (latest); increases scrolling up into history

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
            log_scroll   = 0; // snap log back to latest entry
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

    // ── Fork modal state ─────────────────────────────────────────────────────
    bool show_fork = false;
    std::string fork_parent_str;
    std::string fork_child_str;
    std::string fork_error;

    auto fork_parent_input = Input(&fork_parent_str, "parent PID");
    auto fork_child_input  = Input(&fork_child_str,  "child PID");

    auto do_fork_btn = Button("  Fork  ", [&] {
        int parent = -1, child = -1;
        try { parent = std::stoi(fork_parent_str); } catch (...) {}
        try { child  = std::stoi(fork_child_str);  } catch (...) {}
        if (parent < 0) { fork_error = "Invalid parent PID"; return; }
        if (child  < 0) { fork_error = "Invalid child PID";  return; }
        try {
            sim.fork(parent, child);
            show_fork  = false;
            fork_error.clear();
        } catch (const std::exception& ex) {
            fork_error = ex.what();
        }
    });

    auto fork_cancel = Button(" Cancel ", [&] {
        show_fork  = false;
        fork_error.clear();
    });

    auto fork_body = Container::Vertical({
        fork_parent_input,
        fork_child_input,
        Container::Horizontal({do_fork_btn, fork_cancel}),
    });

    auto fork_modal = Renderer(fork_body, [&] {
        std::vector<Element> rows = {
            text(" Fork Process ") | bold | hcenter,
            separator(),
            hbox({text("  Parent PID: "), fork_parent_input->Render() | size(WIDTH, EQUAL, 10)}),
            hbox({text("  Child PID:  "), fork_child_input->Render()  | size(WIDTH, EQUAL, 10)}),
            text("  Shares all frames via CoW.") | dim,
        };
        if (!fork_error.empty())
            rows.push_back(text("  " + fork_error) | color(Color::Red));
        rows.push_back(separator());
        rows.push_back(
            hbox({do_fork_btn->Render(), text("  "), fork_cancel->Render()}) | hcenter);
        return vbox(std::move(rows))
            | border | size(WIDTH, EQUAL, 38) | size(HEIGHT, EQUAL, 11);
    });

    // ── Presets modal state ──────────────────────────────────────────────────
    bool show_presets   = false;
    int  preset_idx     = 0;
    std::string preset_error;

    const auto& presets = get_presets();
    std::vector<std::string> preset_names;
    for (const auto& p : presets) preset_names.push_back(p.name);

    auto do_run = [&] {
        try {
            const Preset& p = presets[preset_idx];
            sim.reset(p.policy, p.num_frames);
            p.run(sim);
            log_scroll   = 0;
            show_presets = false;
            preset_error.clear();
        } catch (const std::exception& ex) {
            preset_error = ex.what();
        }
    };

    MenuOption menu_opt;
    menu_opt.on_enter = do_run; // Enter on a menu item runs it directly
    auto preset_menu  = Menu(&preset_names, &preset_idx, menu_opt);
    auto run_btn      = Button("  Run  ", do_run);
    auto presets_cancel = Button(" Cancel ", [&] {
        show_presets = false;
        preset_error.clear();
    });

    auto presets_body = Container::Vertical({
        preset_menu,
        Container::Horizontal({run_btn, presets_cancel}),
    });

    auto presets_modal = Renderer(presets_body, [&] {
        std::vector<Element> rows = {
            text(" Presets ") | bold | hcenter,
            separator(),
            preset_menu->Render()
                | size(HEIGHT, EQUAL, static_cast<int>(presets.size())),
            separator(),
            paragraph(presets[preset_idx].description)
                | size(HEIGHT, LESS_THAN, 5) | dim,
        };
        if (!preset_error.empty())
            rows.push_back(text("  " + preset_error) | color(Color::Red));
        rows.push_back(separator());
        rows.push_back(
            hbox({run_btn->Render(), text("  "), presets_cancel->Render()}) | hcenter);
        return vbox(std::move(rows))
            | border | size(WIDTH, EQUAL, 50) | size(HEIGHT, EQUAL, 17);
    });

    // ── Sequence modal state ─────────────────────────────────────────────────
    bool show_seq = false;
    std::string seq_pid_str;
    std::string seq_input_str;
    int seq_mode = 0; // 0=Read, 1=Write
    std::string seq_result;
    std::string seq_error;

    auto do_seq_run = [&] {
        int pid = -1;
        try { pid = std::stoi(seq_pid_str); } catch (...) {}
        if (pid < 0) { seq_error = "Invalid PID"; seq_result.clear(); return; }

        auto vpns = parse_vpn_sequence(seq_input_str);
        if (vpns.empty()) { seq_error = "No valid VPNs in sequence"; seq_result.clear(); return; }

        bool is_write = (seq_mode == 1);
        int faults = 0, tlb_hits = 0, evictions = 0;
        try {
            for (int vpn : vpns) {
                auto evt = sim.step(pid, vpn * PAGE_SIZE, is_write);
                log_scroll = 0;
                if (evt.page_fault) ++faults;
                if (evt.tlb_hit)    ++tlb_hits;
                if (evt.eviction)   ++evictions;
            }
            seq_error.clear();
            seq_result = std::to_string(vpns.size()) + " accesses — "
                       + std::to_string(faults)    + " faults, "
                       + std::to_string(tlb_hits)  + " TLB hits, "
                       + std::to_string(evictions) + " evictions";
        } catch (const std::exception& ex) {
            seq_error = ex.what();
            seq_result.clear();
        }
    };

    auto seq_pid_input  = Input(&seq_pid_str, "PID");

    InputOption seq_opt;
    seq_opt.multiline = false;
    seq_opt.on_enter  = [&] { do_seq_run(); };
    auto seq_seq_input  = Input(&seq_input_str, "e.g. 0 1 2 3 0 1 4", seq_opt);

    std::vector<std::string> seq_mode_labels = {"Read", "Write"};
    auto seq_mode_radio = Radiobox(&seq_mode_labels, &seq_mode);

    auto seq_run_btn = Button("  Run  ", [&] { do_seq_run(); });
    auto seq_cancel  = Button(" Close ", [&] {
        show_seq = false;
        seq_error.clear();
        seq_result.clear();
    });

    auto seq_body = Container::Vertical({
        seq_pid_input,
        seq_seq_input,
        seq_mode_radio,
        Container::Horizontal({seq_run_btn, seq_cancel}),
    });

    auto seq_modal = Renderer(seq_body, [&] {
        std::vector<Element> rows = {
            text(" Sequence Run ") | bold | hcenter,
            separator(),
            hbox({text("  PID:      "), seq_pid_input->Render()  | size(WIDTH, EQUAL, 10)}),
            hbox({text("  Sequence: "), seq_seq_input->Render()  | flex}),
            hbox({text("  Mode:     "), seq_mode_radio->Render()}),
            separator(),
        };
        if (!seq_result.empty())
            rows.push_back(text("  " + seq_result) | color(Color::Green));
        if (!seq_error.empty())
            rows.push_back(text("  " + seq_error) | color(Color::Red));
        rows.push_back(separator());
        rows.push_back(
            hbox({seq_run_btn->Render(), text("  "), seq_cancel->Render()}) | hcenter);
        rows.push_back(text("  Enter on sequence field also runs") | dim);
        return vbox(std::move(rows))
            | border | size(WIDTH, EQUAL, 54) | size(HEIGHT, EQUAL, 14);
    });

    // ── Compare modal state ──────────────────────────────────────────────────
    bool show_compare = false;
    std::string cmp_seq_str;
    std::string cmp_frames_str = std::to_string(sim.get_num_frames());
    bool cmp_sweep = false;
    std::string cmp_error;
    // Holds rendered results; rebuilt on each Run press.
    std::vector<PolicyResult> cmp_results;
    bool cmp_ran = false;

    auto do_compare_run = [&] {
        auto vpns = parse_vpn_sequence(cmp_seq_str);
        if (vpns.empty()) { cmp_error = "No valid VPNs in sequence"; cmp_results.clear(); cmp_ran = false; return; }

        int pid = sim.get_viewed_pid() >= 0 ? sim.get_viewed_pid() : 0;

        int frames = 0;
        try { frames = std::stoi(cmp_frames_str); } catch (...) {}
        if (frames < 1 || frames > 256) { cmp_error = "Frames must be 1-256"; cmp_results.clear(); cmp_ran = false; return; }

        try {
            if (cmp_sweep) {
                int sweep_max = std::min(frames, 8);
                cmp_results = run_frame_sweep(vpns, pid, false, 1, sweep_max);
            } else {
                cmp_results = run_comparison(vpns, pid, false, frames);
            }
            cmp_error.clear();
            cmp_ran = true;
        } catch (const std::exception& ex) {
            cmp_error = ex.what();
            cmp_results.clear();
            cmp_ran = false;
        }
    };

    InputOption cmp_seq_opt;
    cmp_seq_opt.multiline = false;
    cmp_seq_opt.on_enter  = [&] { do_compare_run(); };
    auto cmp_seq_input    = Input(&cmp_seq_str, "e.g. 0 1 2 3 0 1 4", cmp_seq_opt);
    auto cmp_frames_input = Input(&cmp_frames_str, "count");

    auto cmp_sweep_cb = Checkbox("Frame sweep (1–N frames, shows Belady's anomaly)", &cmp_sweep);

    auto cmp_run_btn  = Button("  Run  ", [&] { do_compare_run(); });
    auto cmp_cancel   = Button(" Close ", [&] {
        show_compare = false;
        cmp_error.clear();
        cmp_results.clear();
        cmp_ran = false;
    });

    auto cmp_body = Container::Vertical({
        cmp_seq_input,
        cmp_frames_input,
        cmp_sweep_cb,
        Container::Horizontal({cmp_run_btn, cmp_cancel}),
    });

    auto cmp_modal = Renderer(cmp_body, [&] {
        std::vector<Element> rows = {
            text(" Algorithm Comparison ") | bold | hcenter,
            separator(),
            hbox({text("  Sequence: "), cmp_seq_input->Render()    | flex}),
            hbox({text("  Frames:   "), cmp_frames_input->Render() | size(WIDTH, EQUAL, 8)}),
            hbox({text("  "),           cmp_sweep_cb->Render()}),
            separator(),
        };

        if (cmp_ran && !cmp_results.empty()) {
            if (cmp_sweep) {
                int frames = 0;
                try { frames = std::stoi(cmp_frames_str); } catch (...) { frames = 8; }
                int sweep_max = std::min(frames, 8);
                rows.push_back(render_sweep_table(cmp_results, 1, sweep_max));
            } else {
                rows.push_back(render_comparison_table(cmp_results));
            }
            rows.push_back(separator());
        }

        if (!cmp_error.empty())
            rows.push_back(text("  " + cmp_error) | color(Color::Red));

        rows.push_back(
            hbox({cmp_run_btn->Render(), text("  "), cmp_cancel->Render()}) | hcenter);
        rows.push_back(text("  Sequence pre-filled from [s] if run first  |  Enter on sequence runs") | dim);

        return vbox(std::move(rows))
            | border | size(WIDTH, EQUAL, 58);
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
                render_log(sim, log_scroll) | flex,
                separator(),
                render_metrics(sim) | size(WIDTH, GREATER_THAN, 32),
            }) | size(HEIGHT, LESS_THAN, 13),
            separator(),
            render_controls(),
        });
    });

    // ── Compose: chain modals over the main view ─────────────────────────────
    auto app = Modal(
        Modal(
            Modal(
                Modal(
                    Modal(
                        Modal(main_view, config_modal, &show_config),
                        np_modal, &show_new_proc
                    ),
                    fork_modal, &show_fork
                ),
                presets_modal, &show_presets
            ),
            seq_modal, &show_seq
        ),
        cmp_modal, &show_compare
    ),
        access_modal, &show_access
    );

    // ── Global event handling ────────────────────────────────────────────────
    auto with_events = CatchEvent(app, [&](Event e) {
        if (e == Event::Escape) {
            if (show_access)   { show_access   = false; access_error.clear(); last_access.reset(); return true; }
            if (show_seq)      { show_seq      = false; seq_error.clear();    seq_result.clear();  return true; }
            if (show_presets)  { show_presets  = false; preset_error.clear();                      return true; }
            if (show_fork)     { show_fork     = false; fork_error.clear();                        return true; }
            if (show_new_proc) { show_new_proc = false; new_proc_error.clear();                    return true; }
            if (show_config)   { show_config   = false; config_error.clear();                      return true; }
        }
        if (show_config || show_new_proc || show_fork || show_presets || show_seq || show_access) return false;

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
        if (e == Event::Character('p')) {
            preset_idx   = 0;
            preset_error.clear();
            show_presets = true;
            return true;
        }
        if (e == Event::Character('f')) {
            fork_parent_str = sim.get_viewed_pid() >= 0
                                  ? std::to_string(sim.get_viewed_pid()) : "";
            fork_child_str  = "";
            fork_error.clear();
            show_fork       = true;
            return true;
        }
        if (e == Event::Character('s')) {
            seq_pid_str = sim.get_viewed_pid() >= 0
                              ? std::to_string(sim.get_viewed_pid()) : "0";
            seq_error.clear();
            seq_result.clear();
            show_seq    = true;
            return true;
        }
        if (e == Event::Character('r')) {
            sim.reset(sim.get_policy_type(), sim.get_num_frames());
            sim.create_process(0);
            log_scroll = 0;
            return true;
        }
        if (e == Event::Tab) { sim.cycle_viewed_pid(); return true; }
        if (e == Event::PageUp) {
            const int total = static_cast<int>(sim.get_log().size());
            log_scroll = std::min(log_scroll + LOG_VISIBLE, std::max(0, total - LOG_VISIBLE));
            return true;
        }
        if (e == Event::PageDown) {
            log_scroll = std::max(log_scroll - LOG_VISIBLE, 0);
            return true;
        }
        return false;
    });

    screen.Loop(with_events);
}
