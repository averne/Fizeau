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

#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <ini.h>
#include <stratosphere.hpp>
#include <common.hpp>

#include "fizeau.h"
#include "config.hpp"

namespace fz::cfg {

#define COMMENT ";"

#define MATCH(s1, s2)     (std::strcmp(s1, s2) == 0)
#define MATCH_ENTRY(s, n) (MATCH(section, (s)) && MATCH(name, (n)))

static int ini_handler(void *user, const char *section, const char *name, const char *value) {
    Config *config = static_cast<Config *>(user);

    auto profile_name_to_id = [](const std::string_view &str) -> FizeauProfileId {
        return static_cast<FizeauProfileId>(str.back() - '0' - 1);
    };

    auto parse_time = [](const std::string_view &str) -> Time {
        Time t = {};
        t.h = std::atoi(str.data());
        t.m = std::atoi(str.substr(str.find(':') + 1).data());
        return t;
    };

    auto parse_range = [](const std::string_view &str) -> ColorRange {
        ColorRange r = {};
        r.lo = std::atof(str.data());
        r.hi = std::atof(str.substr(str.find('-') + 1).data());
        return r;
    };

    if (MATCH_ENTRY("", "active")) {
        if (MATCH(value, "1") || strcasecmp(value, "true") == 0)
            config->active = true;
        else
            config->active = false;
        config->has_active_override = true;
    } else if (MATCH_ENTRY("", "handheld_profile")) {
        config->active_internal_profile = profile_name_to_id(value);
    } else if (MATCH_ENTRY("", "docked_profile")) {
        config->active_external_profile = profile_name_to_id(value);
    } else if (std::strcmp(section, "profile") > 0) {
        auto id = profile_name_to_id(section);
        if (config->cur_profile_id != id) {
            if (config->cur_profile.s.session)
                if (auto rc = apply(*config); R_FAILED(rc))
                    LOG("Failed to apply config: %#x\n", rc);
            if (auto rc = open_profile(*config, id); R_FAILED(rc))
                LOG("Failed to open profile: %#x\n", rc);
        }

        if (MATCH(name, "dusk_begin"))
            config->dusk_begin        = parse_time(value);
        else if (MATCH(name, "dusk_end"))
            config->dusk_end          = parse_time(value);
        else if (MATCH(name, "dawn_begin"))
            config->dawn_begin        = parse_time(value);
        else if (MATCH(name, "dawn_end"))
            config->dawn_end          = parse_time(value);
        else if (MATCH(name, "temperature_day"))
            config->temperature_day   = std::atoi(value);
        else if (MATCH(name, "temperature_night"))
            config->temperature_night = std::atoi(value);
        else if (MATCH(name, "brightness_day"))
            config->brightness_day    = std::atof(value);
        else if (MATCH(name, "brightness_night"))
            config->brightness_night  = std::atof(value);
        else if (MATCH(name, "gamma_day"))
            config->gamma_day         = std::atof(value);
        else if (MATCH(name, "gamma_night"))
            config->gamma_night       = std::atof(value);
        else if (MATCH(name, "luminance_day"))
            config->luminance_day     = std::atof(value);
        else if (MATCH(name, "luminance_night"))
            config->luminance_night   = std::atof(value);
        else if (MATCH(name, "range_day"))
            config->range_day         = parse_range(value);
        else if (MATCH(name, "range_night"))
            config->range_night       = parse_range(value);
        else
            return 0;
    } else {
        return 0;
    }
    return 1;
}

Config read(const std::string_view &path) {
    Config config{};
    ini_parse(path.data(), ini_handler, &config);
    if (config.cur_profile_id != FizeauProfileId_Invalid)
        if (auto rc = apply(config); R_FAILED(rc))
            LOG("Failed to apply config: %#x\n", rc);
    return config;
}

inline std::string make(Config &config) {
    std::stringstream stream;

    auto format_profile = [](FizeauProfileId id) -> std::string {
        std::array<char, 0x10> buf;
        std::snprintf(buf.data(), buf.size(), "profile%u", id + 1);
        return std::string(buf.data());
    };

    auto format_time = [](Time t) -> std::string {
        std::array<char, 0x10> buf;
        std::snprintf(buf.data(), buf.size(), "%02d:%02d", t.h, t.m);
        return std::string(buf.data());
    };

    auto format_range = [](ColorRange r) -> std::string {
        std::array<char, 0x10> buf;
        std::snprintf(buf.data(), buf.size(), "%.2f-%.2f", r.lo, r.hi);
        return std::string(buf.data());
    };

    stream << (config.has_active_override ? "" : COMMENT) << "active            = " << (config.active ? "true" : "false") << '\n';
    stream << '\n';

    stream << "handheld_profile  = " << format_profile(config.active_internal_profile) << '\n';
    stream << "docked_profile    = " << format_profile(config.active_external_profile) << '\n';
    stream << '\n';

    for (int id = FizeauProfileId_Profile1; id < FizeauProfileId_Total; ++id) {
        if (auto rc = open_profile(config, static_cast<FizeauProfileId>(id)); R_FAILED(rc))
            LOG("Failed to open profile %u: %#x\n", id, rc);
        if (auto rc = update(config); R_FAILED(rc))
            LOG("Failed to update profile: %#x\n", rc);

        stream << "[profile" << config.cur_profile_id + 1 << "]\n";

        stream << "dusk_begin        = " << format_time(config.dusk_begin)   << '\n';
        stream << "dusk_end          = " << format_time(config.dusk_end)     << '\n';
        stream << "dawn_begin        = " << format_time(config.dawn_begin)   << '\n';
        stream << "dawn_end          = " << format_time(config.dawn_end)     << '\n';

        stream << "temperature_day   = " << config.temperature_day           << '\n';
        stream << "temperature_night = " << config.temperature_night         << '\n';

        stream << "brightness_day    = " << config.brightness_day            << '\n';
        stream << "brightness_night  = " << config.brightness_night          << '\n';

        stream << "gamma_day         = " << config.gamma_day                 << '\n';
        stream << "gamma_night       = " << config.gamma_night               << '\n';

        stream << "luminance_day     = " << config.luminance_day             << '\n';
        stream << "luminance_night   = " << config.luminance_night           << '\n';

        stream << "range_day         = " << format_range(config.range_day)   << '\n';
        stream << "range_night       = " << format_range(config.range_night) << '\n';

        stream << '\n';
    }

    return stream.str();
}

void dump(const std::string &path,
Config &config) {
    auto str = make(config);
    FILE *fp = fopen(path.c_str(), "w");
    fwrite(str.c_str(), str.length(), 1, fp);
    fclose(fp);
}

Result update(Config &config) {
    R_TRY(fizeauGetIsActive(&config.active));
    R_TRY(fizeauGetActiveInternalProfileId(&config.active_internal_profile));
    R_TRY(fizeauGetActiveExternalProfileId(&config.active_external_profile));

    R_TRY(fizeauProfileGetDuskTime(&config.cur_profile, &config.dusk_begin, &config.dusk_end));
    R_TRY(fizeauProfileGetDawnTime(&config.cur_profile, &config.dawn_begin, &config.dawn_end));
    R_TRY(fizeauProfileGetCmuTemperature(&config.cur_profile, &config.temperature_day, &config.temperature_night));
    R_TRY(fizeauProfileGetCmuGamma(&config.cur_profile, &config.gamma_day, &config.gamma_night));
    R_TRY(fizeauProfileGetCmuLuminance(&config.cur_profile, &config.luminance_day, &config.luminance_night));
    R_TRY(fizeauProfileGetCmuColorRange(&config.cur_profile, &config.range_day, &config.range_night));
    R_TRY(fizeauProfileGetScreenBrightness(&config.cur_profile, &config.brightness_day, &config.brightness_night));
    return 0;
}

Result apply(cfg::Config &config) {
    R_TRY(fizeauProfileSetDuskTime(&config.cur_profile, config.dusk_begin, config.dusk_end));
    R_TRY(fizeauProfileSetDawnTime(&config.cur_profile, config.dawn_begin, config.dawn_end));
    R_TRY(fizeauProfileSetCmuTemperature(&config.cur_profile, config.temperature_day, config.temperature_night));
    R_TRY(fizeauProfileSetCmuGamma(&config.cur_profile, config.gamma_day, config.gamma_night));
    R_TRY(fizeauProfileSetCmuLuminance(&config.cur_profile, config.luminance_day, config.luminance_night));
    R_TRY(fizeauProfileSetCmuColorRange(&config.cur_profile, config.range_day, config.range_night));
    R_TRY(fizeauProfileSetScreenBrightness(&config.cur_profile, config.brightness_day, config.brightness_night));
    return 0;
}

Result reset(Config &config) {
    config.temperature_day = DEFAULT_TEMP,   config.temperature_night = DEFAULT_TEMP;
    config.gamma_day       = DEFAULT_GAMMA,  config.gamma_night       = DEFAULT_GAMMA;
    config.luminance_day   = DEFAULT_LUMA,   config.luminance_night   = DEFAULT_LUMA;
    config.range_day       = DEFAULT_RANGE,  config.range_night       = DEFAULT_RANGE;
    config.brightness_day  = MAX_BRIGHTNESS, config.brightness_night  = MAX_BRIGHTNESS;
    return apply(config);
}

Result open_profile(Config &config, FizeauProfileId id) {
    if (config.cur_profile.s.session != 0)
        fizeauProfileClose(&config.cur_profile);
    R_TRY(fizeauOpenProfile(&config.cur_profile, id));
    config.cur_profile_id = id;
    return update(config);
}

} // namespace fz::cfg
