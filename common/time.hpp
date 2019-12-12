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

inline u64 get_timestamp() {
    u64 ts = 0;
    timeGetCurrentTime(TimeType_LocalSystemClock, &ts);
    return ts;
}

inline Time get_time() {
    Time time = {};
    TimeCalendarTime caltime;
    TRY_RETURNV(timeToCalendarTimeWithMyRule(get_timestamp(), &caltime, nullptr), time);
    time.hour = caltime.hour, time.minute = caltime.minute, time.second = caltime.second;
    return time;
}

} // namespace fz
