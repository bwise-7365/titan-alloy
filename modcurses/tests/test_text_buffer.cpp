#include <filesystem>
#include <fstream>
#include <string>

#include "doctest.h"
#include "modcurses/text.hpp"
#include "modcurses/utf8.hpp"

using namespace modcurses;
using Cursor = TextBuffer::Cursor;

namespace {

// A scratch file that removes itself, so a failing assertion cannot leave
// litter behind in the temp directory.
struct TempFile {
    std::filesystem::path path;
    explicit TempFile(const char* name)
        : path(std::filesystem::temp_directory_path() / ("modcurses_test_" + std::string(name))) {
        std::filesystem::remove(path);
    }
    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    void write_bytes(std::string_view bytes) const {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    [[nodiscard]] std::string read_bytes() const {
        std::ifstream in(path, std::ios::binary);
        return std::string{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    }
};

}  // namespace

TEST_CASE("a new buffer is one empty line, not zero lines") {
    TextBuffer b;
    CHECK(b.line_count() == 1);
    CHECK(b.empty());
    CHECK(b.text().empty());
    CHECK(b.cursor() == Cursor{0, 0});
    CHECK_FALSE(b.dirty());
}

TEST_CASE("set_text splits on newlines and text() joins them back") {
    TextBuffer b;
    b.set_text(U"one\ntwo\nthree");
    CHECK(b.line_count() == 3);
    CHECK(b.line(1) == U"two");
    CHECK(b.text() == U"one\ntwo\nthree");

    SUBCASE("a trailing newline makes a final empty line") {
        b.set_text(U"a\n");
        CHECK(b.line_count() == 2);
        CHECK(b.line(1).empty());
    }
    SUBCASE("carriage returns are line-ending artefacts, not content") {
        b.set_text(U"a\r\nb");
        CHECK(b.line_count() == 2);
        CHECK(b.line(0) == U"a");
    }
    SUBCASE("out-of-range line reads are empty, not undefined") {
        CHECK(b.line(-1).empty());
        CHECK(b.line(99).empty());
        CHECK(b.line_length(99) == 0);
    }
}

TEST_CASE("inserting text at the cursor") {
    TextBuffer b;
    b.insert(U'a');
    b.insert(U'b');
    CHECK(b.text() == U"ab");
    CHECK(b.cursor() == Cursor{0, 2});
    CHECK(b.dirty());

    SUBCASE("inserting mid-line") {
        b.set_cursor({0, 1});
        b.insert(U'X');
        CHECK(b.text() == U"aXb");
        CHECK(b.cursor() == Cursor{0, 2});
    }
    SUBCASE("a string containing newlines splits lines as it goes") {
        b.insert(U"1\n2");
        CHECK(b.text() == U"ab1\n2");
        CHECK(b.cursor() == Cursor{1, 1});
    }
    SUBCASE("inserting a newline character is a line split") {
        b.insert(U'\n');
        CHECK(b.line_count() == 2);
    }
}

TEST_CASE("insert_newline splits the line at the cursor") {
    TextBuffer b{U"hello"};
    b.set_cursor({0, 2});
    b.insert_newline();
    CHECK(b.line_count() == 2);
    CHECK(b.line(0) == U"he");
    CHECK(b.line(1) == U"llo");
    CHECK(b.cursor() == Cursor{1, 0});
}

TEST_CASE("backspace") {
    TextBuffer b{U"ab\ncd"};

    SUBCASE("mid-line deletes the character before the cursor") {
        b.set_cursor({0, 2});
        CHECK(b.backspace());
        CHECK(b.line(0) == U"a");
        CHECK(b.cursor() == Cursor{0, 1});
    }
    SUBCASE("at the start of a line it joins with the previous one") {
        b.set_cursor({1, 0});
        CHECK(b.backspace());
        CHECK(b.line_count() == 1);
        CHECK(b.line(0) == U"abcd");
        CHECK(b.cursor() == Cursor{0, 2});  // lands at the seam
    }
    SUBCASE("at the very start of the buffer it does nothing and says so") {
        b.set_cursor({0, 0});
        CHECK_FALSE(b.backspace());
        CHECK(b.text() == U"ab\ncd");
    }
}

TEST_CASE("erase_forward") {
    TextBuffer b{U"ab\ncd"};

    SUBCASE("mid-line deletes the character under the cursor") {
        b.set_cursor({0, 0});
        CHECK(b.erase_forward());
        CHECK(b.line(0) == U"b");
        CHECK(b.cursor() == Cursor{0, 0});  // cursor stays put
    }
    SUBCASE("at end of line it pulls the next line up") {
        b.set_cursor({0, 2});
        CHECK(b.erase_forward());
        CHECK(b.line_count() == 1);
        CHECK(b.line(0) == U"abcd");
    }
    SUBCASE("at the very end of the buffer it does nothing and says so") {
        b.set_cursor({1, 2});
        CHECK_FALSE(b.erase_forward());
    }
}

TEST_CASE("erase_line") {
    TextBuffer b{U"a\nb\nc"};
    b.set_cursor({1, 0});
    b.erase_line();
    CHECK(b.text() == U"a\nc");

    SUBCASE("erasing the only line empties it rather than leaving zero lines") {
        TextBuffer one{U"solo"};
        one.erase_line();
        CHECK(one.line_count() == 1);
        CHECK(one.empty());
    }
}

TEST_CASE("cursor movement across line boundaries") {
    TextBuffer b{U"ab\ncd"};

    SUBCASE("left from column 0 wraps to the end of the previous line") {
        b.set_cursor({1, 0});
        b.move_left();
        CHECK(b.cursor() == Cursor{0, 2});
    }
    SUBCASE("right from end of line wraps to the start of the next") {
        b.set_cursor({0, 2});
        b.move_right();
        CHECK(b.cursor() == Cursor{1, 0});
    }
    SUBCASE("neither wraps off the ends of the buffer") {
        b.set_cursor({0, 0});
        b.move_left();
        CHECK(b.cursor() == Cursor{0, 0});
        b.set_cursor({1, 2});
        b.move_right();
        CHECK(b.cursor() == Cursor{1, 2});
    }
}

TEST_CASE("moving between lines of different lengths clamps the column") {
    TextBuffer b{U"long line\nab"};
    b.set_cursor({0, 9});
    b.move_down();
    CHECK(b.cursor() == Cursor{1, 2});  // clamped to the shorter line
}

TEST_CASE("line and buffer ends") {
    TextBuffer b{U"abc\ndefgh"};
    b.set_cursor({1, 2});
    b.move_line_start();
    CHECK(b.cursor() == Cursor{1, 0});
    b.move_line_end();
    CHECK(b.cursor() == Cursor{1, 5});
    b.move_buffer_start();
    CHECK(b.cursor() == Cursor{0, 0});
    b.move_buffer_end();
    CHECK(b.cursor() == Cursor{1, 5});
}

TEST_CASE("word movement") {
    TextBuffer b{U"foo bar_baz  qux"};
    b.set_cursor({0, 0});

    b.move_word_right();
    CHECK(b.cursor().col == 4);  // start of "bar_baz" (underscore is a word char)
    b.move_word_right();
    CHECK(b.cursor().col == 13);  // start of "qux", spaces skipped
    b.move_word_right();
    CHECK(b.cursor().col == 16);  // end of line

    b.move_word_left();
    CHECK(b.cursor().col == 13);
    b.move_word_left();
    CHECK(b.cursor().col == 4);
    b.move_word_left();
    CHECK(b.cursor().col == 0);
}

TEST_CASE("set_cursor clamps rather than trusting the caller") {
    TextBuffer b{U"ab\ncd"};
    b.set_cursor({99, 99});
    CHECK(b.cursor() == Cursor{1, 2});
    b.set_cursor({-5, -5});
    CHECK(b.cursor() == Cursor{0, 0});
}

TEST_CASE("signals distinguish an edit from a cursor move") {
    TextBuffer b{U"ab"};
    int changed = 0, moved = 0;
    auto c1 = b.changed.connect([&] { ++changed; });
    auto c2 = b.cursor_moved.connect([&] { ++moved; });

    b.move_right();
    CHECK(moved == 1);
    CHECK(changed == 0);

    b.insert(U'x');
    CHECK(changed == 1);

    SUBCASE("a movement that changes nothing signals nothing") {
        b.move_buffer_end();          // get to the end first...
        const int before = moved;
        b.move_buffer_end();          // ...so that these two are genuine no-ops
        b.move_line_end();
        b.move_right();
        CHECK(moved == before);
    }
}

TEST_CASE("the dirty flag tracks edits and can be cleared") {
    TextBuffer b;
    CHECK_FALSE(b.dirty());
    b.insert(U'x');
    CHECK(b.dirty());
    b.clear_dirty();
    CHECK_FALSE(b.dirty());
    b.move_left();
    CHECK_FALSE(b.dirty());  // moving is not editing
}

// ---------------------------------------------------------------- file I/O

TEST_CASE("load and save round-trip an LF file byte for byte") {
    TempFile f{"lf.txt"};
    f.write_bytes("alpha\nbeta\ngamma\n");

    TextBuffer b;
    REQUIRE(b.load(f.path));
    CHECK(b.line_count() == 3);
    CHECK(b.line(2) == U"gamma");
    CHECK(b.line_ending() == TextBuffer::LineEnding::Lf);
    CHECK_FALSE(b.dirty());
    CHECK(b.path() == f.path);

    REQUIRE(b.save());
    CHECK(f.read_bytes() == "alpha\nbeta\ngamma\n");
}

TEST_CASE("a CRLF file stays CRLF across a save") {
    // The whole point of opening in binary: editing a Windows file on any
    // machine must not silently rewrite every line of it.
    TempFile f{"crlf.txt"};
    f.write_bytes("one\r\ntwo\r\n");

    TextBuffer b;
    REQUIRE(b.load(f.path));
    CHECK(b.line_count() == 2);
    CHECK(b.line(0) == U"one");
    CHECK(b.line_ending() == TextBuffer::LineEnding::CrLf);

    b.set_cursor({1, 3});
    b.insert(U'!');
    REQUIRE(b.save());
    CHECK(f.read_bytes() == "one\r\ntwo!\r\n");
}

TEST_CASE("a file with no final newline does not grow one") {
    TempFile f{"nonl.txt"};
    f.write_bytes("no newline here");

    TextBuffer b;
    REQUIRE(b.load(f.path));
    CHECK(b.line_count() == 1);
    CHECK_FALSE(b.final_newline());
    REQUIRE(b.save());
    CHECK(f.read_bytes() == "no newline here");
}

TEST_CASE("UTF-8 content survives the round trip") {
    TempFile f{"utf8.txt"};
    f.write_bytes("héllo wörld\nsecond ligne\n");

    TextBuffer b;
    REQUIRE(b.load(f.path));
    CHECK(b.line(0) == U"héllo wörld");
    CHECK(b.line_length(0) == 11);  // codepoints, not bytes
    REQUIRE(b.save());
    CHECK(f.read_bytes() == "héllo wörld\nsecond ligne\n");
}

TEST_CASE("a UTF-8 BOM is stripped rather than treated as content") {
    TempFile f{"bom.txt"};
    f.write_bytes("\xEF\xBB\xBF" "text");

    TextBuffer b;
    REQUIRE(b.load(f.path));
    CHECK(b.line(0) == U"text");
}

TEST_CASE("a malformed byte is replaced, not fatal") {
    TempFile f{"bad.txt"};
    f.write_bytes("ok\x80" "then\n");

    TextBuffer b;
    REQUIRE(b.load(f.path));
    CHECK(b.line(0) == std::u32string{U"ok"} + kReplacementChar + U"then");
}

TEST_CASE("an empty file loads as one empty line") {
    TempFile f{"empty.txt"};
    f.write_bytes("");
    TextBuffer b;
    REQUIRE(b.load(f.path));
    CHECK(b.line_count() == 1);
    CHECK(b.empty());
}

TEST_CASE("failures report rather than throw") {
    TextBuffer b;
    std::string error;
    CHECK_FALSE(b.load(std::filesystem::temp_directory_path() / "modcurses_does_not_exist_xyz",
                       &error));
    CHECK_FALSE(error.empty());

    error.clear();
    CHECK_FALSE(b.save(&error));  // no path set
    CHECK(error == "no filename");
}
