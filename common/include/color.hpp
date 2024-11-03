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

#include <cstdint>
#include <array>
#include <tuple>

#include "types.h"

namespace fz {

using ColorMatrix = std::array<float, 9>;

constexpr ColorMatrix dot(const ColorMatrix &r, const ColorMatrix &l) {
    return {
        r[0]*l[0] + r[1]*l[3] + r[2]*l[6], r[0]*l[1] + r[1]*l[4] + r[2]*l[7], r[0]*l[2] + r[1]*l[5] + r[2]*l[8],
        r[3]*l[0] + r[4]*l[3] + r[5]*l[6], r[3]*l[1] + r[4]*l[4] + r[5]*l[7], r[3]*l[2] + r[4]*l[5] + r[5]*l[8],
        r[6]*l[0] + r[7]*l[3] + r[8]*l[6], r[6]*l[1] + r[7]*l[4] + r[8]*l[7], r[6]*l[2] + r[7]*l[5] + r[8]*l[8],
    };
}

ColorMatrix filter_matrix(Component filter);
std::tuple<float, float, float> whitepoint(Temperature temperature);
ColorMatrix hue_matrix(Hue hue);
ColorMatrix saturation_matrix(Saturation sat);

float degamma(float x, Gamma gamma);
float regamma(float x, Gamma gamma);

void gamma_ramp(float (*func)(float, Gamma), std::uint16_t *array, std::size_t size, Gamma gamma, std::size_t nb_bits, float lo, float hi, float off);

[[maybe_unused]]
static inline void degamma_ramp(std::uint16_t *array, std::size_t size, Gamma gamma, std::size_t nb_bits, float lo = 0.0f, float hi = 1.0f, float off = 0.0f) {
    return gamma_ramp(degamma, array, size, gamma, nb_bits, lo, hi, off);
}

[[maybe_unused]]
static inline void regamma_ramp(std::uint16_t *array, std::size_t size, Gamma gamma, std::size_t nb_bits, float lo = 0.0f, float hi = 1.0f, float off = 0.0f) {
    return gamma_ramp(regamma, array, size, gamma, nb_bits, lo, hi, off);
}

void apply_luma(std::uint16_t *array, std::size_t size, std::size_t nb_bits, Luminance luma);
void apply_range(std::uint16_t *array, std::size_t size, std::size_t nb_bits, float lo, float hi);

} // namespace fz
