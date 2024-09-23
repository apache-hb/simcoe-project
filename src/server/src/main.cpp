#include "stdafx.hpp"

#include "db/transaction.hpp"
#include "db/connection.hpp"

#include "threads/threads.hpp"

#include "logs/structured/message.hpp"

#include "net/net.hpp"

#include "packets/packets.hpp"

using namespace sm;

static sm::opt<bool> gRunAsClient {
    name = "client",
    desc = "Run as a client",
    init = false
};

static fmt_backtrace_t print_options_make(io_t *io) {
    fmt_backtrace_t print = {
        .options = {
            .arena = get_default_arena(),
            .io = io,
            .pallete = &kColourDefault,
        },
        .header = eHeadingGeneric,
        .config = eBtZeroIndexedLines,
        .project_source_path = SMC_SOURCE_DIR,
    };

    return print;
}

class DefaultSystemError final : public ISystemError {
    bt_report_t *mReport = nullptr;

    void error_begin(OsError error) override {
        bt_update();

        mReport = bt_report_new(get_default_arena());
        io_t *io = io_stderr();
        io_printf(io, "System error detected: (%s)\n", error.toString().c_str());
    }

    void error_frame(bt_address_t it) override {
        bt_report_add(mReport, it);
    }

    void error_end() override {
        const fmt_backtrace_t kPrintOptions = print_options_make(io_stderr());
        fmt_backtrace(kPrintOptions, mReport);
        std::exit(CT_EXIT_INTERNAL); // NOLINT
    }
};

static DefaultSystemError gDefaultError{};
static logs::FileChannel gFileChannel{};

struct LogWrapper {
    db::Environment env;
    db::Connection connection;

    LogWrapper()
        : env(db::Environment::create(db::DbType::eSqlite3))
        , connection(env.connect({ .host = "logs.db" }))
    {
        sm::logs::structured::setup(connection).throwIfFailed();
    }

    ~LogWrapper() {
        sm::logs::structured::cleanup();
    }
};

sm::UniquePtr<LogWrapper> gLogWrapper;

static void commonInit(void) {
    // bt_init();
    os_init();

    // TODO: popup window for panics and system errors
    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        bt_update();
        io_t *io = io_stderr();
        arena_t *arena = get_default_arena();

        const fmt_backtrace_t kPrintOptions = print_options_make(io);

        auto message = sm::vformat(msg, args);

        fmt::println(stderr, "{}", message);

        bt_report_t *report = bt_report_collect(arena);
        fmt_backtrace(kPrintOptions, report);

        std::exit(CT_EXIT_INTERNAL); // NOLINT
    };

    auto& logger = logs::getGlobalLogger();

    if (logs::isConsoleHandleAvailable())
        logger.addChannel(logs::getConsoleHandle());

    if (logs::isDebugConsoleAvailable())
        logger.addChannel(logs::getDebugConsole());

    if (auto file = logs::FileChannel::open("server.log"); file) {
        gFileChannel = std::move(file.value());
        logger.addChannel(gFileChannel);
    } else {
        logs::gGlobal.error("failed to open log file: {}", file.error());
    }

    gLogWrapper = sm::makeUnique<LogWrapper>();

    threads::init();
}

static constexpr net::IPv4Address kAddress = net::IPv4Address::loopback();
static constexpr uint16_t kPort = 9989;

__attribute__((target("crc32")))
static uint32_t crc32(const std::byte *data, size_t size) {
    uint32_t crc = 0;
    while (size > sizeof(uint64_t)) {
        crc = _mm_crc32_u64(crc, *reinterpret_cast<const uint64_t *>(data));
        data += sizeof(uint64_t);
        size -= sizeof(uint64_t);
    }

    while (size >= sizeof(uint8_t)) {
        crc = _mm_crc32_u8(crc, *reinterpret_cast<const uint8_t *>(data));
        data += sizeof(uint8_t);
        size -= sizeof(uint8_t);
    }

    return crc;
}

static int clientMain() {
    net::Network network = net::Network::create();

    auto socket = network.connect(kAddress, kPort);

    return 0;
}

static int serverMain() {
    const threads::CpuGeometry& geometry = threads::getCpuGeometry();

    threads::SchedulerConfig threadConfig = {
        .workers = 8,
        .priority = threads::PriorityClass::eNormal,
    };
    threads::Scheduler scheduler{threadConfig, geometry};

    bool running = true;

    net::Network network = net::Network::create();

    auto listener = network.bind(kAddress, kPort);

    listener.listen(8).throwIfFailed();

    while (running) {
        auto maybeSocket = listener.tryAccept();
        if (!maybeSocket && maybeSocket.error().cancelled()) {
            break;
        }

        if (!maybeSocket) {
            logs::gGlobal.error("failed to accept connection: {}", maybeSocket.error().message());
            continue;
        }

        net::Socket socket = std::move(maybeSocket.value());
        auto maybeHeader = socket.recv<game::PacketHeader>();
        if (!maybeHeader) {
            logs::gGlobal.error("failed to receive packet header: {}", maybeHeader.error().message());
            continue;
        }

        game::PacketHeader header = maybeHeader.value();

        std::byte buffer[512];
        size_t size = game::getPacketDataSize(header);
        auto maybeRead = socket.recvBytes(buffer, size);
        if (!maybeRead) {
            logs::gGlobal.error("failed to receive packet data: {}", maybeRead.error().message());
            continue;
        }

        auto read = maybeRead.value();
        if (read != size) {
            logs::gGlobal.error("failed to receive all packet data: expected {}, got {}", size, read);
            continue;
        }

        uint32_t crc = crc32(buffer, size);
        if (crc != header.crc) {
            logs::gGlobal.error("packet data crc mismatch: expected {:x}, got {:x}", header.crc, crc);
            continue;
        }

        switch (header.type) {
        case game::PacketType::eCreateAccountRequest: {
            auto *packet = reinterpret_cast<game::CreateAccountRequestPacket *>(buffer);
            logs::gGlobal.info("received create account request {} {}", packet->username, packet->password);
            break;
        }

        default: {
            logs::gGlobal.error("unknown packet type: {}", header.type);
            break;
        }
        }
    }

    return 0;
}

static int commonMain() noexcept try {
    logs::gGlobal.info("SMC_DEBUG = {}", SMC_DEBUG);
    logs::gGlobal.info("CTU_DEBUG = {}", CTU_DEBUG);

    if (gRunAsClient.getValue()) {
        return clientMain();
    } else {
        return serverMain();
    }
} catch (const std::exception& err) {
    logs::gGlobal.error("unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    logs::gGlobal.error("unknown unhandled exception");
    return -1;
}

int main(int argc, const char **argv) noexcept try {
    commonInit();
    defer { gLogWrapper.reset(); };

    sm::Span<const char*> args{argv, size_t(argc)};
    logs::gGlobal.info("args = [{}]", fmt::join(args, ", "));

    int result = [&] {
        sys::create(GetModuleHandleA(nullptr));
        defer { sys::destroy(); };

        if (int err = sm::parseCommandLine(argc, argv)) {
            return (err == -1) ? 0 : err; // TODO: this is a little silly, should wrap in a type
        }

        return commonMain();
    }();

    logs::gGlobal.info("editor exiting with {}", result);

    logs::shutdown();

    return result;
} catch (const db::DbException& err) {
    logs::gGlobal.error("database error: {} ({})", err.what(), err.code());
    return -1;
} catch (const std::exception& err) {
    logs::gGlobal.error("unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    logs::gGlobal.error("unknown unhandled exception");
    return -1;
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    commonInit();
    defer { gLogWrapper.reset(); };

    logs::gGlobal.info("lpCmdLine = {}", lpCmdLine);
    logs::gGlobal.info("nShowCmd = {}", nShowCmd);
    // TODO: parse lpCmdLine

    int result = [&] {
        sys::create(hInstance);
        defer { sys::destroy(); };

        return commonMain();
    }();

    logs::gGlobal.info("editor exiting with {}", result);

    logs::shutdown();

    return result;
}
