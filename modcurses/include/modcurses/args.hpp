#pragma once
//
// modcurses/args.hpp - declarative, typed, GNU-style argument parsing.
//
// PUBLIC HEADER: no curses. Nothing here so much as knows a terminal exists,
// which is the point: App's construction order guarantees parse() runs BEFORE
// the terminal is initialised, so --help, --version and usage errors print to
// an ordinary stdout and never enter curses mode.
//
#include <charconv>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace modcurses {

// What parse() decided. App interprets this, so applications usually never
// touch it directly.
struct ParseResult {
    bool ok = true;                // the command line was well-formed
    bool exit_requested = false;   // --help or --version was handled
    int exit_code = 0;             // 0 for help/version, 2 for a usage error
    std::string message;           // the help/version text, or the error

    // True when the program should print `message` and stop.
    [[nodiscard]] bool done() const { return !ok || exit_requested; }
};

namespace detail {

// The type-erased half of an Arg<T>, so ArgParser can hold a heterogeneous
// list of them.
class ArgBase {
public:
    virtual ~ArgBase() = default;

    char short_name = 0;
    std::string long_name;
    std::string help_text;
    std::string metavar_text;
    bool takes_value = true;
    bool is_positional = false;
    bool is_required = false;
    bool was_present = false;

    [[nodiscard]] virtual std::string type_name() const = 0;
    [[nodiscard]] virtual std::string default_text() const = 0;
    [[nodiscard]] virtual bool has_default() const = 0;
    virtual bool assign(std::string_view text, std::string* error) = 0;
    virtual void assign_flag() {}

    [[nodiscard]] std::string display_name() const {
        if (is_positional) return long_name;
        return long_name.empty() ? std::string{"-"} + short_name : "--" + long_name;
    }
    [[nodiscard]] std::string metavar() const {
        return metavar_text.empty() ? type_name() : metavar_text;
    }
};

// Locale-independent conversion. Every failure produces the message the
// design specifies: "expected an integer, got 'x'".
bool convert_value(std::string_view text, std::string& out, std::string* error);
bool convert_value(std::string_view text, int& out, std::string* error);
bool convert_value(std::string_view text, double& out, std::string* error);
bool convert_value(std::string_view text, bool& out, std::string* error);

const char* type_name_of(const std::string*);
const char* type_name_of(const int*);
const char* type_name_of(const double*);
const char* type_name_of(const bool*);

std::string default_text_of(const std::string& v);
std::string default_text_of(int v);
std::string default_text_of(double v);
std::string default_text_of(bool v);

}  // namespace detail

// A typed argument. Created by ArgParser and owned by it; the reference stays
// valid for the parser's lifetime.
template <typename T>
class Arg final : public detail::ArgBase {
public:
    // Rejects an otherwise-valid value with a message of the caller's
    // choosing. This is the v1.1 validation hook, wired up now because the
    // shape of Arg<T> is the expensive part to change later.
    using Validator = std::function<bool(const T& value, std::string& error)>;

    Arg& default_value(T v) {
        value_ = std::move(v);
        has_default_ = true;
        is_required = false;  // a default is exactly what "optional" means
        return *this;
    }
    Arg& required(bool v = true) {
        is_required = v;
        return *this;
    }
    Arg& metavar(std::string m) {
        metavar_text = std::move(m);
        return *this;
    }
    Arg& help(std::string h) {
        help_text = std::move(h);
        return *this;
    }
    Arg& validate(Validator v) {
        validator_ = std::move(v);
        return *this;
    }

    // The parsed value, or the default if the argument was not given.
    [[nodiscard]] const T& value() const { return value_; }
    [[nodiscard]] T value_or(T fallback) const {
        return (was_present || has_default_) ? value_ : std::move(fallback);
    }
    // Whether it actually appeared on the command line.
    [[nodiscard]] bool present() const { return was_present; }

    [[nodiscard]] std::string type_name() const override {
        return detail::type_name_of(static_cast<const T*>(nullptr));
    }
    [[nodiscard]] bool has_default() const override { return has_default_; }
    [[nodiscard]] std::string default_text() const override {
        return has_default_ ? detail::default_text_of(value_) : std::string{};
    }

    bool assign(std::string_view text, std::string* error) override {
        T parsed{};
        if (!detail::convert_value(text, parsed, error)) return false;
        if (validator_) {
            std::string why;
            if (!validator_(parsed, why)) {
                if (error) *error = why;
                return false;
            }
        }
        value_ = std::move(parsed);
        was_present = true;
        return true;
    }

    void assign_flag() override {
        if constexpr (std::is_same_v<T, bool>) {
            value_ = true;
            was_present = true;
        }
    }

private:
    T value_{};
    bool has_default_ = false;
    Validator validator_;
};

// Declarative, GNU-style. The supported grammar, in full:
//
//   --long              --long=value        --long value
//   -s                  -svalue             -s value
//   -abc                (bundled boolean shorts)
//   --                  (everything after is positional)
//
// followed by positionals in declaration order.
class ArgParser {
public:
    explicit ArgParser(std::string program, std::string version = "",
                       std::string description = "");

    ArgParser(const ArgParser&) = delete;
    ArgParser& operator=(const ArgParser&) = delete;

    // ---- declaration ----

    Arg<bool>& flag(char short_name, std::string long_name, std::string help);
    Arg<bool>& flag(std::string long_name, std::string help);

    template <typename T>
    Arg<T>& option(char short_name, std::string long_name, std::string help) {
        auto owned = std::make_unique<Arg<T>>();
        Arg<T>& ref = *owned;
        ref.short_name = short_name;
        ref.long_name = std::move(long_name);
        ref.help_text = std::move(help);
        ref.takes_value = true;
        args_.push_back(std::move(owned));
        return ref;
    }

    template <typename T>
    Arg<T>& option(std::string long_name, std::string help) {
        return option<T>('\0', std::move(long_name), std::move(help));
    }

    // Positionals are required by default; call .required(false) or give a
    // default to make one optional.
    template <typename T>
    Arg<T>& positional(std::string name, std::string help) {
        auto owned = std::make_unique<Arg<T>>();
        Arg<T>& ref = *owned;
        ref.long_name = std::move(name);
        ref.help_text = std::move(help);
        ref.is_positional = true;
        ref.is_required = true;
        args_.push_back(std::move(owned));
        return ref;
    }

    // ---- parsing ----

    // argv[0] is ignored: the program name comes from the constructor, so the
    // help text does not change depending on how the binary was invoked.
    ParseResult parse(int argc, char** argv);
    ParseResult parse(const std::vector<std::string>& args);

    // ---- generated text ----

    [[nodiscard]] std::string help_text() const;
    [[nodiscard]] std::string version_text() const;
    [[nodiscard]] std::string usage_line() const;

    void set_epilog(std::string text) { epilog_ = std::move(text); }
    [[nodiscard]] const std::string& program() const { return program_; }

private:
    [[nodiscard]] detail::ArgBase* find_long(std::string_view name) const;
    [[nodiscard]] detail::ArgBase* find_short(char c) const;
    [[nodiscard]] std::vector<detail::ArgBase*> positionals() const;
    [[nodiscard]] std::vector<detail::ArgBase*> options() const;

    static ParseResult error(std::string message);

    std::string program_;
    std::string version_;
    std::string description_;
    std::string epilog_;
    std::vector<std::unique_ptr<detail::ArgBase>> args_;
    Arg<bool>* help_arg_ = nullptr;
    Arg<bool>* version_arg_ = nullptr;
};

}  // namespace modcurses
