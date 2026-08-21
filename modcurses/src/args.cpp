#include "modcurses/args.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace modcurses {
namespace detail {

// ---------------------------------------------------------------- conversion

bool convert_value(std::string_view text, std::string& out, std::string*) {
    out.assign(text);
    return true;
}

bool convert_value(std::string_view text, int& out, std::string* error) {
    int parsed = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto res = std::from_chars(begin, end, parsed);
    if (res.ec != std::errc{} || res.ptr != end) {
        if (error) *error = "expected an integer, got '" + std::string{text} + "'";
        return false;
    }
    out = parsed;
    return true;
}

bool convert_value(std::string_view text, double& out, std::string* error) {
    const std::string owned{text};  // both paths below need a contiguous buffer
    bool ok = false;
    double parsed = 0.0;

#if defined(__cpp_lib_to_chars)
    const char* begin = owned.data();
    const char* end = owned.data() + owned.size();
    const auto res = std::from_chars(begin, end, parsed);
    ok = res.ec == std::errc{} && res.ptr == end;
#else
    // libstdc++ only gained floating-point from_chars in GCC 11. strtod is
    // the fallback, and it is locale-independent HERE for a reason worth
    // knowing: App parses arguments before the terminal calls
    // setlocale(LC_ALL, ""), so LC_NUMERIC is still "C" and the decimal point
    // is still '.'.
    char* stop = nullptr;
    parsed = std::strtod(owned.c_str(), &stop);
    ok = stop != nullptr && *stop == '\0' && !owned.empty();
#endif

    if (!ok) {
        if (error) *error = "expected a number, got '" + owned + "'";
        return false;
    }
    out = parsed;
    return true;
}

bool convert_value(std::string_view text, bool& out, std::string* error) {
    if (text == "1" || text == "true" || text == "yes" || text == "on") {
        out = true;
        return true;
    }
    if (text == "0" || text == "false" || text == "no" || text == "off") {
        out = false;
        return true;
    }
    if (error) *error = "expected true or false, got '" + std::string{text} + "'";
    return false;
}

const char* type_name_of(const std::string*) { return "string"; }
const char* type_name_of(const int*) { return "int"; }
const char* type_name_of(const double*) { return "number"; }
const char* type_name_of(const bool*) { return "bool"; }

std::string default_text_of(const std::string& v) { return v; }
std::string default_text_of(int v) { return std::to_string(v); }

std::string default_text_of(double v) {
    // std::to_string on a double gives "4.000000"; trim it back to something
    // a help screen can show.
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << v;
    return out.str();
}

std::string default_text_of(bool v) { return v ? "true" : "false"; }

}  // namespace detail

// ----------------------------------------------------------------- ArgParser

ArgParser::ArgParser(std::string program, std::string version, std::string description)
    : program_(std::move(program)),
      version_(std::move(version)),
      description_(std::move(description)) {
    // Registered first so a user-declared -h or --version wins the lookup
    // (find_* returns the first match, and these sit at the front only for
    // help-text ordering - see options(), which moves them to the end).
    help_arg_ = &flag('h', "help", "show this help and exit");
    if (!version_.empty()) version_arg_ = &flag('V', "version", "show the version and exit");
}

Arg<bool>& ArgParser::flag(char short_name, std::string long_name, std::string help) {
    auto owned = std::make_unique<Arg<bool>>();
    Arg<bool>& ref = *owned;
    ref.short_name = short_name;
    ref.long_name = std::move(long_name);
    ref.help_text = std::move(help);
    ref.takes_value = false;
    args_.push_back(std::move(owned));
    return ref;
}

Arg<bool>& ArgParser::flag(std::string long_name, std::string help) {
    return flag('\0', std::move(long_name), std::move(help));
}

detail::ArgBase* ArgParser::find_long(std::string_view name) const {
    for (const auto& a : args_)
        if (!a->is_positional && a->long_name == name) return a.get();
    return nullptr;
}

detail::ArgBase* ArgParser::find_short(char c) const {
    for (const auto& a : args_)
        if (!a->is_positional && a->short_name == c && c != '\0') return a.get();
    return nullptr;
}

std::vector<detail::ArgBase*> ArgParser::positionals() const {
    std::vector<detail::ArgBase*> out;
    for (const auto& a : args_)
        if (a->is_positional) out.push_back(a.get());
    return out;
}

std::vector<detail::ArgBase*> ArgParser::options() const {
    // Declaration order, except that the built-in --help and --version are
    // listed last however early they were registered.
    std::vector<detail::ArgBase*> out;
    for (const auto& a : args_)
        if (!a->is_positional && a.get() != help_arg_ && a.get() != version_arg_)
            out.push_back(a.get());
    if (help_arg_) out.push_back(help_arg_);
    if (version_arg_) out.push_back(version_arg_);
    return out;
}

ParseResult ArgParser::error(std::string message) {
    ParseResult r;
    r.ok = false;
    r.exit_code = 2;  // the conventional shell code for a usage error
    r.message = "error: " + std::move(message);
    return r;
}

ParseResult ArgParser::parse(int argc, char** argv) {
    std::vector<std::string> tokens;
    for (int i = 1; i < argc; ++i) tokens.emplace_back(argv[i]);  // argv[0] ignored
    return parse(tokens);
}

ParseResult ArgParser::parse(const std::vector<std::string>& tokens) {
    // Re-parsing is legal: start from a clean slate rather than accumulating.
    for (auto& a : args_) a->was_present = false;

    const auto pos_args = positionals();
    std::size_t next_positional = 0;
    bool only_positionals = false;
    std::string convert_error;

    const auto take_positional = [&](const std::string& text) -> std::optional<ParseResult> {
        if (next_positional >= pos_args.size())
            return error("unexpected argument '" + text + "'");
        detail::ArgBase* target = pos_args[next_positional++];
        if (!target->assign(text, &convert_error))
            return error(target->display_name() + ": " + convert_error);
        return std::nullopt;
    };

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];

        if (only_positionals) {
            if (auto err = take_positional(token)) return *err;
            continue;
        }

        // ---- "--" : everything after this is positional, even "-x" ----
        if (token == "--") {
            only_positionals = true;
            continue;
        }

        // ---- --long, --long=value, --long value ----
        if (token.size() > 2 && token[0] == '-' && token[1] == '-') {
            std::string name = token.substr(2);
            std::optional<std::string> inline_value;
            if (const auto eq = name.find('='); eq != std::string::npos) {
                inline_value = name.substr(eq + 1);
                name = name.substr(0, eq);
            }

            detail::ArgBase* target = find_long(name);
            if (target == nullptr) return error("unknown option '--" + name + "'");

            if (!target->takes_value) {
                if (inline_value) return error("--" + name + " does not take a value");
                target->assign_flag();
                continue;
            }

            std::string value;
            if (inline_value) {
                value = *inline_value;
            } else {
                if (i + 1 >= tokens.size())
                    return error(target->display_name() + ": expected a value");
                value = tokens[++i];
            }
            if (!target->assign(value, &convert_error))
                return error(target->display_name() + ": " + convert_error);
            continue;
        }

        // ---- -s, -svalue, -s value, -abc ----
        if (token.size() > 1 && token[0] == '-') {
            bool consumed_next = false;
            for (std::size_t c = 1; c < token.size(); ++c) {
                detail::ArgBase* target = find_short(token[c]);
                if (target == nullptr)
                    return error("unknown option '-" + std::string(1, token[c]) + "'");

                if (!target->takes_value) {
                    target->assign_flag();
                    continue;  // keep unbundling: -abc
                }

                // An option that takes a value ends the bundle: the rest of
                // the token is its value, or the next token is.
                std::string value;
                if (c + 1 < token.size()) {
                    value = token.substr(c + 1);
                } else {
                    if (i + 1 >= tokens.size())
                        return error(target->display_name() + ": expected a value");
                    value = tokens[i + 1];
                    consumed_next = true;
                }
                if (!target->assign(value, &convert_error))
                    return error(target->display_name() + ": " + convert_error);
                break;
            }
            if (consumed_next) ++i;
            continue;
        }

        // ---- a bare "-" or anything else is positional ----
        if (auto err = take_positional(token)) return *err;
    }

    // ---- the built-ins, checked before anything is reported as missing ----
    if (help_arg_ != nullptr && help_arg_->value()) {
        ParseResult r;
        r.exit_requested = true;
        r.exit_code = 0;
        r.message = help_text();
        return r;
    }
    if (version_arg_ != nullptr && version_arg_->value()) {
        ParseResult r;
        r.exit_requested = true;
        r.exit_code = 0;
        r.message = version_text();
        return r;
    }

    for (const auto& a : args_) {
        if (!a->is_required || a->was_present) continue;
        if (a->is_positional) return error("missing required argument '" + a->long_name + "'");
        return error("missing required option '" + a->display_name() + "'");
    }

    return ParseResult{};
}

// -------------------------------------------------------------- generated text

std::string ArgParser::usage_line() const {
    std::string out = "Usage: " + program_;
    if (!options().empty()) out += " [options]";
    for (const detail::ArgBase* p : positionals()) {
        out += ' ';
        out += p->is_required ? "<" + p->long_name + ">" : "[<" + p->long_name + ">]";
    }
    return out;
}

std::string ArgParser::version_text() const {
    if (version_.empty()) return program_;
    return program_ + " " + version_;
}

std::string ArgParser::help_text() const {
    // Left column of every row, so the two tables share one alignment.
    const auto option_label = [](const detail::ArgBase* a) {
        std::string label;
        if (a->short_name != '\0') {
            label += '-';
            label += a->short_name;
            if (!a->long_name.empty()) label += ", ";
        } else {
            label += "    ";
        }
        if (!a->long_name.empty()) label += "--" + a->long_name;
        if (a->takes_value) label += " <" + a->metavar() + ">";
        return label;
    };

    const auto pos_args = positionals();
    const auto opt_args = options();

    std::size_t width = 0;
    for (const detail::ArgBase* a : pos_args) width = std::max(width, a->long_name.size());
    for (const detail::ArgBase* a : opt_args) width = std::max(width, option_label(a).size());
    // Wide labels get their own line rather than pushing every description off
    // the right-hand edge.
    width = std::min<std::size_t>(width, 28);

    std::string out;
    if (!description_.empty()) out += version_text() + " - " + description_ + "\n\n";
    out += usage_line() + "\n";

    const auto row = [&](const std::string& label, const std::string& help,
                         const std::string& suffix) {
        out += "  " + label;
        if (label.size() > width) {
            out += "\n  " + std::string(width, ' ');
        } else {
            out += std::string(width - label.size(), ' ');
        }
        out += "  " + help + suffix + "\n";
    };

    if (!pos_args.empty()) {
        out += "\nArguments:\n";
        for (const detail::ArgBase* a : pos_args) {
            std::string suffix;
            if (a->has_default()) suffix = " (default: " + a->default_text() + ")";
            row(a->long_name, a->help_text, suffix);
        }
    }
    if (!opt_args.empty()) {
        out += "\nOptions:\n";
        for (const detail::ArgBase* a : opt_args) {
            std::string suffix;
            if (a->has_default()) suffix = " (default: " + a->default_text() + ")";
            row(option_label(a), a->help_text, suffix);
        }
    }
    if (!epilog_.empty()) out += "\n" + epilog_ + "\n";
    return out;
}

}  // namespace modcurses
