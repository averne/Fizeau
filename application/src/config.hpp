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

#include <string>
#include <common.hpp>

namespace fz {

constexpr char config_path[] = "/switch/Fizeau/config.ini";

struct Config {
    bool active = false;
    Time dusk{}, dawn{};
    Temp temp = min_temp;
    rgba4444_t color = transparent;
    float brightness = 0.0f;

    bool has_active_override = false;
};

Config read_config(const std::string &path);
std::string make_config(const Config &config);
void dump_config(const std::string &path, const Config &config);

} // namespace fz
