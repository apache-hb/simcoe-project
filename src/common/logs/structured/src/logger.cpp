#include "stdafx.hpp"

#include "logs/structured/logger.hpp"

#include "timer.hpp"

namespace structured = sm::logs::structured;
namespace detail = sm::logs::structured::detail;
namespace chrono = std::chrono;

using MessageInfo = sm::logs::structured::MessageInfo;

using SystemTimePoint = chrono::system_clock::time_point;
using PreciseTimePoint = chrono::high_resolution_clock::time_point;

union CpuId {
    int32_t iregs[4];
    uint32_t uregs[4];

    struct {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
    };

    static CpuId get(int leaf) noexcept {
        CpuId id;
        __cpuid(id.iregs, leaf);
        return id;
    }
};

#if 0
static uint64_t getInvariantTscRatio() noexcept {
    // Intel SDM Vol. 2A 3-237 - Table 1-17.
    CpuId cpuid = CpuId::get(0x15);
    uint32_t den = cpuid.eax; // denominator of TSC to core crystal clock ratio
    uint32_t num = cpuid.ebx; // numerator of TSC to core crystal clock ratio
    uint32_t clk = cpuid.ecx; // core crystal clock frequency in MHz

    if (num == 0 || clk == 0)
        return 0;

    return (uint64_t(den) * clk) / num;
}
#endif

static bool hasInvariantTsc() noexcept {
    // Intel SDM Vol. 2A 3-247 - Table 1-17.
    CpuId cpuid = CpuId::get(0x80000007);
    return cpuid.edx & (1 << 8); // Bit 08: Invariant TSC available if 1
}

static detail::HighResolutionSource gHighResolutionSource;
static detail::InvariantTscSource gInvariantTscSource;
static detail::ITimeSource *gTimeSource = &gHighResolutionSource;

void structured::create(LoggingConfig config) {
    // if invariant TSC is available, use it
    if (config.timer == TimerSource::eAutoDetect && hasInvariantTsc())
        config.timer = TimerSource::eInvariantTsc;

    if (config.timer == TimerSource::eInvariantTsc) {
        gTimeSource = &gInvariantTscSource;
    } else {
        gTimeSource = &gHighResolutionSource;
    }
}

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
    uint64_t timestamp = getCurrentTime(*gTimeSource).count();

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
