// Copyright (C) 2020 averne
//
// This file is part of Fizeau.
//
// Fizeau is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Fizeau is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Fizeau.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cstdint>
#include <switch.h>
#include <stratosphere.hpp>

#include "types.h"

namespace fz {

class Clock {
    public:
        static Result initialize() {
            R_TRY(smInitialize());
            ON_SCOPE_EXIT { smExit(); };
            R_TRY(timeInitialize());
            ON_SCOPE_EXIT { timeExit(); };

            std::uint64_t time;
            TimeCalendarTime caltime;
            Clock::tick = armGetSystemTick();
            R_TRY(timeGetCurrentTime(TimeType_Default, &time));
            R_TRY(timeToCalendarTimeWithMyRule(time, &caltime, nullptr));
            Clock::timestamp = ams::TimeSpan::FromSeconds(60 * 60 * caltime.hour + 60 * caltime.minute + caltime.second);
            return 0;
        }

        static Time get_current_time() {
            auto ts = Clock::timestamp + ams::TimeSpan::FromNanoSeconds(armTicksToNs(armGetSystemTick() - Clock::tick));
            return {static_cast<std::uint8_t>(ts.GetHours() % 24),
                static_cast<std::uint8_t>(ts.GetMinutes() % 60), static_cast<std::uint8_t>(ts.GetSeconds() % 60)};
        }

        static bool is_in_interval(const Time &cur, const Time &lo, const Time &hi) {
            return (lo <= cur) && (cur < hi);
        }

        static bool is_in_interval(const Time &lo, const Time &hi) {
            return Clock::is_in_interval(Clock::get_current_time(), lo, hi);
        }

    private:
        static inline std::uint64_t tick      = 0;
        static inline ams::TimeSpan timestamp = 0;
};

} // namespace fz
