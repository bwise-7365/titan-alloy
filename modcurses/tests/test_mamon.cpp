//
// mamon editor-logic tests. Everything here runs with no terminal, because
// the editing operations were deliberately kept out of the UI.
//
#include <string>

#include "doctest.h"
#include "editor.hpp"
#include "modcurses/text.hpp"
#include "modcurses/utf8.hpp"

using namespace mamon;
using modcurses::TextBuffer;
using Cursor = TextBuffer::Cursor;

// ------------------------------------------------------------ search helpers

TEST_CASE("find_in honours case sensitivity") {
    CHECK(find_in(U"Hello World", U"World", 0, true) == 6);
    CHECK(find_in(U"Hello World", U"world", 0, true) == std::u32string_view::npos);
    CHECK(find_in(U"Hello World", U"world", 0, false) == 6);
    CHECK(find_in(U"aaa", U"aa", 1, false) == 1);
    CHECK(find_in(U"abc", U"", 0, false) == std::u32string_view::npos);
    CHECK(find_in(U"ab", U"abc", 0, false) == std::u32string_view::npos);
}

// ------------------------------------------------------------------- search

TEST_CASE("search moves the cursor to the next match and wraps") {
    TextBuffer b{U"alpha\nbeta\ngamma\nalpha again"};
    Editor e{b};
    b.set_cursor({0, 0});

    SearchOutcome out = e.find(U"alpha", false, false);
    CHECK(out.found);
    CHECK_FALSE(out.wrapped);
    CHECK(b.cursor() == Cursor{3, 0});  // the second "alpha", not the one we were on

    SUBCASE("searching again comes back round to the first") {
        out = e.find(U"alpha", false, false);
        CHECK(out.found);
        CHECK(out.wrapped);
        CHECK(b.cursor() == Cursor{0, 0});
    }
}

TEST_CASE("search is case-insensitive by default, as nano's is") {
    TextBuffer b{U"The Quick Brown Fox"};
    Editor e{b};
    b.set_cursor({0, 0});
    CHECK(e.find(U"quick", false, false).found);
    CHECK(b.cursor().col == 4);

    b.set_cursor({0, 0});
    CHECK_FALSE(e.find(U"quick", false, true).found);
}

TEST_CASE("a search that matches nothing reports so and leaves the cursor alone") {
    TextBuffer b{U"one\ntwo"};
    Editor e{b};
    b.set_cursor({1, 1});
    const SearchOutcome out = e.find(U"absent", false, false);
    CHECK_FALSE(out.found);
    CHECK(b.cursor() == Cursor{1, 1});
}

TEST_CASE("backward search walks the other way") {
    TextBuffer b{U"match\nfiller\nmatch\nfiller"};
    Editor e{b};
    b.set_cursor({3, 0});
    const SearchOutcome out = e.find(U"match", true, false);
    CHECK(out.found);
    CHECK(b.cursor() == Cursor{2, 0});
}

// ------------------------------------------------------------------ replace

TEST_CASE("replace_all changes every occurrence and counts them") {
    TextBuffer b{U"cat dog cat\ncat"};
    Editor e{b};
    CHECK(e.replace_all(U"cat", U"bird", false) == 3);
    CHECK(b.text() == U"bird dog bird\nbird");
}

TEST_CASE("replace_all copes with a replacement that contains the needle") {
    // The naive loop would never terminate here.
    TextBuffer b{U"a a a"};
    Editor e{b};
    CHECK(e.replace_all(U"a", U"aa", false) == 3);
    CHECK(b.text() == U"aa aa aa");
}

TEST_CASE("replace_all with a shorter replacement") {
    TextBuffer b{U"xxxx"};
    Editor e{b};
    CHECK(e.replace_all(U"xx", U"y", false) == 2);
    CHECK(b.text() == U"yy");
}

TEST_CASE("replace_at_cursor only fires when the text really is there") {
    TextBuffer b{U"hello world"};
    Editor e{b};
    b.set_cursor({0, 6});
    CHECK(e.replace_at_cursor(U"world", U"there", false));
    CHECK(b.text() == U"hello there");

    b.set_cursor({0, 0});
    CHECK_FALSE(e.replace_at_cursor(U"world", U"nope", false));
}

// ---------------------------------------------------------------- cut/paste

TEST_CASE("cut takes the current line into the cutbuffer") {
    TextBuffer b{U"one\ntwo\nthree"};
    Editor e{b};
    b.set_cursor({1, 0});
    e.cut_line();
    CHECK(b.text() == U"one\nthree");
    CHECK(e.cut_buffer().size() == 1);
    CHECK(e.cut_buffer()[0] == U"two");
}

TEST_CASE("consecutive cuts ACCUMULATE, which is what makes cut-and-paste work") {
    // nano's rule, and the reason a run of ^K then one ^U moves a block.
    TextBuffer b{U"one\ntwo\nthree\nfour"};
    Editor e{b};
    b.set_cursor({1, 0});
    e.cut_line();
    e.cut_line();
    CHECK(e.cut_buffer().size() == 2);
    CHECK(b.text() == U"one\nfour");

    e.paste();
    CHECK(b.text() == U"one\ntwo\nthree\nfour");
}

TEST_CASE("anything between two cuts starts the cutbuffer over") {
    TextBuffer b{U"one\ntwo\nthree"};
    Editor e{b};
    b.set_cursor({0, 0});
    e.cut_line();
    e.interrupt_cut_run();  // what a cursor move or a keystroke does
    e.cut_line();
    CHECK(e.cut_buffer().size() == 1);
    CHECK(e.cut_buffer()[0] == U"two");
}

TEST_CASE("pasting an empty cutbuffer does nothing") {
    TextBuffer b{U"unchanged"};
    Editor e{b};
    e.paste();
    CHECK(b.text() == U"unchanged");
}

// -------------------------------------------------------------------- undo

TEST_CASE("undo and redo walk the edit history") {
    TextBuffer b{U"start"};
    Editor e{b};
    CHECK_FALSE(e.can_undo());

    b.set_cursor({0, 5});
    e.begin_edit(EditKind::Insert);
    b.insert(U'!');
    CHECK(b.text() == U"start!");
    CHECK(e.can_undo());

    CHECK(e.undo());
    CHECK(b.text() == U"start");
    CHECK(e.can_redo());

    CHECK(e.redo());
    CHECK(b.text() == U"start!");
}

TEST_CASE("a run of typing undoes as one word, not one letter at a time") {
    // The coalescing rule. Without it, undo after typing "hello" would take
    // five presses, which is not what anyone means by undo.
    TextBuffer b;
    Editor e{b};
    for (char32_t c : std::u32string{U"hello"}) {
        e.begin_edit(EditKind::Insert);
        b.insert(c);
    }
    CHECK(b.text() == U"hello");
    CHECK(e.undo());
    CHECK(b.text().empty());
}

TEST_CASE("a different kind of edit starts a new undo step") {
    TextBuffer b;
    Editor e{b};
    e.begin_edit(EditKind::Insert);
    b.insert(U"abc");
    e.begin_edit(EditKind::Cut);
    b.erase_line();

    CHECK(e.undo());
    CHECK(b.text() == U"abc");  // the cut came back, the typing did not
    CHECK(e.undo());
    CHECK(b.text().empty());
}

TEST_CASE("a fresh edit discards the redo stack") {
    TextBuffer b{U"one"};
    Editor e{b};
    e.begin_edit(EditKind::Insert);
    b.insert(U'x');
    CHECK(e.undo());
    CHECK(e.can_redo());

    e.begin_edit(EditKind::Other);
    b.insert(U'y');
    CHECK_FALSE(e.can_redo());
}

TEST_CASE("undo restores the cursor as well as the text") {
    TextBuffer b{U"line one\nline two"};
    Editor e{b};
    b.set_cursor({1, 4});
    e.begin_edit(EditKind::Erase);
    b.erase_line();
    CHECK(b.line_count() == 1);

    CHECK(e.undo());
    CHECK(b.line_count() == 2);
    CHECK(b.cursor() == Cursor{1, 4});
}

TEST_CASE("the history is bounded") {
    TextBuffer b;
    Editor e{b};
    e.history_limit = 5;
    for (int i = 0; i < 50; ++i) {
        e.begin_edit(EditKind::Other);
        b.insert(U'x');
    }
    int undos = 0;
    while (e.undo()) ++undos;
    CHECK(undos <= 5);
}

// ----------------------------------------------------------------- justify

TEST_CASE("justify re-wraps the paragraph around the cursor") {
    TextBuffer b{U"the quick brown fox jumps over the lazy dog"};
    Editor e{b};
    b.set_cursor({0, 0});
    const int lines = e.justify(20);
    CHECK(lines > 1);
    for (int i = 0; i < b.line_count(); ++i) CHECK(b.line(i).size() <= 20);
    // No words lost or invented.
    std::u32string rejoined;
    for (int i = 0; i < b.line_count(); ++i) {
        if (i > 0) rejoined.push_back(U' ');
        rejoined += b.line(i);
    }
    CHECK(rejoined == U"the quick brown fox jumps over the lazy dog");
}

TEST_CASE("justify stops at a blank line, so it touches one paragraph only") {
    TextBuffer b{U"aaa bbb ccc ddd\n\nuntouched paragraph here"};
    Editor e{b};
    b.set_cursor({0, 0});
    e.justify(8);
    CHECK(b.line(b.line_count() - 1) == U"untouched paragraph here");
}

TEST_CASE("justifying a blank line does nothing") {
    TextBuffer b{U"text\n\nmore"};
    Editor e{b};
    b.set_cursor({1, 0});
    CHECK(e.justify(20) == 0);
    CHECK(b.text() == U"text\n\nmore");
}

TEST_CASE("justify is undoable") {
    TextBuffer b{U"one two three four five six seven"};
    Editor e{b};
    b.set_cursor({0, 0});
    e.justify(10);
    CHECK(b.line_count() > 1);
    CHECK(e.undo());
    CHECK(b.text() == U"one two three four five six seven");
}

// --------------------------------------------------------------- goto line

TEST_CASE("goto_line clamps rather than trusting the number") {
    TextBuffer b{U"a\nb\nc"};
    Editor e{b};
    e.goto_line(1);
    CHECK(b.cursor().line == 1);
    e.goto_line(9999);
    CHECK(b.cursor().line == 2);
    e.goto_line(-5);
    CHECK(b.cursor().line == 0);
}

TEST_CASE("justifying the last paragraph does not leave a trailing blank line") {
    // Regression: the rebuild appended a newline after the replacement, so a
    // paragraph that ran to the bottom of the buffer grew an empty line each
    // time it was justified.
    TextBuffer b{U"heading\n\nlast paragraph that is long enough to be re-wrapped here"};
    Editor e{b};
    b.set_cursor({2, 0});
    const int before = b.line_count();
    CHECK(e.justify(200) == 1);  // it all fits on one line at this width
    CHECK(b.line_count() == before);
    CHECK(b.line(b.line_count() - 1) ==
          U"last paragraph that is long enough to be re-wrapped here");

    SUBCASE("and justifying it repeatedly is idempotent") {
        const std::u32string once = b.text();
        e.justify(200);
        e.justify(200);
        CHECK(b.text() == once);
    }
}

TEST_CASE("justify leaves the lines after the paragraph alone") {
    TextBuffer b{U"a b c d e f\n\ntail one\ntail two"};
    Editor e{b};
    b.set_cursor({0, 0});
    e.justify(5);
    CHECK(b.line(b.line_count() - 2) == U"tail one");
    CHECK(b.line(b.line_count() - 1) == U"tail two");
}
