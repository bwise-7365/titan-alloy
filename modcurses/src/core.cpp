#include "modcurses/core.hpp"

namespace modcurses {

const char* to_string(Key k) {
    switch (k) {
        case Key::None: return "None";
        case Key::Char: return "Char";
        case Key::Enter: return "Enter";
        case Key::Escape: return "Escape";
        case Key::Tab: return "Tab";
        case Key::BackTab: return "BackTab";
        case Key::Backspace: return "Backspace";
        case Key::Delete: return "Delete";
        case Key::Insert: return "Insert";
        case Key::Up: return "Up";
        case Key::Down: return "Down";
        case Key::Left: return "Left";
        case Key::Right: return "Right";
        case Key::Home: return "Home";
        case Key::End: return "End";
        case Key::PageUp: return "PageUp";
        case Key::PageDown: return "PageDown";
        case Key::F1: return "F1";
        case Key::F2: return "F2";
        case Key::F3: return "F3";
        case Key::F4: return "F4";
        case Key::F5: return "F5";
        case Key::F6: return "F6";
        case Key::F7: return "F7";
        case Key::F8: return "F8";
        case Key::F9: return "F9";
        case Key::F10: return "F10";
        case Key::F11: return "F11";
        case Key::F12: return "F12";
    }
    return "?";
}

}  // namespace modcurses
