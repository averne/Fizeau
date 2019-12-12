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

#include <cstdio>
#include <algorithm>
#include <type_traits>
#include <initializer_list>
#include <switch.h>
#include <stratosphere.hpp>

#undef CONCATENATE // Defined in libstrat

#define _STRINGIFY(x)        #x
#define  STRINGIFY(x)        _STRINGIFY(x)
#define _CONCATENATE(x1, x2) x1##x2
#define  CONCATENATE(x1, x2) _CONCATENATE(x1, x2)

#define TRY(x, cb) ({                      \
    if (Result _rc = (x); R_FAILED(_rc))   \
        ({cb;});                           \
})
#define TRY_GOTO(x, l)    TRY(x, goto l)
#define TRY_RETURN(x)     TRY(x, return _rc)
#define TRY_RETURNV(x, v) TRY(x, return v)
#define TRY_FATAL(x)      TRY(x, fatalThrow(_rc))

#define SERV_INIT(s, ...) TRY_FATAL(CONCATENATE(s, Initialize)(__VA_ARGS__))
#define SERV_EXIT(s, ...) CONCATENATE(s, Exit)(__VA_ARGS__)

#define ASSERT_SIZE(x, sz)        static_assert(sizeof(x) == (sz), "Wrong size in " STRINGIFY(x))
#define ASSERT_STANDARD_LAYOUT(x) static_assert(std::is_standard_layout_v<x>, STRINGIFY(x) " is not standard layout")

#ifdef DEBUG
#   define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#   define LOG(fmt, ...) ({})
#endif

namespace fz::utils {

template <typename T>
struct Vec2X {
    union {
        struct {
            T x = 0, y = 0;
        };
        struct {
            T w, h;
        };
        struct {
            T u, v;
        };
    };

    constexpr inline Vec2X() = default;
    constexpr inline Vec2X(T x, T y = 0): x(x), y(y) { }
    constexpr inline Vec2X(std::initializer_list<T> list) {
        std::copy_n(list.begin(), 2, &this->x);
    }

    constexpr inline T &operator[](std::size_t idx) {
        return *(reinterpret_cast<T *>(this) + idx);
    }
};

using Vec2i  = Vec2X<std::int32_t>;
using Vec2u  = Vec2X<std::uint32_t>;
using Vec2il = Vec2X<std::int64_t>;
using Vec2ul = Vec2X<std::uint64_t>;
using Vec2f  = Vec2X<float>;
using Vec2d  = Vec2X<double>;
using Vec2   = Vec2i;

template <typename F>
static inline auto do_with_sm_session(F f) {
    smInitialize();
    ON_SCOPE_EXIT{ smExit(); };
    return f();
}

} // namespace fz
