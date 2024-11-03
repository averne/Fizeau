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

#include <array>
#include <string_view>
#include <switch.h>

#include "fizeau.h"
#include "types.h"

namespace fz {

class Config {
    public:
        constexpr static FizeauSettings default_settings = {
            .temperature = DEFAULT_TEMP,
            .saturation  = DEFAULT_SAT,
            .hue         = DEFAULT_HUE,
            .contrast    = DEFAULT_CONTRAST,
            .gamma       = DEFAULT_GAMMA,
            .luminance   = DEFAULT_LUMA,
            .range       = DEFAULT_RANGE,
        };

    public:
        constinit static inline std::array config_locations = {
            std::string_view("/switch/Fizeau/config.ini"),
            std::string_view("/config/Fizeau/config.ini"),
        };

    public:
        bool active = true, has_active_override = false;

        FizeauProfileId cur_profile_id = FizeauProfileId_Invalid,
            internal_profile = FizeauProfileId_Profile1, external_profile = FizeauProfileId_Profile1;
        bool is_editing_day_profile = false, is_editing_night_profile = false;

        FizeauProfile profile = {
            .day_settings   = Config::default_settings,
            .night_settings = Config::default_settings,
            .components     = Component_All,
            .filter         = Component_None,
        };

        void (*parse_profile_switch_action)(Config *, FizeauProfileId) = nullptr;

    public:
        static int ini_handler(void *user, const char *section, const char *name, const char *value);
        static std::string_view find_config();

    public:
        void read();
        void write();
        std::string make();

        Result update();
        Result apply();
        Result reset();
        Result open_profile(FizeauProfileId id);

    private:
        void sanitize_profile();
};

} // namespace fz
