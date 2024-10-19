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
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <concepts>

#include <switch.h>

#include <common.hpp>

namespace fz {

// Represents a fixed-point fractional number
template <bool Signed, std::size_t M, std::size_t N, typename Rep>
struct Q {
    constexpr static std::size_t Sign          = Signed;
    constexpr static std::size_t Integer       = M;
    constexpr static std::size_t Fractional    = N;

    constexpr static std::size_t NbIntegerBits = Integer + Fractional;
    constexpr static std::size_t NbBits        = Sign + NbIntegerBits;

    constexpr static std::size_t BitMask       = (1 << NbBits) - 1;

    using Underlying = Rep;

    constexpr Q() = default;

    template <typename T>
    constexpr Q(T n) requires std::integral<T>:
        rep(static_cast<Underlying>(n)) { }

    template <typename T>
    constexpr Q(T n) requires std::floating_point<T>:
        rep(static_cast<Underlying>(n * static_cast<T>(1 << Fractional))) { }

    template <typename T>
    constexpr operator T() const requires std::integral<T> {
        return this->rep;
    }

    template <typename T>
    constexpr operator T() const requires std::floating_point<T> {
        return (this->rep & (1 << NbIntegerBits) ? -1.0f : 1.0f) *
            static_cast<T>(this->rep & ((1 << NbIntegerBits) - 1)) / static_cast<T>(1 << Fractional);
    }

    private:
        Underlying rep = 0;
};

using QS18 = Q<true, 1, 8, std::int16_t>;
static_assert(static_cast<float>(QS18(0x100))          == 1.0f);
static_assert(static_cast<float>(QS18(-1.0f))          == -1.0f);
static_assert(std::bit_cast<std::uint16_t>(QS18(1.0))  == 0x100);
static_assert(std::bit_cast<std::uint16_t>(QS18(-1.0)) == 0xff00);

struct Cmu {
    __nv_in std::uint16_t enable;

    __nv_in QS18 krr, kgr, kbr,
                 krg, kgg, kbg,
                 krb, kgb, kbb;

    __nv_in std::array<std::uint16_t, 256> lut_1;
    __nv_in std::array<std::uint16_t, 960> lut_2;

    __nv_out std::uint16_t csc_modified;
    __nv_out std::uint16_t lut1_modified;
    __nv_out std::uint16_t lut2_modified;

    constexpr Cmu(bool enable = true, QS18 krr = 1.0, QS18 kgg = 1.0, QS18 kbb = 1.0):
        enable(enable), krr(krr), kgg(kgg), kbb(kbb) { }

    template <typename ...Args>
    inline void reset(Args &&...args) {
        new (this) Cmu(std::forward<Args>(args)...);
    }
};
ASSERT_SIZE(Cmu, 2458);

static inline Result nvioctlNvDisp_SetCmu(u32 fd, Cmu *cmu) {
    return nvIoctl(fd, _NV_IOWR(2, 14, Cmu), cmu);
}

// Defined in CEA-861-D table 11
enum RgbQuantRange {
    Default = 0,
    Limited = 1,
    Full    = 2,
};

// Unpacked standard AVI infoframe struct (HDMI v1.4b/2.0)
struct AviInfoframe {
    std::uint32_t csum;
    std::uint32_t scan;
    std::uint32_t bar_valid;
    std::uint32_t act_fmt_valid;
    std::uint32_t rgb_ycc;
    std::uint32_t act_format;
    std::uint32_t aspect_ratio;
    std::uint32_t colorimetry;
    std::uint32_t scaling;
    std::uint32_t rgb_quant;
    std::uint32_t ext_colorimetry;
    std::uint32_t it_content;
    std::uint32_t video_format;
    std::uint32_t pix_rep;
    std::uint32_t it_content_type;
    std::uint32_t ycc_quant;
    std::uint32_t top_bar_end_line_low_byte, top_bar_end_line_high_byte;
    std::uint32_t bot_bar_start_line_low_byte, bot_bar_start_line_high_byte;
    std::uint32_t left_bar_end_pixel_low_byte, left_bar_end_pixel_high_byte;
    std::uint32_t right_bar_start_pixel_low_byte, right_bar_start_pixel_high_byte;
};
ASSERT_SIZE(AviInfoframe, 96);

static inline Result nvioctlNvDisp_GetAviInfoframe(u32 fd, AviInfoframe *infoframe) {
    return nvIoctl(fd, _NV_IOR(2, 16, AviInfoframe), infoframe);
}

static inline Result nvioctlNvDisp_SetAviInfoframe(u32 fd, AviInfoframe *infoframe) {
    return nvIoctl(fd, _NV_IOW(2, 17, AviInfoframe), infoframe);
}

class DisplayController {
    public:
        using Csc  = std::array<std::uint16_t, 9>;
        using Lut1 = std::array<std::uint16_t, 256>;
        using Lut2 = std::array<std::uint8_t,  960>;

        // Lut1 and Lut2 ignored since they cannot be read back directly from registers
        struct CmuShadow {
            Csc csc;
        };

    public:
        Result initialize() {
            return nvOpen(&this->disp0_fd, "/dev/nvdisp-disp0") || nvOpen(&this->disp1_fd, "/dev/nvdisp-disp1");
        }

        Result finalize() const {
            return nvClose(this->disp0_fd) || nvClose(this->disp1_fd);
        }

        Result disable(bool external) const;
        Result apply_color_profile(bool external, FizeauSettings &settings,
            Component components, Component filter, CmuShadow &shadow) const;
        Result set_hdmi_color_range(bool external, ColorRange range) const;

    private:
        std::uint32_t disp0_fd, disp1_fd;
};

} // namespace fz
