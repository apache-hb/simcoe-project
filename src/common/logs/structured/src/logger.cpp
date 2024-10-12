#include "stdafx.hpp"

#include "logs/structured/logger.hpp"

namespace structured = sm::logs::structured;

using MessageInfo = sm::logs::structured::MessageInfo;

static bool hasInvariantTsc() noexcept {
    union {
        int data[4];
        uint32_t regs[4];
    };

    __cpuid(data, 0x80000007);

    uint32_t edx = regs[3];

    fmt::println(stderr, "Invariant TSC: {}", edx & (1 << 8));
    return edx & (1 << 8);
}

static uint64_t getInvariantTscRatio() noexcept {
    union {
        int data[4];
        uint32_t regs[4];
    };

    // Intel SDM Vol. 2A 3-237 - Table 1-17.
    __cpuid(data, 0x15);
    uint32_t eax = regs[0]; // denominator of TSC to core crystal clock ratio
    uint32_t ebx = regs[1]; // numerator of TSC to core crystal clock ratio
    uint32_t ecx = regs[2]; // core crystal clock frequency in MHz

    fmt::println(stderr, "TSC ratio: {}/{}, crystal clock: {} MHz", eax, ebx, ecx);

    if (ebx == 0 || ecx == 0)
        return 1;

    return (ecx * eax) / ebx;
}

// theres probably like 2 or 3 ticks of difference between these
// but it's not really a big deal
static const auto kStartTime = std::chrono::system_clock::now();
static const auto kStartTicks = std::chrono::high_resolution_clock::now();
static const auto kTscRatio = getInvariantTscRatio();

static std::chrono::milliseconds getTimestamp() noexcept {
    // use kStartTime and kStartTicks to calculate current time in milliseconds
    // using high_resolution_clock is much faster than system_clock on windows
    auto now = std::chrono::high_resolution_clock::now();
    auto ticksSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(now - kStartTicks);
    auto timeSinceStart = kStartTime + ticksSinceStart;

    return std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceStart.time_since_epoch());
}

static std::chrono::milliseconds readTscTimestamp() noexcept {
    unsigned long long tsc = __rdtsc();
    auto ms = tsc / kTscRatio;
    auto timeSinceStart = kStartTime + std::chrono::milliseconds(ms);

    return std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceStart.time_since_epoch());
}

static bool useTscForTimestamp() noexcept {
    return hasInvariantTsc() && kTscRatio != 1;
}

static std::chrono::milliseconds (*gTimestampAccess)() noexcept = useTscForTimestamp() ? &readTscTimestamp : &getTimestamp;

void structured::Logger::addChannel(std::unique_ptr<ILogChannel>&& channel) {
    channel->attach();
    mChannels.emplace_back(std::move(channel));
}

void structured::Logger::setAsyncChannel(std::unique_ptr<IAsyncLogChannel>&& channel) {
    channel->attach();
    mAsyncChannel = std::move(channel);
}

void structured::Logger::destroy() noexcept {
    mChannels.clear();
}

void structured::Logger::postMessage(const MessageInfo& message, std::unique_ptr<DynamicArgStore> args) noexcept {
    uint64_t timestamp = getTimestamp().count();

    for (auto& channel : mChannels) {
        MessagePacket packet = {
            .message = message,
            .timestamp = timestamp,
            .args = *args
        };
        channel->postMessage(packet);
    }

    if (mAsyncChannel == nullptr)
        return;

    AsyncMessagePacket packet = {
        .message = message,
        .timestamp = timestamp,
        .args = std::move(args)
    };
    mAsyncChannel->postMessageAsync(std::move(packet));
}

structured::Logger& structured::Logger::instance() noexcept {
    static Logger sInstance;
    return sInstance;
}
