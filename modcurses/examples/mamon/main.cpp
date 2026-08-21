//
// mamon - a nano-like editor on modcurses.
//
// The screen furniture, the key map and the wording of the status messages
// all follow GNU nano (9.2) as closely as this library allows, because the
// point of the exercise is that a nano-shaped program is expressible: the
// design names nano as one of four capability targets.
//
// What is real: the layout, the whole default help bar, the editing and file
// commands behind it, search and replace, cut and paste, justify, undo/redo,
// go-to-line, and the command-line options that matter.
//
// What is not: ^T Execute (running a shell command is out of scope for an
// example), and --softwrap, which needs wrapping in TextArea itself.
//
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "editor.hpp"
#include "modcurses/app.hpp"
#include "modcurses/args.hpp"
#include "modcurses/text.hpp"
#include "modcurses/utf8.hpp"
#include "modcurses/widgets.hpp"
#include "ui.hpp"

using namespace modcurses;

namespace {

constexpr const char* kVersion = "1.0";

// What the prompt line is currently asking for. nano drives its prompts as a
// small state machine like this; the answer's meaning depends on the state.
enum class Prompt {
    None,
    WriteOut,        // "File Name to Write: "
    ReadFile,        // "File to insert: "
    Search,          // "Search: "
    SearchBackward,  // "Search Backwards: "
    ReplaceFind,     // "Search (to replace): "
    ReplaceWith,     // "Replace with: "
    ReplaceConfirm,  // "Replace this instance?"
    GotoLine,        // "Enter line number, column number: "
    SaveOnExit,      // "Save modified buffer?"
};

const char* kHelpText =
    "                          mamon help text\n"
    "\n"
    "  mamon is a small editor in the style of GNU nano, written as a test\n"
    "  case for the modcurses TUI library. The commands below follow nano's.\n"
    "\n"
    "  The top line shows the program version, the name of the file being\n"
    "  edited, and whether it has been modified. Below that is the main\n"
    "  editor window. The status line is the third line from the bottom and\n"
    "  shows important messages. The bottom two lines list the most commonly\n"
    "  used shortcuts.\n"
    "\n"
    "  Shortcuts are written below as follows: control-key sequences are\n"
    "  notated with a '^' and are entered with the Ctrl key. Meta-key\n"
    "  sequences are notated with 'M-' and are entered with the Alt key.\n"
    "\n"
    "  ^G   (F1)     Display this help text\n"
    "  ^X   (F2)     Close the buffer and exit from mamon\n"
    "  ^O   (F3)     Write the current buffer to disk\n"
    "  ^R   (F5)     Insert another file into the current buffer\n"
    "  ^W   (F6)     Search forward for a string\n"
    "  ^Q            Search backward for a string\n"
    "  ^\\            Replace a string\n"
    "  ^K   (F9)     Cut the current line into the cutbuffer\n"
    "  ^U   (F10)    Paste the cutbuffer into the current line\n"
    "  ^J   (F4)     Justify the current paragraph\n"
    "  ^C            Report the cursor position\n"
    "  ^/            Go to a specified line (also M-G)\n"
    "  M-U           Undo the last operation\n"
    "  M-E           Redo the last undone operation\n"
    "\n"
    "  ^A            Go to the beginning of the current line\n"
    "  ^E            Go to the end of the current line\n"
    "  ^Y            Go up one page\n"
    "  ^V            Go down one page\n"
    "  ^P / ^N       Go up / down one line\n"
    "  ^B / ^F       Go back / forward one character\n"
    "  M-\\           Go to the first line of the file\n"
    "  M-/           Go to the last line of the file\n"
    "  ^D            Delete the character under the cursor\n"
    "  ^H            Delete the character before the cursor\n"
    "\n"
    "  Consecutive ^K presses accumulate into the cutbuffer, so several\n"
    "  lines can be cut and pasted back together.\n"
    "\n"
    "  Press ^X or Escape to leave this help text.\n";

std::string basename_of(const std::filesystem::path& p) {
    return p.empty() ? std::string{} : p.filename().string();
}

}  // namespace

// ------------------------------------------------------------------ mamon

namespace {

class Mamon : public VBox {
public:
    Mamon(App& app, TextBuffer& buffer) : app_(&app), buffer_(&buffer), editor_(buffer) {}

    void build();
    void report(const std::string& message) { say(message); }

    bool show_line_numbers = false;
    bool view_mode = false;  // -v: look, do not touch
    int tab_width = 4;

protected:
    bool on_key(const KeyEvent& ev) override;

private:
    // ---- chrome ----
    void refresh_title();
    void say(const std::string& message);          // a status message
    void begin_prompt(Prompt kind, const std::string& label, const std::string& seed = "");
    void end_prompt();
    void answer_prompt(const std::u32string& text);
    [[nodiscard]] bool prompting() const { return prompt_ != Prompt::None; }

    // ---- commands ----
    bool do_key_command(const KeyEvent& ev);
    void write_out(const std::filesystem::path& path);
    void read_file(const std::filesystem::path& path);
    void report_location();
    void run_search(bool backwards, const std::u32string& needle);
    void toggle_help();
    void try_exit();

    App* app_;
    TextBuffer* buffer_;
    mamon::Editor editor_;

    mamon::TitleBar* title_ = nullptr;
    mamon::MessageBar* message_ = nullptr;
    mamon::HelpBar* help_bar_ = nullptr;
    TextArea* view_ = nullptr;
    Stack* pages_ = nullptr;
    HBox* prompt_row_ = nullptr;
    Label* prompt_label_ = nullptr;
    TextInput* prompt_input_ = nullptr;
    TextArea* help_view_ = nullptr;
    TextBuffer help_buffer_;

    ScopedConnection changed_conn_;
    ScopedConnection submit_conn_;

    Prompt prompt_ = Prompt::None;
    std::u32string search_needle_;
    std::u32string replace_with_;
    int replaced_count_ = 0;
    bool helping_ = false;
};

void Mamon::build() {
    title_ = &emplace_child<mamon::TitleBar>();
    title_->set_version("mamon " + std::string{kVersion});

    pages_ = &emplace_child<Stack>();

    // ---- page 0: the editor ---------------------------------------------
    auto& body = pages_->emplace_child<HBox>();
    // The gutter is added first so it lands to the left of the text.
    mamon::LineNumbers* gutter =
        show_line_numbers ? &body.emplace_child<mamon::LineNumbers>() : nullptr;
    view_ = &body.emplace_child<TextArea>(*buffer_);
    if (gutter != nullptr) gutter->set_view(*view_);
    view_->tab_width = tab_width;
    view_->read_only = view_mode;
    view_->capture_tab = !view_mode;

    // ---- page 1: the help text ------------------------------------------
    auto& help_page = pages_->emplace_child<HBox>();
    help_buffer_.set_text(utf8_decode(kHelpText));
    help_buffer_.clear_dirty();
    help_view_ = &help_page.emplace_child<TextArea>(help_buffer_);
    help_view_->read_only = true;
    help_view_->capture_tab = false;

    // ---- the status line, which doubles as the prompt --------------------
    message_ = &emplace_child<mamon::MessageBar>();
    prompt_row_ = &emplace_child<HBox>();
    prompt_row_->height_hint = SizeReq::fixed(1);
    prompt_row_->visible = false;
    prompt_label_ = &prompt_row_->emplace_child<Label>();
    prompt_label_->style = Style{}.with(Trait::Reverse);
    prompt_input_ = &prompt_row_->emplace_child<TextInput>();

    help_bar_ = &emplace_child<mamon::HelpBar>();

    submit_conn_ = prompt_input_->submitted.connect(
        [this](const std::u32string& text) { answer_prompt(text); });
    changed_conn_ = buffer_->changed.connect([this] { refresh_title(); });

    // Undo and cut-run bookkeeping has to happen BEFORE the TextArea mutates
    // the buffer, and the hook is consulted before the widget's own on_key -
    // which is exactly what it is for. Returning false lets the view go on to
    // do the actual editing.
    view_->on_key_hook = [this](const KeyEvent& ev) {
        if (ev.mods.ctrl || ev.mods.alt) return false;  // a command, not text
        editor_.interrupt_cut_run();  // nano: anything but ^K ends a cut run
        message_->clear();
        if (!view_mode) {
            if (ev.key == Key::Char || ev.key == Key::Enter)
                editor_.begin_edit(mamon::EditKind::Insert);
            else if (ev.key == Key::Backspace || ev.key == Key::Delete)
                editor_.begin_edit(mamon::EditKind::Erase);
        }
        return false;
    };

    view_->take_focus();
    refresh_title();
    if (view_mode) say("[ View mode: the buffer is read-only ]");
}

void Mamon::refresh_title() {
    title_->set_filename(basename_of(buffer_->path()));
    title_->set_modified(buffer_->dirty());
}

void Mamon::say(const std::string& message) {
    message_->show_message(message);
    invalidate();
}

// ---------------------------------------------------------------- prompts

void Mamon::begin_prompt(Prompt kind, const std::string& label, const std::string& seed) {
    prompt_ = kind;
    message_->clear();
    message_->visible = false;
    prompt_row_->visible = true;
    prompt_label_->set_text(label);
    prompt_label_->width_hint = SizeReq::fixed(static_cast<int>(utf8_decode(label).size()));

    // A yes/no question is NOT an editable field. Giving the TextInput focus
    // for one would let it swallow the answer as typing, and the question
    // would never be answered.
    const bool editable = kind != Prompt::SaveOnExit && kind != Prompt::ReplaceConfirm;
    prompt_input_->visible = editable;
    if (editable) {
        prompt_input_->set_text(seed);
        prompt_input_->take_focus();
        help_bar_->set_prompt_entries();
    } else {
        // No focus at all, so the keys reach this widget directly.
        app_->loop().focus().set(nullptr);
        help_bar_->set_yesno_entries();
    }
    invalidate_layout();
}

void Mamon::end_prompt() {
    prompt_ = Prompt::None;
    prompt_row_->visible = false;
    prompt_input_->visible = true;
    message_->visible = true;
    prompt_input_->set_text("");
    help_bar_->set_default_entries();
    // Focus goes back to the text, or typing would stop working.
    view_->take_focus();
    invalidate_layout();
}

void Mamon::answer_prompt(const std::u32string& text) {
    const Prompt kind = prompt_;
    const std::string answer = utf8_encode(text);
    end_prompt();

    switch (kind) {
        case Prompt::WriteOut:
            if (answer.empty()) {
                say("[ Cancelled ]");
                return;
            }
            write_out(std::filesystem::path{answer});
            return;
        case Prompt::ReadFile:
            if (answer.empty()) {
                say("[ Cancelled ]");
                return;
            }
            read_file(std::filesystem::path{answer});
            return;
        case Prompt::Search:
        case Prompt::SearchBackward: {
            if (!text.empty()) search_needle_ = text;
            run_search(kind == Prompt::SearchBackward, search_needle_);
            return;
        }
        case Prompt::ReplaceFind:
            if (text.empty()) {
                say("[ Cancelled ]");
                return;
            }
            search_needle_ = text;
            begin_prompt(Prompt::ReplaceWith, "Replace with: ");
            return;
        case Prompt::ReplaceWith:
            replace_with_ = text;
            replaced_count_ = editor_.replace_all(search_needle_, replace_with_, false);
            say("[ Replaced " + std::to_string(replaced_count_) +
                (replaced_count_ == 1 ? " occurrence ]" : " occurrences ]"));
            return;
        case Prompt::GotoLine: {
            int line = 0;
            const auto* end = answer.data() + answer.size();
            const auto res = std::from_chars(answer.data(), end, line);
            if (res.ec != std::errc{} || res.ptr != end) {
                say("[ Invalid line number ]");
                return;
            }
            editor_.goto_line(line - 1);  // nano counts from 1
            return;
        }
        case Prompt::SaveOnExit:
        case Prompt::ReplaceConfirm:
        case Prompt::None:
            return;
    }
}

// -------------------------------------------------------------- file work

void Mamon::write_out(const std::filesystem::path& path) {
    std::string error;
    if (!buffer_->save(path, &error)) {
        say("[ Error writing " + path.filename().string() + ": " + error + " ]");
        return;
    }
    const int lines = buffer_->line_count();
    say("[ Wrote " + std::to_string(lines) + (lines == 1 ? " line ]" : " lines ]"));
    refresh_title();
}

void Mamon::read_file(const std::filesystem::path& path) {
    TextBuffer incoming;
    std::string error;
    if (!incoming.load(path, &error)) {
        say("[ Error reading " + path.filename().string() + ": " + error + " ]");
        return;
    }
    editor_.begin_edit(mamon::EditKind::Paste);
    buffer_->insert(incoming.text());
    const int lines = incoming.line_count();
    say("[ Read " + std::to_string(lines) + (lines == 1 ? " line ]" : " lines ]"));
}

void Mamon::report_location() {
    const auto cursor = buffer_->cursor();
    const int lines = buffer_->line_count();
    // nano counts characters over the whole buffer to give a percentage.
    long long before = 0, total = 0;
    for (int i = 0; i < lines; ++i) {
        const auto len = static_cast<long long>(buffer_->line(i).size()) + 1;
        if (i < cursor.line) before += len;
        total += len;
    }
    before += cursor.col;
    const int percent = total > 0 ? static_cast<int>((before * 100) / total) : 0;
    say("line " + std::to_string(cursor.line + 1) + "/" + std::to_string(lines) + " (" +
        std::to_string(lines > 0 ? ((cursor.line + 1) * 100) / lines : 0) + "%), col " +
        std::to_string(cursor.col + 1) + ", char " + std::to_string(before) + "/" +
        std::to_string(total) + " (" + std::to_string(percent) + "%)");
}

void Mamon::run_search(bool backwards, const std::u32string& needle) {
    if (needle.empty()) {
        say("[ Cancelled ]");
        return;
    }
    const mamon::SearchOutcome out = editor_.find(needle, backwards, false);
    if (!out.found) {
        say("\"" + utf8_encode(needle) + "\" not found");
    } else if (out.wrapped) {
        say("[ Search Wrapped ]");
    } else {
        message_->clear();
    }
}

void Mamon::toggle_help() {
    helping_ = !helping_;
    pages_->set_active(helping_ ? 1 : 0);
    if (helping_) {
        help_view_->take_focus();  // so the arrows scroll the help
        say("[ Press ^X or Escape to leave the help ]");
    } else {
        view_->take_focus();
        message_->clear();
    }
}

void Mamon::try_exit() {
    if (!buffer_->dirty()) {
        app_->quit(0);
        return;
    }
    begin_prompt(Prompt::SaveOnExit,
                 "Save modified buffer? (Answering \"No\" will DISCARD changes.) ");
}

// ------------------------------------------------------------------- keys

bool Mamon::on_key(const KeyEvent& ev) {
    // ---- a prompt is up: it owns the keyboard, bar cancelling ----
    if (prompting()) {
        if (prompt_ == Prompt::SaveOnExit) {
            if (ev.key == Key::Char && (ev.text == U'y' || ev.text == U'Y')) {
                end_prompt();
                if (buffer_->path().empty()) {
                    begin_prompt(Prompt::WriteOut, "File Name to Write: ");
                } else {
                    write_out(buffer_->path());
                    app_->quit(0);
                }
                return true;
            }
            if (ev.key == Key::Char && (ev.text == U'n' || ev.text == U'N')) {
                app_->quit(0);
                return true;
            }
        }
        if (ev.key == Key::Escape || (ev.key == Key::Char && ev.mods.ctrl && ev.text == U'c')) {
            end_prompt();
            say("[ Cancelled ]");
            return true;
        }
        return false;  // the TextInput has focus and handles the rest
    }

    // ---- the help page ----
    if (helping_) {
        if (ev.key == Key::Escape ||
            (ev.key == Key::Char && ev.mods.ctrl && (ev.text == U'x' || ev.text == U'g'))) {
            toggle_help();
            return true;
        }
        return false;  // let the pager scroll
    }

    return do_key_command(ev);
}

bool Mamon::do_key_command(const KeyEvent& ev) {
    TextBuffer& b = *buffer_;
    const bool writable = !view_mode;

    // Function keys, as nano also offers them.
    switch (ev.key) {
        case Key::F1: toggle_help(); return true;
        case Key::F2: try_exit(); return true;
        case Key::F3: begin_prompt(Prompt::WriteOut, "File Name to Write: ",
                                   buffer_->path().string()); return true;
        case Key::F5: begin_prompt(Prompt::ReadFile, "File to insert: "); return true;
        case Key::F6: begin_prompt(Prompt::Search, "Search: "); return true;
        default: break;
    }

    if (ev.key == Key::Char && ev.mods.alt) {
        switch (ev.text) {
            case U'u':
                if (!writable) return true;
                say(editor_.undo() ? "[ Undid last action ]" : "[ Nothing to undo ]");
                return true;
            case U'e':
                if (!writable) return true;
                say(editor_.redo() ? "[ Redid last undone action ]" : "[ Nothing to redo ]");
                return true;
            case U'g': begin_prompt(Prompt::GotoLine, "Enter line number: "); return true;
            case U'r': begin_prompt(Prompt::ReplaceFind, "Search (to replace): "); return true;
            case U'w': run_search(false, search_needle_); return true;
            case U'\\': b.move_buffer_start(); return true;
            case U'/': b.move_buffer_end(); return true;
            default: return false;
        }
    }

    // Anything that is not a Ctrl chord belongs to the TextArea, which has
    // already had it (it holds focus, and this runs on the way back up).
    if (ev.key != Key::Char || !ev.mods.ctrl) return false;

    // ---- the control commands, in help-bar order ----
    switch (ev.text) {
        case U'g': toggle_help(); return true;
        case U'x': try_exit(); return true;
        case U'o':
            begin_prompt(Prompt::WriteOut, "File Name to Write: ", buffer_->path().string());
            return true;
        case U'r': begin_prompt(Prompt::ReadFile, "File to insert: "); return true;
        case U'w': begin_prompt(Prompt::Search, "Search: "); return true;
        case U'q': begin_prompt(Prompt::SearchBackward, "Search Backwards: "); return true;
        case U'\\': begin_prompt(Prompt::ReplaceFind, "Search (to replace): "); return true;
        case U'k':
            if (!writable) return true;
            editor_.cut_line();
            return true;
        case U'u':
            if (!writable) return true;
            editor_.paste();
            return true;
        case U't': say("[ Execute is not supported in mamon ]"); return true;
        case U'j': {
            if (!writable) return true;
            const int width = std::max(8, view_->size().width - 1);
            const int lines = editor_.justify(width);
            say(lines > 0 ? "[ Justified paragraph ]" : "[ Nothing to justify ]");
            return true;
        }
        case U'c': report_location(); return true;
        case U'_':
        case U'/': begin_prompt(Prompt::GotoLine, "Enter line number: "); return true;

        // Navigation, nano's emacs-flavoured set.
        case U'a': b.move_line_start(); return true;
        case U'e': b.move_line_end(); return true;
        case U'p': b.move_up(); return true;
        case U'n': b.move_down(); return true;
        case U'b': b.move_left(); return true;
        case U'f': b.move_right(); return true;
        case U'y': {
            const int page = std::max(1, view_->visible_lines() - 2);
            b.set_cursor({b.cursor().line - page, b.cursor().col});
            return true;
        }
        case U'v': {
            const int page = std::max(1, view_->visible_lines() - 2);
            b.set_cursor({b.cursor().line + page, b.cursor().col});
            return true;
        }
        case U'd':
            if (!writable) return true;
            editor_.begin_edit(mamon::EditKind::Erase);
            b.erase_forward();
            return true;
        case U'h':
            if (!writable) return true;
            editor_.begin_edit(mamon::EditKind::Erase);
            b.backspace();
            return true;
        case U'l': message_->clear(); return true;  // nano: refresh the screen
        default: return false;
    }
}

}  // namespace

// ---------------------------------------------------------------------- main

int main(int argc, char** argv) {
    try {
        ArgParser args{"mamon", kVersion, "a small editor in the style of GNU nano"};

        auto& file_arg =
            args.positional<std::string>("file", "the file to edit").required(false);
        auto& line_arg = args.option<int>('L', "line", "put the cursor on this line")
                             .default_value(1)
                             .metavar("N");
        auto& tab_arg =
            args.option<int>('T', "tabsize", "the width of a tab in columns").default_value(4);
        auto& numbers_arg = args.flag('l', "linenumbers", "show line numbers left of the text");
        auto& view_arg = args.flag('v', "view", "read-only mode; the buffer cannot be changed");
        auto& mouse_arg = args.flag('m', "mouse", "enable mouse support");

        tab_arg.validate([](const int& v, std::string& why) {
            if (v >= 1 && v <= 16) return true;
            why = "must be 1 to 16, got " + std::to_string(v);
            return false;
        });
        line_arg.validate([](const int& v, std::string& why) {
            if (v >= 1) return true;
            why = "must be 1 or greater, got " + std::to_string(v);
            return false;
        });

        // The buffer outlives the widget tree that views it.
        TextBuffer buffer;

        App app{argc, argv, args, AppInfo{"mamon", kVersion, "a nano-like editor"}};
        if (app.should_exit()) return app.exit_code();

        std::string opening_message;
        if (!file_arg.value_or("").empty()) {
            const std::filesystem::path path{file_arg.value()};
            if (!std::filesystem::exists(path)) {
                // nano opens a not-yet-existing file as a new buffer carrying
                // that name, rather than refusing to start.
                buffer.set_path(path);
                buffer.clear_dirty();
                opening_message = "[ New File ]";
            } else {
                std::string error;
                if (buffer.load(path, &error)) {
                    const int lines = buffer.line_count();
                    opening_message = "[ Read " + std::to_string(lines) +
                                      (lines == 1 ? " line ]" : " lines ]");
                } else {
                    opening_message = "[ Error reading " + path.filename().string() + ": " +
                                      error + " ]";
                }
            }
        }

        auto& editor = app.make_root<Mamon>(app, buffer);
        editor.show_line_numbers = numbers_arg.value();
        editor.view_mode = view_arg.value();
        editor.tab_width = tab_arg.value();
        editor.build();

        // ^C is Location in nano, so App's default Ctrl-C quit has to go.
        // REMOVED, not replaced with a no-op: app shortcuts are consulted
        // before any widget and return immediately, so a do-nothing handler
        // would swallow ^C and neither Location nor Cancel would ever fire.
        app.loop().remove_shortcut(ctrl_ev(U'c'));

        buffer.set_cursor({line_arg.value() - 1, 0});
        if (!opening_message.empty()) editor.report(opening_message);
        (void)mouse_arg;  // the backend enables the mouse unconditionally

        return app.run();
    } catch (const TerminalError& e) {
        std::fprintf(stderr, "mamon: %s\n", e.what());
        return 1;
    }
}
