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

#include <string_view>
#include <switch.h>

#include "fizeau.h"
#include "types.h"

namespace fz::cfg {

constexpr std::array config_locations = {
    "/switch/Fizeau/config.ini",
    "/config/Fizeau/config.ini"
};

struct Config {
    bool active = true, has_active_override = false;

    FizeauProfile cur_profile = {};
    FizeauProfileId cur_profile_id = FizeauProfileId_Invalid,
        active_internal_profile = FizeauProfileId_Invalid, active_external_profile = FizeauProfileId_Invalid;
    bool is_editing_day_profile = false, is_editing_night_profile = false;

    Time dusk_begin, dusk_end;
    Time dawn_begin, dawn_end;

    Temperature temperature_day, temperature_night;
    ColorFilter filter_day,      filter_night;
    Gamma       gamma_day,       gamma_night;
    Luminance   luminance_day,   luminance_night;
    ColorRange  range_day,       range_night;
    Brightness  brightness_day,  brightness_night;

    Time dimming_timeout;
};

std::string_view find_config();
Config read();
std::string make(Config &config);
void dump(Config &config);

Result update(Config &config);
Result apply(Config &config);
Result reset(Config &config);
Result open_profile(Config &cfg, FizeauProfileId id);

} // namespace fz::cfg
