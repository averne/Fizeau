/**
 * Copyright (c) 2024 averne
 *
 * This file is part of Fizeau.
 *
 * Fizeau is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Fizeau is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fizeau.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>
#include <switch.h>

typedef uint32_t Temperature;
#define MIN_TEMP     1000u  // °K
#define D65_TEMP     6500u  // °K, D65 standard illuminant
#define MAX_TEMP     10000u // °K
#define DEFAULT_TEMP D65_TEMP

typedef float Saturation;
#define MIN_SAT      0.0f
#define MAX_SAT      2.0f
#define DEFAULT_SAT  1.0f

typedef float Hue;
#define MIN_HUE     -1.0f
#define MAX_HUE      1.0f
#define DEFAULT_HUE  0.0f

typedef enum {
    Component_None  = 0,
    Component_Red   = BIT(0),
    Component_Green = BIT(1),
    Component_Blue  = BIT(2),
    Component_All   = Component_Red | Component_Green | Component_Blue,
} Component;

typedef float Contrast;
#define MIN_CONTRAST     0.0f
#define MAX_CONTRAST     2.0f
#define DEFAULT_CONTRAST 1.0f

typedef float Gamma;
#define MIN_GAMMA     0.0f
#define MAX_GAMMA     5.0f
#define DEFAULT_GAMMA 2.4f

typedef float Luminance;
#define MIN_LUMA    -1.0f
#define MAX_LUMA     1.0f
#define DEFAULT_LUMA 0.0f

typedef struct {
    float lo, hi;
} ColorRange;
#define MIN_RANGE 0.0f
#define MAX_RANGE 1.0f
#define MIN_LIMITED_RANGE (16.0f  / 255.0f)
#define MAX_LIMITED_RANGE (235.0f / 255.0f)
#define DEFAULT_RANGE         { MIN_RANGE,         MAX_RANGE }
#define DEFAULT_LIMITED_RANGE { MIN_LIMITED_RANGE, MAX_LIMITED_RANGE}

typedef uint64_t Timestamp;
typedef struct {
    uint8_t h, m, s;
} __attribute__((aligned(4))) Time;

NX_CONSTEXPR Timestamp to_timestamp(Time t) {
    return (60 * 60 * t.h + 60 * t.m + t.s);
}

NX_CONSTEXPR Time from_timestamp(Timestamp s) {
    return (Time){ (uint8_t)(s / (60 * 60)), (uint8_t)(s / 60 % 60), (uint8_t)(s % 60) };
}

#ifdef __cplusplus

#include <compare>

constexpr auto operator ==(const Time &l, const Time &r) {
    return to_timestamp(l) == to_timestamp(r);
}

constexpr auto operator <=>(const Time &l, const Time &r) {
    return to_timestamp(l) <=> to_timestamp(r);
}

constexpr Time operator -(const Time &l, const Time &r) {
    return from_timestamp(to_timestamp(l) - to_timestamp(r));
}

constexpr auto operator ==(const ColorRange &l, const ColorRange &r) {
    return (l.hi == r.hi) && (l.lo == r.lo);
}

#endif // __cplusplus

#endif // _TYPES_H
