/**
 * Copyright (C) 2020 averne
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
#define MIN_TEMP     1000u // °K
#define MAX_TEMP     6500u // °K, D65 standard illuminant
#define DEFAULT_TEMP MAX_TEMP

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

typedef float Brightness;
#define MIN_BRIGHTNESS 0.0f
#define MAX_BRIGHTNESS 1.0f

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

constexpr auto operator <=>(const Time &l, const Time &r) {
    return to_timestamp(l) <=> to_timestamp(r);
}

constexpr Time operator -(const Time &l, const Time &r) {
    return from_timestamp(to_timestamp(l) - to_timestamp(r));
}

#endif // __cplusplus

#endif // _TYPES_H
