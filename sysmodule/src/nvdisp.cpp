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

#include <cmath>
#include <algorithm>
#include <common.hpp>

#include "color.hpp"

#include "nvdisp.hpp"

namespace fz {

namespace {

Cmu calculate_cmu(FizeauSettings &settings, Component components, Component filter) {
    Cmu cmu;

    // Calculate initial coefficients
    auto coeffs = filter_matrix(filter);

    // Apply temperature color correction
    ColorMatrix m = {};
    std::tie(m[0], m[4], m[8]) = whitepoint(settings.temperature);
    m[0] = degamma(m[0], 2.4f), m[4] = degamma(m[4], 2.4f), m[8] = degamma(m[8], 2.4f);
    coeffs = dot(coeffs, m);

    // Apply contrast multiplier
    auto c = contrast_slant(settings.contrast);
    m[0] = m[4] = m[8] = c;
    coeffs = dot(coeffs, m);

    // Apply saturation
    coeffs = dot(coeffs, saturation_matrix(settings.saturation));

    // Apply hue rotation
    coeffs = dot(coeffs, hue_matrix(settings.hue));

    // Copy calculated coefficients to the cmu matrix if they are enabled
    if (components & Component_Red)
        std::copy_n(coeffs.begin() + 0, 3, &cmu.krr);
    if (components & Component_Green)
        std::copy_n(coeffs.begin() + 3, 3, &cmu.krg);
    if (components & Component_Blue)
        std::copy_n(coeffs.begin() + 6, 3, &cmu.krb);

    // Calculate gamma ramps, with contrast offset
    float off = (1.0f - c) / 2.0f;
    degamma_ramp(cmu.lut_1.data(), cmu.lut_1.size(), DEFAULT_GAMMA, 12);                                // Set the LUT1 with a fixed gamma corresponding to the incoming data
    regamma_ramp(cmu.lut_2.data(), 512, settings.gamma, 8, 0.0f, 0.125f, off);                          // Set the first part of LUT2 (more precision in darker components)
    regamma_ramp(cmu.lut_2.data() + 512, cmu.lut_2.size() - 512, settings.gamma, 8, 0.125f, 1.0f, off); // Set the second part of LUT2 (less precision in brighter components)

    // Apply luminance
    apply_luma(cmu.lut_2.data(), cmu.lut_2.size(), 8, settings.luminance);

    // Apply color range
    apply_range(cmu.lut_2.data(), cmu.lut_2.size(), 8,
        settings.range.lo, std::min(settings.range.hi, cmu.lut_2.back() / 255.0f)); // Adjust max for luma

    return cmu;
}

} // namespace

Result DisplayController::disable(bool external) const {
    Cmu cmu(false);

    if (auto rc = nvioctlNvDisp_SetCmu(!external ? this->disp0_fd : this->disp1_fd, &cmu))
        return rc;

    if (external)
        return 0;

    AviInfoframe infoframe;
    if (auto rc = nvioctlNvDisp_GetAviInfoframe(this->disp1_fd, &infoframe); R_FAILED(rc))
        return rc;

    infoframe.rgb_quant = RgbQuantRange::Default;
    if (auto rc = nvioctlNvDisp_SetAviInfoframe(this->disp1_fd, &infoframe); R_FAILED(rc))
        return rc;

    return 0;
}

Result DisplayController::apply_color_profile(bool external, FizeauSettings &settings,
        Component components, Component filter, CmuShadow &shadow) const {
    Cmu cmu = calculate_cmu(settings, components, filter);

    if (auto rc = nvioctlNvDisp_SetCmu(!external ? this->disp0_fd : this->disp1_fd, &cmu); R_FAILED(rc))
        return rc;

    // Save cmu shadow, to be used for change detection
    std::transform(&cmu.krr, &cmu.krr + 9, shadow.csc.begin(),
        [](QS18 c) -> std::uint16_t { return static_cast<Csc::value_type>(c) & QS18::BitMask; });

    return 0;
}

Result DisplayController::set_hdmi_color_range(bool external, ColorRange range) const {
    if (external)
        return 0;

    auto is_limited = [](const ColorRange &range) {
        return (range.lo >= MIN_LIMITED_RANGE) && (range.hi <= MAX_LIMITED_RANGE);
    };

    AviInfoframe infoframe;
    if (auto rc = nvioctlNvDisp_GetAviInfoframe(this->disp1_fd, &infoframe); R_FAILED(rc))
        return rc;

    infoframe.rgb_quant = is_limited(range) ? RgbQuantRange::Limited : RgbQuantRange::Full;

    if (auto rc = nvioctlNvDisp_SetAviInfoframe(this->disp1_fd, &infoframe); R_FAILED(rc))
        return rc;

    return 0;
}

} // namespace fz
