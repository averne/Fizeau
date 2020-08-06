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
#include <algorithm>
#include <array>
#include <atomic>
#include <concepts>
#include <switch.h>
#include <common.hpp>

namespace fz {

// Represents a fixed-point fractionel number
template <std::size_t M, std::size_t N, typename U = std::uint16_t>
struct Q {
    constexpr static std::size_t Integer    = M;
    constexpr static std::size_t Fractional = N;
    constexpr static std::size_t NbBits     = Integer + Fractional;

    using Underlying = U;

    constexpr Q() = default;

    template <typename T>
    constexpr Q(T n) requires std::integral<T>:
        rep(static_cast<Underlying>(n)) { }

    template <typename T>
    constexpr Q(T n) requires std::floating_point<T>:
        rep(static_cast<Underlying>(n * static_cast<T>(1 << Fractional)) & ((1 << NbBits) - 1)) { }

    template <typename T>
    constexpr operator T() const requires std::integral<T> {
        return this->rep;
    }

    template <typename T>
    constexpr operator T() const requires std::floating_point<T> {
        return static_cast<T>(this->rep) / static_cast<T>(1 << Fractional);
    }

    private:
        Underlying rep = 0;
};

using QS18 = Q<1, 8>;
ASSERT_SIZE(QS18, 2);
static_assert(static_cast<float>(QS18(0x100))       == 1.0f);
static_assert(static_cast<std::uint16_t>(QS18(1.0)) == 0x100);

struct Cmu {
    __nv_in std::uint16_t enable;

    __nv_in QS18 krr, kgr, kbr,
                 krg, kgg, kbg,
                 krb, kgb, kbb;

    __nv_in std::array<std::uint16_t, 256> lut_1;
    __nv_in std::array<std::uint16_t, 960> lut_2;

    __nv_out std::uint16_t enabled;
    __nv_out std::array<std::uint8_t, 4> unk;

    constexpr Cmu(bool enable = true, QS18 krr = 1.0, QS18 kgg = 1.0, QS18 kbb = 1.0):
        enable(enable), krr(krr), kgg(kgg), kbb(kbb) { }

    template <typename ...Args>
    inline void reset(Args &&...args) {
        new (this) Cmu(std::forward<Args>(args)...);
    }
};
ASSERT_SIZE(Cmu, 2458);

static inline Result nvioctlNvDisp_SetCmu(u32 fd, Cmu &cmu) {
    return nvIoctl(fd, _NV_IOWR(2, 14, Cmu), &cmu);
}

class CmuManager {
    public:
        static ams::Result initialize() {
            return nvOpen(&CmuManager::disp0_fd, "/dev/nvdisp-disp0") | nvOpen(&CmuManager::disp1_fd, "/dev/nvdisp-disp1");
        }

        static ams::Result finalize() {
            return nvClose(CmuManager::disp0_fd) | nvClose(CmuManager::disp1_fd);
        }

        static ams::Result set_cmu(std::uint32_t fd, Temperature temp, Gamma gamma, Luminance luma, ColorRange range);

        static inline ams::Result set_cmu_internal(Temperature temp, Gamma gamma, Luminance luma, ColorRange range) {
            return CmuManager::set_cmu(CmuManager::disp0_fd, temp, gamma, luma, range);
        }

        static inline ams::Result set_cmu_external(Temperature temp, Gamma gamma, Luminance luma, ColorRange range) {
            return CmuManager::set_cmu(CmuManager::disp1_fd, temp, gamma, luma, range);
        }

        static ams::Result disable();

    private:
        static inline Cmu cmu;
        static inline std::uint32_t disp0_fd;
        static inline std::uint32_t disp1_fd;
};

} // namespace fz
