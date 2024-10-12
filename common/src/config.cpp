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

#include <cstring>
#include <algorithm>
#include <string>
#include <ini.h>
#include <common.hpp>
#include <sys/stat.h>

#include "fizeau.h"
#include "config.hpp"

namespace fz {

#define COMMENT ";"

std::string_view Config::find_config() {
    struct stat tmp;
    for (auto loc: config_locations) {
        if (::stat(loc.data(), &tmp) == 0)
            return loc;
    }
    return config_locations[1];
};

void Config::read() {
    if (!this->parse_profile_switch_action) {
        this->parse_profile_switch_action = +[](Config *self, FizeauProfileId profile_id) {
            if (self->cur_profile_id != FizeauProfileId_Invalid)
                if (auto rc = self->apply(); R_FAILED(rc))
                    LOG("Failed to apply config: %#x\n", rc);
            if (auto rc = self->open_profile(profile_id); R_FAILED(rc))
                LOG("Failed to open profile: %#x\n", rc);
        };
    }

    auto loc = Config::find_config();
    ini_parse(loc.data(), Config::ini_handler, this);

    if (this->cur_profile_id != FizeauProfileId_Invalid) {
        if (auto rc = this->apply(); R_FAILED(rc))
            LOG("Failed to apply config: %#x\n", rc);
    }
}

void Config::sanitize_profile() {
    auto sanitize_minmax = []<typename T>(T& val, auto min, auto max) {
        val = std::clamp(val, static_cast<T>(min), static_cast<T>(max));
    };

    auto sanitize_time = [&](Time &time) {
        sanitize_minmax(time.h, 0, 24);
        sanitize_minmax(time.m, 0, 60);
        sanitize_minmax(time.s, 0, 60);
    };

    auto sanitize_colorrange = [&](ColorRange &range) {
        sanitize_minmax(range.lo, MIN_RANGE, MAX_RANGE);
        sanitize_minmax(range.hi, MIN_RANGE, MAX_RANGE);
    };

    sanitize_time(this->profile.dusk_begin);
    sanitize_time(this->profile.dusk_end);
    sanitize_time(this->profile.dawn_begin);
    sanitize_time(this->profile.dawn_end);

    sanitize_minmax(this->profile.day_settings  .temperature, MIN_TEMP,  MAX_TEMP);
    sanitize_minmax(this->profile.night_settings.temperature, MIN_TEMP,  MAX_TEMP);

    sanitize_minmax(this->profile.day_settings  .gamma,       MIN_GAMMA, MAX_GAMMA);
    sanitize_minmax(this->profile.night_settings.gamma,       MIN_GAMMA, MAX_GAMMA);

    sanitize_minmax(this->profile.day_settings  .saturation,  MIN_SAT,   MAX_SAT);
    sanitize_minmax(this->profile.night_settings.saturation,  MIN_SAT,   MAX_SAT);

    sanitize_minmax(this->profile.day_settings  .hue,         MIN_HUE,   MAX_HUE);
    sanitize_minmax(this->profile.night_settings.hue,         MIN_HUE,   MAX_HUE);

    sanitize_minmax(this->profile.day_settings  .luminance,   MIN_LUMA,  MAX_LUMA);
    sanitize_minmax(this->profile.night_settings.luminance,   MIN_LUMA,  MAX_LUMA);

    sanitize_colorrange(this->profile.day_settings  .range);
    sanitize_colorrange(this->profile.night_settings.range);

    sanitize_minmax(this->profile.day_settings  .filter, ColorFilter_None, ColorFilter_Blue);
    sanitize_minmax(this->profile.night_settings.filter, ColorFilter_None, ColorFilter_Blue);
}

std::string Config::make() {
    auto format = []<typename ...Args>(const std::string_view &fmt, Args &&...args) -> std::string {
        std::string str(std::snprintf(nullptr, 0, fmt.data(), args...), 0);
        std::snprintf(str.data(), str.capacity(), fmt.data(), args...);
        return str;
    };

    auto format_profile = [&format](FizeauProfileId id) -> std::string {
        return format("profile%u", id + 1);
    };

    auto format_filter = [](ColorFilter f) -> std::string {
        switch (f) {
            case ColorFilter_Red:   return "red";
            case ColorFilter_Green: return "green";
            case ColorFilter_Blue:  return "blue";
            default:                return "none";
        }
    };

    auto format_time = [&format](Time t) -> std::string {
        return format("%02d:%02d", t.h, t.m);
    };

    auto format_range = [&format](ColorRange r) -> std::string {
        return format("%.2f-%.2f", r.lo, r.hi);
    };

    std::string str;

    str += std::string(this->has_active_override ? "" : COMMENT) + "active            = " + (this->active ? "true" : "false") + '\n';
    str += '\n';


    if (this->internal_profile < FizeauProfileId_Total)
        str += "handheld_profile  = " + format_profile(this->internal_profile) + '\n';
    else
        str += "handheld_profile  = " + format_profile(FizeauProfileId_Profile1) + '\n';
    if (this->external_profile < FizeauProfileId_Total)
        str += "docked_profile    = " + format_profile(this->external_profile) + '\n';
    else
        str += "docked_profile    = " + format_profile(FizeauProfileId_Profile2) + '\n';
    str += '\n';

    for (int id = FizeauProfileId_Profile1; id < FizeauProfileId_Total; ++id) {
        if (auto rc = this->open_profile(static_cast<FizeauProfileId>(id)); R_FAILED(rc))
            LOG("Failed to open profile %u: %#x\n", id, rc);

        this->sanitize_profile();

        str += "[profile" + std::to_string(this->cur_profile_id + 1) + "]\n";

        str += "dusk_begin        = " + format_time(this->profile.dusk_begin)                    + '\n';
        str += "dusk_end          = " + format_time(this->profile.dusk_end)                      + '\n';
        str += "dawn_begin        = " + format_time(this->profile.dawn_begin)                    + '\n';
        str += "dawn_end          = " + format_time(this->profile.dawn_end)                      + '\n';

        str += "temperature_day   = " + std::to_string(this->profile.day_settings.temperature)   + '\n';
        str += "temperature_night = " + std::to_string(this->profile.night_settings.temperature) + '\n';

        str += "filter_day        = " + format_filter(this->profile.day_settings.filter)         + '\n';
        str += "filter_night      = " + format_filter(this->profile.night_settings.filter)       + '\n';

        str += "gamma_day         = " + std::to_string(this->profile.day_settings.gamma)         + '\n';
        str += "gamma_night       = " + std::to_string(this->profile.night_settings.gamma)       + '\n';

        str += "saturation_day    = " + std::to_string(this->profile.day_settings.saturation)    + '\n';
        str += "saturation_night  = " + std::to_string(this->profile.night_settings.saturation)  + '\n';

        str += "hue_day           = " + std::to_string(this->profile.day_settings.hue)           + '\n';
        str += "hue_night         = " + std::to_string(this->profile.night_settings.hue)         + '\n';

        str += "luminance_day     = " + std::to_string(this->profile.day_settings.luminance)     + '\n';
        str += "luminance_night   = " + std::to_string(this->profile.night_settings.luminance)   + '\n';

        str += "range_day         = " + format_range(this->profile.day_settings.range)           + '\n';
        str += "range_night       = " + format_range(this->profile.night_settings.range)         + '\n';

        str += "dimming_timeout   = " + format_time({ this->profile.dimming_timeout.m, this->profile.dimming_timeout.s }) + '\n';

        str += '\n';
    }

    return str;
}

void Config::write() {
    auto str = this->make();

    auto loc = Config::find_config();
    FILE *fp = std::fopen(loc.data(), "w");
    FZ_SCOPEGUARD([&fp] { std::fclose(fp); });
    if (!fp)
        return;

    std::fwrite(str.c_str(), str.length(), 1, fp);
}

Result update(Config &config) {
    if (auto rc = fizeauGetIsActive(&config.active); R_FAILED(rc))
        return rc;

    if (auto rc = fizeauGetActiveProfileId(false, &config.internal_profile); R_FAILED(rc))
        return rc;

    if (auto rc = fizeauGetActiveProfileId(true,  &config.external_profile); R_FAILED(rc))
        return rc;

    if (auto rc = fizeauGetProfile(config.internal_profile, &config.profile); R_FAILED(rc))
        return rc;

    return 0;
}

Result Config::apply() {
    return fizeauSetProfile(this->cur_profile_id, &this->profile);
}

Result Config::reset() {
    this->profile.day_settings.temperature = DEFAULT_TEMP,     this->profile.night_settings.temperature = DEFAULT_TEMP;
    this->profile.day_settings.gamma       = DEFAULT_GAMMA,    this->profile.night_settings.gamma       = DEFAULT_GAMMA;
    this->profile.day_settings.hue         = DEFAULT_HUE,      this->profile.night_settings.hue         = DEFAULT_HUE;
    this->profile.day_settings.saturation  = DEFAULT_SAT,      this->profile.night_settings.saturation  = DEFAULT_SAT;
    this->profile.day_settings.luminance   = DEFAULT_LUMA,     this->profile.night_settings.luminance   = DEFAULT_LUMA;
    this->profile.day_settings.range       = DEFAULT_RANGE,    this->profile.night_settings.range       = DEFAULT_RANGE;
    this->profile.day_settings.filter      = ColorFilter_None, this->profile.night_settings.filter      = ColorFilter_None;
    return this->apply();
}

Result Config::open_profile(FizeauProfileId id) {
    if (auto rc = fizeauGetProfile(id, &this->profile); R_FAILED(rc))
        return rc;

    this->cur_profile_id = id;
    return 0;
}

} // namespace fz
