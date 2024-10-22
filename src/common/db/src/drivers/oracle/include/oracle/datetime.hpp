#pragma once

#include "dao/dao.hpp"

#include <chrono>

#include <oci.h>

namespace sm::db::oracle {
    using DateTime = sm::db::DateTime;

    struct DateTimeComponents {
        sb2 year;
        ub1 month;
        ub1 day;
        ub1 hour;
        ub1 minute;
        ub1 second;
        ub4 fsec;

        constexpr DateTime toDateTime() const noexcept {
            std::chrono::year y{year};
            std::chrono::month m{month};
            std::chrono::day d{day};
            std::chrono::hours hours{hour};
            std::chrono::minutes minutes{minute};
            std::chrono::seconds seconds{second};
            std::chrono::milliseconds fsecs{fsec};
            return (std::chrono::sys_days{y/m/d} + hours + minutes + seconds + fsecs);
        }

        static constexpr DateTimeComponents fromDateTime(DateTime value) noexcept {
            auto days = std::chrono::floor<std::chrono::days>(value);
            std::chrono::year_month_day ymd = std::chrono::floor<std::chrono::days>(days);
            std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::milliseconds>(value - days)};

            return DateTimeComponents {
                .year = static_cast<sb2>((int)ymd.year()),
                .month = static_cast<ub1>((unsigned)ymd.month()),
                .day = static_cast<ub1>((unsigned)ymd.day()),
                .hour = static_cast<ub1>(hms.hours().count()),
                .minute = static_cast<ub1>(hms.minutes().count()),
                .second = static_cast<ub1>(hms.seconds().count()),
                .fsec = static_cast<ub4>(hms.subseconds().count()),
            };
        }

        constexpr auto operator<=>(const DateTimeComponents&) const = default;

        constexpr static void tests() {
            using namespace std::chrono_literals;

            static_assert(DateTimeComponents::fromDateTime(DateTime{std::chrono::sys_days(2021y/1/1)}) == DateTimeComponents{2021, 1, 1, 0, 0, 0, 0});
            static_assert(DateTimeComponents::fromDateTime(DateTime{std::chrono::sys_days(2021y/1/1) + 1h + 1min + 1s + 1ms}) == DateTimeComponents{2021, 1, 1, 1, 1, 1, 1});

            static_assert(DateTimeComponents{2021, 1, 1, 0, 0, 0, 0}.toDateTime() == DateTime{std::chrono::sys_days(2021y/1/1)});
            static_assert(DateTimeComponents{2021, 1, 1, 1, 1, 1, 1}.toDateTime() == DateTime{std::chrono::sys_days(2021y/1/1) + 1h + 1min + 1s + 1ms});
        }
    };
}
