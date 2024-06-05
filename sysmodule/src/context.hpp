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

#include <common.hpp>

#include "nvdisp.hpp"

namespace fz {

enum FizeauProfileState: std::uint32_t {
    Day,
    Night,
};

struct Context {
    bool is_lite = false, is_active = false;

    FizeauProfileId internal_profile = FizeauProfileId_Invalid,
        external_profile = FizeauProfileId_Invalid;

    std::array<FizeauProfile, FizeauProfileId_Total> profiles = {
        FizeauProfile{
            .day_settings   = Config::default_settings,
            .night_settings = Config::default_settings,
        },
        FizeauProfile{
            .day_settings   = Config::default_settings,
            .night_settings = Config::default_settings,
        },
        FizeauProfile{
            .day_settings   = Config::default_settings,
            .night_settings = Config::default_settings,
        },
        FizeauProfile{
            .day_settings   = Config::default_settings,
            .night_settings = Config::default_settings,
        },
    };

    std::array<FizeauProfileState, FizeauProfileId_Total> profile_states = {};

    DisplayController::Csc saved_internal_csc = {},
        saved_external_csc = {};
};

} // namespace fz
