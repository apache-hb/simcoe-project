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

static void print() {
    logs::gGlobal.info("Usage: <app> [options]");
    logs::gGlobal.info("Options:");
    logs::gGlobal.info("  --help    print this message");
    logs::gGlobal.info("  --pix     enable PIX");
    logs::gGlobal.info("  --warp    enable WARP");
    logs::gGlobal.info("  --dred    enable DRED");
    logs::gGlobal.info("  --debug   enable debug");
    logs::gGlobal.info("  --appdir  set application directory");
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
