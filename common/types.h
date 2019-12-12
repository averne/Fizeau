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

typedef float Temp;
typedef uint64_t Timestamp;
typedef union {
    struct {
        uint8_t hour, minute, second;
    };
    struct {
        uint8_t h, m, s;
    };
} Time;

#ifdef __cplusplus

inline constexpr bool operator ==(Time l, const Time &r) {
    return (l.h == r.h) && (l.m == r.m) && (l.s == r.s);
}

inline constexpr bool operator !=(Time l, const Time &r) {
    return !(l == r);
}

inline constexpr bool operator >(Time l, const Time &r) {
    if (l.h > r.h)
        return true;
    if (l.m > r.m)
        return true;
    if (l.s > r.s)
        return true;
    return false;
}

#endif // __cplusplus

#endif // _TYPES_H
