//
// M3 acceptance program: every widget in the catalog, on five pages of a
// Stack, driven by a Menu.
//
// It doubles as a worked example of the two pairings the design cares about:
// a ListView or TextArea driving a ScrollBar through `scrolled`, and the
// ScrollBar driving them back through `position_changed`.
//
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "modcurses/app.hpp"
#include "modcurses/text.hpp"
#include "modcurses/utf8.hpp"
#include "modcurses/widgets.hpp"

using namespace modcurses;

namespace {

const char* kLorem =
    "modcurses is a compact C++20 TUI library over ncursesw and PDCurses. "
    "This label has wrapping switched on, so it breaks on word boundaries and "
    "asks the layout for as many rows as it turns out to need. Resize the "
    "window and watch it reflow.";

}  // namespace

int main() {
    try {
        // Declared BEFORE the App on purpose. A TextArea holds a pointer to
        // its buffer and a connection to its signals, so the buffer has to
        // outlive the widget tree that the App destroys.
        TextBuffer buffer{
            U"This is a TextArea over a TextBuffer.\n"
            U"Arrows, Home/End, Backspace and Delete all work.\n"
            U"Tab indents here rather than moving focus; Shift-Tab leaves.\n"
            U"\n"
            U"\tThe line above starts with a real tab character.\n"
            U"The keymap is swappable - this one is Keymap::nano(),\n"
            U"so Ctrl-A and Ctrl-E jump to the ends of a line and\n"
            U"Ctrl-K cuts one.\n"};

        App app{AppInfo{"kitchen", "0.1.0", "modcurses M3 catalog demo"}};

        auto& root = app.make_root<VBox>();
        auto& title = root.emplace_child<Titlebar>("modcurses kitchen sink", "M3");
        auto& body = root.emplace_child<HBox>();
        auto& status = root.emplace_child<StatusBar>("Tab moves focus - Ctrl-C quits");

        auto& menu = body.emplace_child<Menu>();
        menu.width_hint = SizeReq::fixed(14);  // a sidebar, not half the window
        body.emplace_child<Divider>(Divider::Orientation::Vertical);
        auto& pages = body.emplace_child<Stack>();

        // ---- page 0: the small controls ------------------------------------
        auto& controls = pages.emplace_child<VBox>();
        controls.emplace_child<Label>("Buttons, checkboxes, a text field", Align::Center);
        controls.emplace_child<Divider>();
        auto& row = controls.emplace_child<HBox>();
        // A row of one-line controls. Without this, the plain Widget spacers
        // inside it expand vertically too and the row swallows the page.
        row.height_hint = SizeReq::fixed(1);
        row.emplace_child<Widget>();
        auto& greet = row.emplace_child<Button>("Greet");
        row.emplace_child<Widget>();
        auto& flash = row.emplace_child<Button>("Flash");
        row.emplace_child<Widget>();
        auto& box1 = controls.emplace_child<Checkbox>("A checkbox", true);
        controls.emplace_child<Checkbox>("Another one");
        controls.emplace_child<Label>("Type something and press Enter:");
        auto& input = controls.emplace_child<TextInput>();
        input.set_placeholder("(a TextInput, with a placeholder)");
        auto& echo = controls.emplace_child<Label>("");
        controls.emplace_child<Widget>();

        // ---- page 1: a wrapping label --------------------------------------
        auto& textpage = pages.emplace_child<VBox>();
        textpage.emplace_child<Label>("Label with wrap enabled", Align::Center);
        textpage.emplace_child<Divider>();
        auto& lorem = textpage.emplace_child<Label>(kLorem);
        lorem.set_wrap(true);
        textpage.emplace_child<Widget>();

        // ---- page 2: a list beside a scrollbar -----------------------------
        auto& listpage = pages.emplace_child<VBox>();
        listpage.emplace_child<Label>("ListView + ScrollBar", Align::Center);
        listpage.emplace_child<Divider>();
        auto& listrow = listpage.emplace_child<HBox>();
        auto& list = listrow.emplace_child<ListView>();
        auto& listbar = listrow.emplace_child<ScrollBar>();
        {
            std::vector<std::string> items;
            for (int i = 1; i <= 60; ++i) items.push_back("item " + std::to_string(i));
            list.set_items(std::move(items));
        }

        // ---- page 3: an editor beside a scrollbar --------------------------
        auto& editpage = pages.emplace_child<VBox>();
        editpage.emplace_child<Label>("TextArea over a TextBuffer", Align::Center);
        editpage.emplace_child<Divider>();
        auto& editrow = editpage.emplace_child<HBox>();
        auto& editor = editrow.emplace_child<TextArea>(buffer);
        auto& editbar = editrow.emplace_child<ScrollBar>();
        editor.keymap = Keymap::nano();

        // ---- page 4: a grid ------------------------------------------------
        auto& gridpage = pages.emplace_child<VBox>();
        gridpage.emplace_child<Label>("GridCanvas - click a cell", Align::Center);
        gridpage.emplace_child<Divider>();
        auto& gridrow = gridpage.emplace_child<HBox>();
        gridrow.emplace_child<Widget>();
        auto& grid = gridrow.emplace_child<GridCanvas>(10, 12, 2);
        gridrow.emplace_child<Widget>();
        gridpage.emplace_child<Widget>();

        const Color kPieces[] = {Color::BrightCyan,  Color::BrightYellow, Color::BrightMagenta,
                                 Color::BrightGreen, Color::BrightRed,    Color::BrightBlue};
        for (int y = 0; y < 12; ++y)
            for (int x = 0; x < 10; ++x)
                if ((x + y) % 3 == 0)
                    grid.set_cell(x, y, Glyph{U' ', bg(kPieces[((x + y) / 3) % 6])});

        // Two cells per column plus a click hit-test is the whole of the
        // glyph-paint demo, and the same primitive a Tetris board would use.
        grid.focus_policy = FocusPolicy::Click;
        grid.on_mouse_hook = [&grid](const MouseEvent& ev) {
            if (ev.action != MouseEvent::Action::Press) return false;
            const auto cell = grid.cell_at(ev.pos);
            if (!cell) return false;
            const bool lit = grid.cell(cell->x, cell->y).style.bg != Color::Default;
            grid.set_cell(cell->x, cell->y,
                          lit ? Glyph{U' ', {}} : Glyph{U' ', bg(Color::BrightWhite)});
            return true;
        };

        // ---- wiring --------------------------------------------------------

        menu.add_entry("Controls", [&] { pages.set_active(0); });
        menu.add_entry("Wrapping", [&] { pages.set_active(1); });
        menu.add_entry("List", [&] { pages.set_active(2); });
        menu.add_entry("Editor", [&] { pages.set_active(3); });
        menu.add_entry("Grid", [&] { pages.set_active(4); });
        menu.add_entry("Quit", [&] { app.quit(0); });

        auto on_page = menu.selection_changed.connect(
            [&](int i) { title.set_hint("M3 - " + menu.item(i)); });

        int greets = 0;
        auto on_greet = greet.pressed.connect([&] {
            echo.set_text("Greeted " + std::to_string(++greets) +
                          (greets == 1 ? " time" : " times"));
        });
        auto on_flash =
            flash.pressed.connect([&] { status.flash("Flashed!", std::chrono::seconds{2}); });
        auto on_box = box1.toggled.connect([&](bool on) {
            status.flash(on ? "Checked" : "Unchecked", std::chrono::seconds{1});
        });
        auto on_submit = input.submitted.connect(
            [&](const std::u32string& s) { echo.set_text("You typed: " + utf8_encode(s)); });

        // The ScrollBar pairing, in both directions, for both scrollers.
        auto list_scrolled = list.scrolled.connect([&](int top, int total) {
            listbar.set_range(total, list.visible_rows());
            listbar.set_position(top);
        });
        auto list_bar = listbar.position_changed.connect([&](int top) { list.scroll_to(top); });

        auto edit_scrolled = editor.scrolled.connect([&](int top, int total) {
            editbar.set_range(total, editor.visible_lines());
            editbar.set_position(top);
        });
        auto edit_bar =
            editbar.position_changed.connect([&](int top) { editor.scroll_to({0, top}); });

        menu.take_focus();
        return app.run();
    } catch (const TerminalError& e) {
        std::fprintf(stderr, "modcurses: %s\n", e.what());
        return 1;
    }
}
