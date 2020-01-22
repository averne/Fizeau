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
#include <common.hpp>

#include "config.hpp"

namespace fz {

#define COMMENT ";"

#define MATCH(s1, s2)     (std::strcmp(s1, s2) == 0)
#define MATCH_ENTRY(s, n) (MATCH(section, (s)) && MATCH(name, (n)))

static int ini_handler(void *user, const char *section, const char *name, const char *value) {
    Config *config = reinterpret_cast<Config *>(user);

    if (MATCH_ENTRY("", "active")) {
        config->has_active_override = true;
        if (MATCH(value, "1") || strcasecmp(value, "true") == 0)
            config->active = true;
        else
            config->active = false;
    } else if (MATCH_ENTRY("", "dusk") && (strlen(value) > 3)) {
        config->dusk.hour   = std::atoi(value);
        config->dusk.minute = std::atoi(value + 3);
    } else if (MATCH_ENTRY("", "dawn") && (strlen(value) > 3)) {
        config->dawn.hour   = std::atoi(value);
        config->dawn.minute = std::atoi(value + 3);
    } else if (MATCH_ENTRY("", "temperature")) {
        config->temp = std::atof(value);
    } else if (MATCH_ENTRY("", "brightness")) {
        config->brightness = std::atof(value);
    } else if (MATCH(section, "color")) {
        if (MATCH(name, "red"))
            config->color.r = std::atoi(value);
        else if (MATCH(name, "green"))
            config->color.g = std::atoi(value);
        else if (MATCH(name, "blue"))
            config->color.b = std::atoi(value);
        else if (MATCH(name, "alpha"))
            config->color.a = std::atoi(value);
    } else {
        return 0;
    }
    return 1;
}

Config read_config(const std::string &path) {
    Config config{};
    ini_parse(path.c_str(), ini_handler, &config);
    return config;
}

inline std::string make_config(const Config &config) {
    std::stringstream stream;

    stream << COMMENT " If present, overrides dusk/dawn hours\n";
    stream << (config.has_active_override ? "" : COMMENT) << "active         = " << (config.active ? "true" : "false") << '\n';
    stream << '\n';

    stream << COMMENT " Values have to be in hh::mm format\n";
    stream << "dusk           = " << std::setfill('0') << std::setw(2) << std::to_string(config.dusk.hour)
        << ':' << std::setw(2) << std::to_string(config.dusk.minute) << '\n';
    stream << "dawn           = " << std::setfill('0') << std::setw(2) << std::to_string(config.dawn.hour)
        << ':' << std::setw(2) << std::to_string(config.dawn.minute)   << '\n';
    stream << '\n';

    stream << COMMENT " Value has to be >1000.0°K, and <6500.0°K\n";
    stream << "temperature    = " << std::fixed << std::setprecision(1) << config.temp << '\n';
    stream << '\n';

    stream << COMMENT " Controls brightness of the display\n";
    stream << COMMENT " Value has to be >0.0, and <1.0\n";
    stream << "brightness     = " << std::fixed << std::setprecision(1) << config.brightness << '\n';
    stream << '\n';

    stream << COMMENT " If present, overrides temperature\n";
    stream << COMMENT " Values have to be <16\n";
    stream << "[color]\n";
    stream << "red            = " << std::to_string(config.color.r) << '\n';
    stream << "green          = " << std::to_string(config.color.g) << '\n';
    stream << "blue           = " << std::to_string(config.color.b) << '\n';
    stream << "alpha          = " << std::to_string(config.color.a) << '\n';
    stream << '\n';

    return stream.str();
}

void dump_config(const std::string &path, const Config &config) {
    auto str = make_config(config);
    FILE *fp = fopen(path.c_str(), "w");
    fwrite(str.c_str(), str.length(), 1, fp);
    fclose(fp);
}

} // namespace fz
