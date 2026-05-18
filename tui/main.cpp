#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

// ── placeholder renderers (wired to simulator in later steps) ────────────────

static Element render_header() {
    return hbox({
        text("  os-sim") | bold,
        text("  Virtual Memory Simulator") | dim,
        filler(),
        text("Policy: ") | dim,
        text("LRU") | bold | color(Color::Cyan),
        text("  Frames: ") | dim,
        text("16") | bold,
        text("  "),
    }) | color(Color::White) | bgcolor(Color::Blue);
}

static Element render_frame_pool(int num_frames = 16) {
    const int cols = 8;
    std::vector<Element> rows;

    for (int r = 0; r * cols < num_frames; ++r) {
        std::vector<Element> cells;
        for (int c = 0; c < cols && r * cols + c < num_frames; ++c) {
            int idx = r * cols + c;
            // placeholder: all frames free
            cells.push_back(
                text(" " + std::string(idx < 10 ? " " : "") + std::to_string(idx) + " ")
                | dim | border
            );
        }
        rows.push_back(hbox(std::move(cells)));
    }

    rows.push_back(separator());
    rows.push_back(hbox({
        text("  "),
        text("□") | dim,
        text(" free  ") | dim,
        text("■") | color(Color::Green),
        text(" used  ") | dim,
        text("◉") | color(Color::Yellow),
        text(" shared (CoW)") | dim,
    }));

    return window(text(" Frame Pool "), vbox(std::move(rows)));
}

static Element render_page_table() {
    auto header_row = hbox({
        text("  VPN  ") | bold,
        text(" Frame ") | bold,
        text(" Dirty ") | bold,
        text(" Write ") | bold,
    }) | bgcolor(Color::GrayDark);

    return window(
        text(" Page Table — PID 0 "),
        vbox({
            header_row,
            text("  (no pages loaded)") | dim | hcenter,
        })
    );
}

static Element render_tlb() {
    auto header_row = hbox({
        text("  VPN  ") | bold,
        text(" Frame ") | bold,
        text(" Writable ") | bold,
    }) | bgcolor(Color::GrayDark);

    return window(
        text(" TLB — PID 0 "),
        vbox({
            header_row,
            text("  (empty)") | dim | hcenter,
        })
    );
}

static Element render_log() {
    return window(
        text(" Access Log "),
        vbox({
            text("  (no accesses yet)") | dim,
        })
    );
}

static Element render_metrics() {
    auto stat = [](const std::string& label, const std::string& val) {
        return hbox({
            text("  " + label) | dim,
            filler(),
            text(val + "  ") | bold,
        });
    };

    return window(
        text(" Metrics "),
        vbox({
            stat("Page Faults:", "0"),
            stat("Hits:        ", "0"),
            stat("Evictions:   ", "0"),
            separator(),
            stat("TLB Hits:    ", "0"),
            stat("TLB Misses:  ", "0"),
            stat("Hit Rate:    ", "—"),
            separator(),
            stat("CoW Forks:   ", "0"),
            stat("CoW Copies:  ", "0"),
            stat("Avoided:     ", "0"),
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
    auto screen = ScreenInteractive::Fullscreen();

    auto root = Renderer([&] {
        return vbox({
            render_header(),
            hbox({
                vbox({
                    render_frame_pool(),
                }) | flex,
                separator(),
                vbox({
                    render_page_table(),
                    render_tlb(),
                }) | flex,
            }) | flex,
            separator(),
            hbox({
                render_log() | flex,
                separator(),
                render_metrics() | size(WIDTH, GREATER_THAN, 32),
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
