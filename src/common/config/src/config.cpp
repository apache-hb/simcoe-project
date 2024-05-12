#include "config/config.hpp"

#include "core/span.hpp"
#include "core/string.hpp"

#include "logs/logs.hpp"

using namespace sm;

static bool gOptionHelp = false;
static bool gOptionPix = false;
static bool gOptionWarp = false;
static bool gOptionDred = false;
static bool gOptionDebug = false;
static fs::path gOptionAppdir;
static LUID gOptionAdapterLuid = {0, 0};

bool sm::pix_enabled() {
    return gOptionPix;
}

bool sm::warp_enabled() {
    return gOptionWarp;
}

bool sm::dred_enabled() {
    return gOptionDred;
}

bool sm::debug_enabled() {
    return gOptionDebug;
}

fs::path sm::get_appdir() {
    return gOptionAppdir;
}

fs::path sm::get_redist(const fs::path& path) {
    return gOptionAppdir / "redist" / path;
}

std::optional<LUID> sm::override_adapter_luid() {
    if (gOptionAdapterLuid.LowPart == 0 && gOptionAdapterLuid.HighPart == 0) {
        return std::nullopt;
    }

    return gOptionAdapterLuid;
}

static void print() {
    logs::gGlobal.infoString(trimIndent(R"(
    |Usage: <app> [options]
    |Options:
    |    --help    : print this message
    |    --pix     : enable PIX
    |    --warp    : enable WARP
    |    --dred    : enable DRED
    |    --debug   : enable debug
    |    --appdir  : set application directory
    |    --adapter : <high>:<low> set adapter LUID
    )"));
}

static void parse(sm::Span<const char*> args) {
    for (size_t i = 1; i < args.size(); ++i) {
        sm::StringView view{args[i]};
        if (view == "--help") {
            gOptionHelp = true;
        } else if (view == "--pix") {
            gOptionPix = true;
        } else if (view == "--warp") {
            gOptionWarp = true;
        } else if (view == "--dred") {
            gOptionDred = true;
        } else if (view == "--debug") {
            gOptionDebug = true;
        } else if (view == "--appdir") {
            if (i + 1 < args.size()) {
                gOptionAppdir = fs::canonical(fs::current_path() / args[i + 1]);
                ++i;
            } else {
                logs::gGlobal.error("missing argument for --appdir");
            }
        } else if (view == "--adapter") {
            if (i + 1 < args.size()) {
                sm::StringView arg = args[i + 1];
                size_t index = arg.find(':');
                if (index != sm::StringView::npos) {
                    auto high = arg.substr(0, index);
                    auto low = arg.substr(index + 1);
                    gOptionAdapterLuid.HighPart = std::stoul(high.data(), nullptr, 16);
                    gOptionAdapterLuid.LowPart = std::stoul(low.data(), nullptr, 16);
                } else {
                    logs::gGlobal.error("invalid argument for --adapter");
                }
                ++i;
            } else {
                logs::gGlobal.error("missing argument for --adapter");
            }
        } else {
            logs::gGlobal.error("unknown argument: {}", view);
        }
    }
}

bool sm::parse_command_line(int argc, const char **argv, const fs::path& appdir) {
    gOptionAppdir = appdir;
    sm::Span<const char*> args{argv, size_t(argc)};
    parse(args);

    if (gOptionHelp) {
        print();
        return false;
    }

    return true;
}
