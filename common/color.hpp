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
#include <array>

#include "utils.hpp"
#include "types.h"

namespace fz {

struct rgba4444_t {
    static constexpr inline std::uint16_t member_min = 0;
    static constexpr inline std::uint16_t member_max = (1 << 4) - 1;

    union {
        struct {
            std::uint16_t r: 4, g: 4, b: 4, a: 4;
        };
        std::uint16_t rgba;
    };

    constexpr inline rgba4444_t(): rgba(0) { }
    constexpr inline rgba4444_t(std::uint16_t raw): rgba(raw) { }
    constexpr inline rgba4444_t(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a): r(r), g(g), b(b), a(a) { }

    template <typename T>
    inline rgba4444_t(const std::array<T, 4> &arr) {
        this->r = static_cast<decltype(this->r)>(arr[0]);
        this->g = static_cast<decltype(this->g)>(arr[1]);
        this->b = static_cast<decltype(this->b)>(arr[2]);
        this->a = static_cast<decltype(this->a)>(arr[3]);
    }

    constexpr inline operator std::uint16_t() const {
        return this->rgba;
    }

    constexpr inline bool operator ==(const rgba4444_t &other) {
        return this->rgba == other.rgba;
    }
    constexpr inline bool operator !=(const rgba4444_t &other) {
        return !(*this == other);
    }
};
ASSERT_SIZE(rgba4444_t, 2);
ASSERT_STANDARD_LAYOUT(rgba4444_t);

static inline constexpr rgba4444_t black       = {rgba4444_t::member_min, rgba4444_t::member_min,
    rgba4444_t::member_min, rgba4444_t::member_max};
static inline constexpr rgba4444_t white       = {rgba4444_t::member_max, rgba4444_t::member_max,
    rgba4444_t::member_max, rgba4444_t::member_max};
static inline constexpr rgba4444_t transparent = {rgba4444_t::member_min, rgba4444_t::member_min,
    rgba4444_t::member_min, rgba4444_t::member_min};

} // namespace fz

