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

#pragma once

#include <stdio.h>


#define _FZ_CAT(x, y) x ## y
#define  FZ_CAT(x, y) _FZ_CAT(x, y)
#define _FZ_STR(x) #x
#define  FZ_STR(x) _FZ_STR(x)

#define FZ_ANONYMOUS FZ_CAT(var, __COUNTER__)

#define ASSERT_SIZE(x, sz)        static_assert(sizeof(x) == (sz), "Wrong size in " #x)
#define ASSERT_STANDARD_LAYOUT(x) static_assert(std::is_standard_layout_v<x>, #x " is not standard layout")

#if defined(DEBUG) && defined(TWILI)
#   include <twili.h>
#   ifdef SYSMODULE
extern TwiliPipe g_twlPipe;
#   endif
#endif

#ifdef DEBUG
#   if defined(SYSMODULE) && defined(TWILI)
#       define LOG(...) ({                                              \
            char buf[0x100];                                            \
            size_t len = snprintf(buf, sizeof(buf), __VA_ARGS__);       \
            twiliWritePipe(&g_twlPipe, buf, len);                       \
        })
#   elif (defined(APPLICATION) && defined(TWILI)) || defined(NXLINK)
#       define LOG(...) printf(__VA_ARGS__)
#   else
#      define LOG(...) ({})
#   endif
#else
#   define LOG(...) ({})
#endif

#include <ctype.h>

#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 16
#endif

__attribute__((unused)) static void hexdump(void *mem, unsigned int len) {
    unsigned int i, j;

    for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++) {
        if(i % HEXDUMP_COLS == 0)
            LOG("0x%06x: ", i);

        if(i < len)
            LOG("%02x ", 0xFF & ((char*)mem)[i]);
        else
            LOG("   ");

        if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
            for(j = i - (HEXDUMP_COLS - 1); j <= i; j++) {
                if(j >= len)
                    LOG(" ");
                else if(isprint((int)((char*)mem)[j]))
                    LOG("%c", 0xFF & ((char*)mem)[j]);
                else
                    LOG(".");
            }
            LOG("\n");
        }
    }
}

#ifdef __cplusplus

#define FZ_SCOPEGUARD(f) auto FZ_ANONYMOUS = ::fz::ScopeGuard(f)

namespace fz {

template <typename F>
struct ScopeGuard {
    [[nodiscard]] ScopeGuard(F &&f): f(std::move(f)) { }

    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard &operator =(const ScopeGuard &) = delete;

    ~ScopeGuard() {
        if (this->want_run)
            this->f();
    }

    void cancel() {
        this->want_run = false;
    }

    private:
        bool want_run = true;
        F f;
};

} // namespace fz

#endif // __cplusplus
