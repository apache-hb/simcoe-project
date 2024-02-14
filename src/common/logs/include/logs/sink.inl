#pragma once

#include "logs/sink.hpp" // IWYU pragma: export

#include <iterator>

template<sm::logs::Category::Inner C> requires(sm::logs::Category{C}.is_valid())
void sm::logs::Sink<C>::log(Severity severity, std::string_view msg, auto &&...args) const {
    // while ILogger will reject the message, still do an early return
    // to avoid formatting if we don't need to
    if (m_logger.will_accept(severity)) {
        auto out = std::back_inserter(m_buffer);
        fmt::vformat_to(out, msg, fmt::make_format_args(args...));

        m_logger.log(C, severity, std::string_view{m_buffer.data(), m_buffer.size()});

        m_buffer.clear();
    }
}
