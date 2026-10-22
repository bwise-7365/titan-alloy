#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "doctest.h"
#include "modcurses/args.hpp"

using namespace modcurses;

namespace {

// The design's own example parser, used both for the grammar table and for
// the golden help output.
struct Notepad {
    ArgParser args{"notepad", "0.1.0", "A tiny terminal editor"};
    Arg<std::string>& file = args.positional<std::string>("file", "file to open").required(false);
    Arg<bool>& readonly = args.flag('r', "readonly", "open read-only");
    Arg<int>& tabwidth = args.option<int>('t', "tabwidth", "tab width").default_value(4);

    ParseResult run(std::vector<std::string> argv) { return args.parse(argv); }
};

}  // namespace

// ------------------------------------------------------------- the grammar

TEST_CASE("the long-option forms") {
    Notepad n;
    SUBCASE("--long=value") {
        CHECK(n.run({"--tabwidth=8"}).ok);
        CHECK(n.tabwidth.value() == 8);
    }
    SUBCASE("--long value") {
        CHECK(n.run({"--tabwidth", "2"}).ok);
        CHECK(n.tabwidth.value() == 2);
    }
    SUBCASE("--long on a flag") {
        CHECK(n.run({"--readonly"}).ok);
        CHECK(n.readonly.value());
    }
}

TEST_CASE("the short-option forms") {
    Notepad n;
    SUBCASE("-s value") {
        CHECK(n.run({"-t", "3"}).ok);
        CHECK(n.tabwidth.value() == 3);
    }
    SUBCASE("-svalue") {
        CHECK(n.run({"-t3"}).ok);
        CHECK(n.tabwidth.value() == 3);
    }
    SUBCASE("-s on a flag") {
        CHECK(n.run({"-r"}).ok);
        CHECK(n.readonly.value());
    }
}

TEST_CASE("bundled boolean shorts") {
    ArgParser p{"tool"};
    auto& a = p.flag('a', "alpha", "a");
    auto& b = p.flag('b', "beta", "b");
    auto& c = p.flag('c', "gamma", "c");

    REQUIRE(p.parse({"-abc"}).ok);
    CHECK(a.value());
    CHECK(b.value());
    CHECK(c.value());

    SUBCASE("a value-taking option ends the bundle and swallows the rest") {
        ArgParser q{"tool"};
        auto& v = q.flag('v', "verbose", "v");
        auto& n = q.option<int>('n', "count", "n");
        REQUIRE(q.parse({"-vn7"}).ok);
        CHECK(v.value());
        CHECK(n.value() == 7);
    }
    SUBCASE("...or takes the next token") {
        ArgParser q{"tool"};
        auto& v = q.flag('v', "verbose", "v");
        auto& n = q.option<int>('n', "count", "n");
        REQUIRE(q.parse({"-vn", "9"}).ok);
        CHECK(v.value());
        CHECK(n.value() == 9);
    }
}

TEST_CASE("positionals are filled in declaration order") {
    ArgParser p{"tool"};
    auto& first = p.positional<std::string>("src", "source");
    auto& second = p.positional<std::string>("dst", "destination");
    REQUIRE(p.parse({"a.txt", "b.txt"}).ok);
    CHECK(first.value() == "a.txt");
    CHECK(second.value() == "b.txt");
}

TEST_CASE("positionals and options may be interleaved") {
    Notepad n;
    REQUIRE(n.run({"-r", "notes.txt", "--tabwidth", "2"}).ok);
    CHECK(n.readonly.value());
    CHECK(n.file.value() == "notes.txt");
    CHECK(n.tabwidth.value() == 2);
}

TEST_CASE("-- ends option parsing") {
    Notepad n;
    REQUIRE(n.run({"--", "--tabwidth"}).ok);
    CHECK(n.file.value() == "--tabwidth");  // taken as the filename
    CHECK(n.tabwidth.value() == 4);         // still the default

    SUBCASE("a lone dash is a positional, not an option") {
        Notepad m;
        REQUIRE(m.run({"-"}).ok);
        CHECK(m.file.value() == "-");
    }
}

// ------------------------------------------------------------------ values

TEST_CASE("defaults apply when the argument is absent, and present() says so") {
    Notepad n;
    REQUIRE(n.run({}).ok);
    CHECK(n.tabwidth.value() == 4);
    CHECK_FALSE(n.tabwidth.present());
    CHECK_FALSE(n.readonly.value());
    CHECK(n.file.value_or("(none)") == "(none)");

    REQUIRE(n.run({"-t", "4"}).ok);
    CHECK(n.tabwidth.value() == 4);
    CHECK(n.tabwidth.present());  // same value, but it WAS given
}

TEST_CASE("re-parsing starts from a clean slate") {
    Notepad n;
    REQUIRE(n.run({"-r"}).ok);
    CHECK(n.readonly.value());
    REQUIRE(n.run({}).ok);
    CHECK_FALSE(n.readonly.present());
}

TEST_CASE("typed conversion") {
    ArgParser p{"tool"};
    auto& i = p.option<int>('i', "int", "an int");
    auto& d = p.option<double>('d', "dbl", "a double");
    auto& s = p.option<std::string>('s', "str", "a string");

    REQUIRE(p.parse({"-i", "-42", "-d", "1.5", "-s", "hello world"}).ok);
    CHECK(i.value() == -42);
    CHECK(d.value() == doctest::Approx(1.5));
    CHECK(s.value() == "hello world");
}

TEST_CASE("parsing is locale-independent") {
    // Comma decimal separators must not be accepted just because some locale
    // would. App parses before the terminal ever calls setlocale, and
    // from_chars does not consult the locale at all.
    ArgParser p{"tool"};
    p.option<double>('d', "dbl", "a double");
    CHECK_FALSE(p.parse({"-d", "1,5"}).ok);
}

// ------------------------------------------------------------ error reports

TEST_CASE("a conversion failure names the option, the type and the input") {
    Notepad n;
    const ParseResult r = n.run({"--tabwidth", "x"});
    CHECK_FALSE(r.ok);
    CHECK(r.exit_code == 2);
    CHECK(r.message == "error: --tabwidth: expected an integer, got 'x'");
    CHECK(r.done());
}

TEST_CASE("the other usage errors") {
    Notepad n;
    SUBCASE("unknown long option") {
        CHECK(n.run({"--nope"}).message == "error: unknown option '--nope'");
    }
    SUBCASE("unknown short option") {
        CHECK(n.run({"-z"}).message == "error: unknown option '-z'");
    }
    SUBCASE("a missing value") {
        CHECK(n.run({"--tabwidth"}).message == "error: --tabwidth: expected a value");
        // Named canonically even when spelled short, so the message teaches
        // the reader the option's real name.
        CHECK(n.run({"-t"}).message == "error: --tabwidth: expected a value");
    }
    SUBCASE("a value given to a flag") {
        CHECK(n.run({"--readonly=yes"}).message == "error: --readonly does not take a value");
    }
    SUBCASE("too many positionals") {
        CHECK(n.run({"a", "b"}).message == "error: unexpected argument 'b'");
    }
    SUBCASE("a partial conversion is still a failure") {
        CHECK(n.run({"-t", "4x"}).message == "error: --tabwidth: expected an integer, got '4x'");
    }
    SUBCASE("a double that is not a number") {
        ArgParser p{"tool"};
        p.option<double>('d', "dbl", "d");
        CHECK(p.parse({"-d", "nope"}).message == "error: --dbl: expected a number, got 'nope'");
    }
}

TEST_CASE("a required argument that is missing is reported by name") {
    ArgParser p{"tool"};
    p.positional<std::string>("src", "source");  // positionals are required by default
    const ParseResult r = p.parse({});
    CHECK_FALSE(r.ok);
    CHECK(r.message == "error: missing required argument 'src'");

    SUBCASE("a required option too") {
        ArgParser q{"tool"};
        q.option<int>('n', "count", "n").required();
        CHECK(q.parse({}).message == "error: missing required option '--count'");
    }
    SUBCASE("giving a default makes a positional optional") {
        ArgParser q{"tool"};
        q.positional<std::string>("src", "source").default_value("-");
        CHECK(q.parse({}).ok);
    }
}

TEST_CASE("the validation hook can reject an otherwise-valid value") {
    ArgParser p{"tool"};
    p.option<int>('n', "count", "how many").validate([](const int& v, std::string& why) {
        if (v >= 1 && v <= 10) return true;
        why = "must be between 1 and 10, got " + std::to_string(v);
        return false;
    });
    CHECK(p.parse({"-n", "5"}).ok);
    CHECK(p.parse({"-n", "50"}).message == "error: --count: must be between 1 and 10, got 50");
}

// ------------------------------------------------------------ help/version

TEST_CASE("--help and --version request an exit with code 0, not an error") {
    Notepad n;
    SUBCASE("--help") {
        const ParseResult r = n.run({"--help"});
        CHECK(r.ok);
        CHECK(r.exit_requested);
        CHECK(r.exit_code == 0);
        CHECK(r.done());
        CHECK(r.message == n.args.help_text());
    }
    SUBCASE("-h") { CHECK(n.run({"-h"}).exit_requested); }
    SUBCASE("--version") {
        const ParseResult r = n.run({"--version"});
        CHECK(r.exit_requested);
        CHECK(r.message == "notepad 0.1.0");
    }
    SUBCASE("--help wins over a missing required argument") {
        ArgParser p{"tool", "1.0"};
        p.positional<std::string>("src", "source");
        const ParseResult r = p.parse({"--help"});
        CHECK(r.ok);
        CHECK(r.exit_requested);
    }
}

TEST_CASE("no --version is offered when no version was given") {
    ArgParser p{"tool"};
    CHECK_FALSE(p.parse({"--version"}).ok);
    CHECK(p.version_text() == "tool");
}

TEST_CASE("the usage line reflects which positionals are required") {
    ArgParser p{"tool", "1.0"};
    p.positional<std::string>("src", "s");
    p.positional<std::string>("dst", "d").required(false);
    CHECK(p.usage_line() == "Usage: tool [options] <src> [<dst>]");
}

TEST_CASE("help output matches the golden file") {
    // A golden file rather than an inline string: the help screen is the part
    // of this library a user reads first, and a diff against it should be
    // deliberate.
    Notepad n;
    const std::string golden_path = std::string{MODCURSES_TEST_DATA_DIR} + "/notepad_help.txt";
    std::ifstream in(golden_path, std::ios::binary);
    REQUIRE_MESSAGE(in.good(), "cannot open golden file: " << golden_path);

    std::ostringstream buf;
    buf << in.rdbuf();
    std::string expected = buf.str();
    // Tolerate a checkout with CRLF endings.
    std::string normalised;
    for (char c : expected)
        if (c != '\r') normalised.push_back(c);

    CHECK(n.args.help_text() == normalised);
}

// ------------------------------------------------- App integration (M4)

#include <memory>

#include "modcurses/app.hpp"
#include "modcurses/mock_terminal.hpp"
#include "modcurses/widgets.hpp"

namespace {

// argv as a real char**, because that is what main() actually hands over.
struct Argv {
    std::vector<std::string> storage;
    std::vector<char*> pointers;
    explicit Argv(std::vector<std::string> args) : storage(std::move(args)) {
        for (auto& s : storage) pointers.push_back(s.data());
    }
    [[nodiscard]] int argc() const { return static_cast<int>(pointers.size()); }
    [[nodiscard]] char** argv() { return pointers.data(); }
};

}  // namespace

TEST_CASE("App exits early on --help without ever constructing a terminal") {
    // The whole point of the ordering. This test runs headless with no TTY at
    // all: if App touched curses here it would throw, so reaching the checks
    // below IS the proof that it did not.
    Notepad n;
    Argv args{{"notepad", "--help"}};
    App app{args.argc(), args.argv(), n.args, AppInfo{"notepad", "0.1.0"}};

    CHECK(app.should_exit());
    CHECK(app.exit_code() == 0);
    CHECK(app.run() == 0);          // returns immediately, no loop
    CHECK_FALSE(app.pump_once());
    CHECK(app.exit_message() == n.args.help_text());
    CHECK(app.parse_result().exit_requested);
}

TEST_CASE("App exits with code 2 on a usage error") {
    Notepad n;
    Argv args{{"notepad", "--tabwidth", "zzz"}};
    App app{args.argc(), args.argv(), n.args, AppInfo{"notepad", "0.1.0"}};

    CHECK(app.should_exit());
    CHECK(app.exit_code() == 2);
    CHECK(app.run() == 2);
    CHECK(app.exit_message() == "error: --tabwidth: expected an integer, got 'zzz'");
    CHECK_FALSE(app.parse_result().ok);
}

TEST_CASE("using a terminal-less App reports why instead of crashing") {
    Notepad n;
    Argv args{{"notepad", "--help"}};
    App app{args.argc(), args.argv(), n.args};

    // A caller who forgot to check should_exit() gets a diagnostic, not a
    // null dereference.
    CHECK_THROWS_AS(static_cast<void>(app.make_root<VBox>()), TerminalError);
    CHECK_THROWS_AS(static_cast<void>(app.loop()), TerminalError);
    CHECK_THROWS_AS(static_cast<void>(app.terminal()), TerminalError);
    CHECK_THROWS_AS(static_cast<void>(app.buffer()), TerminalError);
    app.quit(1);  // must be a harmless no-op
    CHECK(app.exit_code() == 0);
}

TEST_CASE("parsing and a mock terminal compose for a headless end-to-end test") {
    // The argc/argv overload builds a real curses terminal on success, which
    // no test can do. Parsing separately is the supported way to drive a
    // parsed application headlessly - and it is the same two steps App
    // performs internally, in the same order.
    Notepad n;
    REQUIRE(n.run({"notes.txt", "-r", "-t", "2"}).ok);

    App app{std::make_unique<MockTerminal>(Size{24, 3})};
    auto& root = app.make_root<VBox>();
    root.emplace_child<Label>(n.file.value() + (n.readonly.value() ? " [RO]" : ""));
    app.pump_once();

    auto& term = static_cast<MockTerminal&>(app.terminal());
    CHECK(term.row_text(0) == "notes.txt [RO]          ");
    CHECK(n.tabwidth.value() == 2);
}
