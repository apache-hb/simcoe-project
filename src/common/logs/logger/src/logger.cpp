#include "stdafx.hpp"

#include "logs/logger.hpp"

#include "timer.hpp"

#if CT_HAS_TSC_TIMESOURCE
#   include "core/cpuid.hpp"
#endif

namespace logs = sm::logs;
namespace detail = sm::logs::detail;
namespace chrono = std::chrono;

using MessageInfo = sm::logs::MessageInfo;

using SystemTimePoint = chrono::system_clock::time_point;
using PreciseTimePoint = chrono::high_resolution_clock::time_point;

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
#if CT_HAS_TSC_TIMESOURCE
    // Intel SDM Vol. 2A 3-247 - Table 1-17.
    CpuId cpuid = CpuId::of(0x80000007);
    return cpuid.edx & (1 << 8); // Bit 08: Invariant TSC available if 1
#else
    return false;
#endif
}

static detail::HighResolutionSource gHighResolutionSource;

#if CT_HAS_TSC_TIMESOURCE
static detail::InvariantTscSource gInvariantTscSource;
#endif

static detail::ITimeSource *gTimeSource = &gHighResolutionSource;

void logs::create(LoggingConfig config) {
    // if invariant TSC is available, use it
    if (config.timer == TimerSource::eAutoDetect && hasInvariantTsc())
        config.timer = TimerSource::eInvariantTsc;

#if CT_HAS_TSC_TIMESOURCE
    if (config.timer == TimerSource::eInvariantTsc) {
        gTimeSource = &gInvariantTscSource;
    } else {
        gTimeSource = &gHighResolutionSource;
    }
#else
    gTimeSource = &gHighResolutionSource;
#endif
}

uint64_t logs::getCurrentTime() noexcept {
    return getCurrentTime(*gTimeSource).count();
}

void logs::Logger::addChannel(std::unique_ptr<ILogChannel>&& channel) {
    channel->attach();
    mChannels.emplace_back(std::move(channel));
}

void logs::Logger::setAsyncChannel(std::unique_ptr<IAsyncLogChannel>&& channel) {
    channel->attach();
    mAsyncChannel = std::move(channel);
}

void logs::Logger::destroy() noexcept {
    mChannels.clear();
}

void logs::Logger::postMessage(const MessageInfo& message, std::unique_ptr<DynamicArgStore> args) noexcept {
    uint64_t timestamp = logs::getCurrentTime();

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

logs::Logger& logs::Logger::instance() noexcept {
    static Logger sInstance;
    return sInstance;
}

std::string_view logs::toString(Severity severity) noexcept {
    switch (severity) {
    case Severity::eTrace: return "TRACE";
    case Severity::eDebug: return "DEBUG";
    case Severity::eInfo: return "INFO";
    case Severity::eWarning: return "WARN";
    case Severity::eError: return "ERROR";
    case Severity::eFatal: return "FATAL";
    case Severity::ePanic: return "PANIC";
    default: return "UNK";
    }
}
