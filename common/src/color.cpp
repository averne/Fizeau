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

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <tuple>
#include <numbers>

#include <common.hpp>

namespace fz {

ColorMatrix filter_matrix(ColorFilter filter) {
    ColorMatrix arr = {};

    static constexpr std::array luma_components = {
        0.2126f, 0.7152f, 0.0722f,
    };

    if (filter == ColorFilter_None) {
        arr[0] = arr[4] = arr[8] = 1.0f;
        return arr;
    }

    std::size_t offset = 0;
    switch (filter) {
        default:
        case ColorFilter_Red:   offset = 0; break;
        case ColorFilter_Green: offset = 3; break;
        case ColorFilter_Blue:  offset = 6; break;
    }

    for (std::size_t i = 0; i < 3; ++i)
        arr[offset + i] = luma_components[i];

    return arr;
}

std::tuple<float, float, float> whitepoint(Temperature temperature) {
    if (temperature == D65_TEMP)
        return { 1.0f, 1.0f, 1.0f }; // Fast path

    float temp = static_cast<float>(temperature) / 100.0f, red, green, blue;

    if (temp <= 66.0f)
        red = 255.0f;
    else
        red = 329.698727446f * std::pow(temp - 60.0f, -0.1332047592f);

    if (temp <= 66.0f)
        green = 99.4708025861f * std::log(temp) - 161.1195681661f;
    else
        green = 288.1221695283f * std::pow(temp - 60.0f, -0.0755148492f);

    if (temp >= 66.0f)
        blue = 255.0f;
    else if (temp <= 19.0f)
        blue = 0.0f;
    else
        blue = 138.5177312231f * std::log(temp - 10.0f) - 305.0447927307f;

    return {
        std::clamp(red,   0.0f, 255.0f) / 255.0f,
        std::clamp(green, 0.0f, 255.0f) / 255.0f,
        std::clamp(blue,  0.0f, 255.0f) / 255.0f,
    };
}

ColorMatrix hue_matrix(Hue hue) {
    if (hue == DEFAULT_HUE) // Fast path
        return { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };

    auto angle = hue * std::numbers::pi_v<float>;
    auto c1 = (1.0f + 2.0f * std::cos(angle)) / 3.0f,
        c2 = (1.0f - std::cos(angle)) / 3.0f,
        c3 = std::sin(angle) / std::numbers::sqrt3_v<float>;
    return {
        c1,      c2 - c3, c2 + c3,
        c2 + c3, c1,      c2 - c3,
        c2 - c3, c2 + c3, c1,
    };
}

ColorMatrix saturation_matrix(Saturation sat) {
    if (sat == DEFAULT_SAT) // Fast path
        return { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };

    return {
        (1.0f - sat) * 0.2126f + sat, (1.0f - sat) * 0.7152f      , (1.0f - sat) * 0.0722f,
        (1.0f - sat) * 0.2126f      , (1.0f - sat) * 0.7152f + sat, (1.0f - sat) * 0.0722f,
        (1.0f - sat) * 0.2126f      , (1.0f - sat) * 0.7152f      , (1.0f - sat) * 0.0722f + sat,
    };
}

float degamma(float x, Gamma gamma) {
    if (x <= 0.040045f) // x * pow((0.040045 + 0.055) / (1.0 + 0.055), gamma) / 0.040045;
        return x * 24.972f * std::pow(0.090f, gamma);
    return std::pow((x + 0.055f) / (1.0f + 0.055f), gamma);
}

float regamma(float x, Gamma gamma) {
    if (x <= 0.0031308f) // ((1.0 + 0.055) * std::pow(0.0031308, 1.0f / gamma) - 0.055) / 0.0031308
        return x * (1.055f * std::pow(0.0031308f, (1.0f - gamma) / gamma) - 17.567f);
    return (1.0f + 0.055f) * std::pow(x, 1.0f / gamma) - 0.055f;

}

void gamma_ramp(float (*func)(float, Gamma), std::uint16_t *array, std::size_t size, Gamma gamma, std::size_t nb_bits, float lo, float hi) {
    float step = (hi - lo) / (size - 1), cur = lo;
    std::uint16_t shift = (1 << nb_bits) - 1, mask = (1 << (nb_bits + 1)) - 1;

    for (std::size_t i = 0; i < size; ++i, cur += step)
        array[i] = static_cast<std::uint16_t>(std::round(func(cur, gamma) * shift)) & mask;
}

void apply_luma(std::uint16_t *array, std::size_t size, Luminance luma) {
    luma = std::clamp(luma, MIN_LUMA, MAX_LUMA) + MAX_LUMA;
    if (luma == 1.0f)
        return; // No effect, fast path

    auto max = array[size - 1];
    for (std::size_t i = 0; i < size; ++i)
        array[i] = std::clamp(static_cast<std::uint16_t>(std::round(array[i] * luma)),
            static_cast<std::uint16_t>(0), static_cast<std::uint16_t>(max));
}

void apply_range(std::uint16_t *array, std::size_t size, float lo, float hi) {
    lo = std::clamp(lo, 0.0f, hi), hi = std::clamp(hi, lo, 1.0f);
    if ((lo == 0.0f) && (hi == 1.0f))
        return; // No effect, fast path

    auto max = array[size - 1];
    for (std::size_t i = 0; i < size; ++i)
        array[i] = static_cast<std::uint16_t>(std::round(array[i] * (hi - lo) + lo * max));
}

} // namespace fz
