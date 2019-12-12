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

#include <cmath>
#include <cstdint>

#include "color.hpp"
#include "types.h"

namespace fz {

constexpr Temp min_temp = 1000.0f; // °K
constexpr Temp max_temp = 6500.0f; // °K

// TODO: Better values?
static inline rgba4444_t temp_to_col(Temp temp, std::uint8_t alpha) {
    temp = std::clamp(temp, min_temp, max_temp);
    float factor = std::exp(-(temp - min_temp) / 2000.0f);
    return {
        static_cast<std::uint8_t>(8.0f * factor),
        static_cast<std::uint8_t>(2.0f * factor),
        static_cast<std::uint8_t>(1.0f * factor),
        alpha,
    };
}

} // namespace fz
