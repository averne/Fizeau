// Copyright (c) 2024 averne
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

#include "types.h"

namespace fz {

class Clock {
    public:
        static Result initialize() {
            Result rc;
            if (rc = timeInitialize(); R_FAILED(rc))
                goto exit;

            Clock::tick = armGetSystemTick();

            std::uint64_t time;
            if (rc = timeGetCurrentTime(TimeType_Default, &time); R_FAILED(rc))
                goto exit;

            TimeCalendarTime caltime;
            if (rc = timeToCalendarTimeWithMyRule(time, &caltime, nullptr); R_FAILED(rc))
                goto exit;

            Clock::timestamp = 1'000'000'000ull * (60 * 60 * caltime.hour + 60 * caltime.minute + caltime.second);

            rc = 0;

exit:
            timeExit();
            return rc;
        }

        static Time get_current_time() {
            auto ts = Clock::timestamp + armTicksToNs(armGetSystemTick() - Clock::tick);
            auto [h, m, s]  = from_timestamp(ts / 1'000'000'000);
            return { std::uint8_t(h % 24), m, s };
        }

        static constexpr bool is_in_interval(const Time &cur, const Time &lo, const Time &hi) {
            return (lo <= cur) && (cur < hi);
        }

        static bool is_in_interval(const Time &lo, const Time &hi) {
            return Clock::is_in_interval(Clock::get_current_time(), lo, hi);
        }

    private:
        static inline std::uint64_t tick      = 0;
        static inline std::uint64_t timestamp = 0;
};

} // namespace fz
