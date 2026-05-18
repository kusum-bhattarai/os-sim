#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

int main() {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();

    auto root = Renderer([&] {
        return vbox({
            text("os-sim") | bold | hcenter,
            text("Virtual Memory Simulator") | dim | hcenter,
            separator(),
            text("Press 'q' to quit") | hcenter,
        }) | border | center;
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
