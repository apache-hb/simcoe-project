#pragma once

#include <fmtlib/format.h>
#include <ostream>

class Writer {
    int mDepth = 0;
    std::ostream &mStream;

public:
    Writer(std::ostream &stream) : mStream(stream) {}

    void writeIndent() {
        mStream << std::string(mDepth, '\t');
    }

    template<typename... A>
    void print(fmt::format_string<A...> fmt, A&&... args) {
        mStream << fmt::vformat(fmt, fmt::make_format_args(args...));
    }

    template<typename... A>
    Writer& write(fmt::format_string<A...> fmt, A&&... args) {
        writeIndent();
        mStream << fmt::vformat(fmt, fmt::make_format_args(args...));
        return *this;
    }

    template<typename... A>
    Writer& writeln(fmt::format_string<A...> fmt, A&&... args) {
        writeIndent();
        mStream << fmt::vformat(fmt, fmt::make_format_args(args...));
        mStream << '\n';
        return *this;
    }

    Writer& writeln() {
        mStream << '\n';
        return *this;
    }

    void indent() {
        mDepth += 1;
    }

    void dedent() {
        mDepth -= 1;
    }
};
