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
#include <tuple>

#include "types.h"

namespace fz {

std::array<float, 9> filter_matrix(ColorFilter filter);
std::tuple<float, float, float> whitepoint(Temperature temperature);

float degamma(float x, Gamma gamma);
float regamma(float x, Gamma gamma);

void gamma_ramp(float (*func)(float, Gamma), std::uint16_t *array, std::size_t size, Gamma gamma, std::size_t nb_bits, float lo, float hi);

[[maybe_unused]]
static inline void degamma_ramp(std::uint16_t *array, std::size_t size, Gamma gamma, std::size_t nb_bits, float lo = 0.0f, float hi = 1.0f) {
    return gamma_ramp(degamma, array, size, gamma, nb_bits, lo, hi);
}

[[maybe_unused]]
static inline void regamma_ramp(std::uint16_t *array, std::size_t size, Gamma gamma, std::size_t nb_bits, float lo = 0.0f, float hi = 1.0f) {
    return gamma_ramp(regamma, array, size, gamma, nb_bits, lo, hi);
}

void apply_luma(std::uint16_t *array, std::size_t size, Luminance luma);
void apply_range(std::uint16_t *array, std::size_t size, float lo, float hi);

} // namespace fz
