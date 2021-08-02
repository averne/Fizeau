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

#include <cmath>
#include <algorithm>
#include <common.hpp>

#include "color.hpp"

#include "nvdisp.hpp"

namespace fz {

using Man = DispControlManager;

ams::Result Man::set_cmu(std::uint32_t fd,
        Temperature temp, ColorFilter filter, Gamma gamma, Saturation sat, Luminance luma, ColorRange range) {
    auto &cmu = Man::cmu;
    cmu.reset();

    // Calculate initial coefficients
    auto coeffs = filter_matrix(filter);

    // Apply temperature color correction
    auto [krr, kgg, kbb] = whitepoint(temp);
    krr = degamma(krr, 2.4f), kgg = degamma(kgg, 2.4f), kbb = degamma(kbb, 2.4f);
    coeffs[0] *= krr, coeffs[4] *= kgg, coeffs[8] *= kbb;

    coeffs = dot(coeffs, saturation_matrix(sat));

    std::transform(coeffs.begin(), coeffs.end(), &cmu.krr, [](float c) -> QS18 { return c; });

    // Calculate gamma ramps
    degamma_ramp(cmu.lut_1.data(), cmu.lut_1.size(), DEFAULT_GAMMA, 12);                  // Set the LUT1 with a fixed gamma corresponding to the incoming data
    regamma_ramp(cmu.lut_2.data(), 512, gamma, 8, 0.0f, 0.125f);                          // Set the first part of LUT2 (more precision in darker components)
    regamma_ramp(cmu.lut_2.data() + 512, cmu.lut_2.size() - 512, gamma, 8, 0.125f, 1.0f); // Set the second part of LUT2 (less precision in brighter components)

    // Apply luminance
    apply_luma(cmu.lut_2.data(), cmu.lut_2.size(), luma);

    // Apply color range
    apply_range(cmu.lut_2.data(), cmu.lut_2.size(), range.lo, std::min(range.hi, cmu.lut_2.back() / 255.0f)); // Adjust max for luma

    return nvioctlNvDisp_SetCmu(fd, cmu);
}

ams::Result Man::set_hdmi_color_range(ColorRange range, bool disable) {
    auto is_limited = [](const ColorRange &range) {
        return (range.lo >= MIN_LIMITED_RANGE) && (range.hi <= MAX_LIMITED_RANGE);
    };

    R_TRY(nvioctlNvDisp_GetAviInfoframe(Man::disp1_fd, Man::infoframe));
    Man::infoframe.rgb_quant = static_cast<std::uint32_t>(disable ? RgbQuantRange::Default :
        (is_limited(range) ? RgbQuantRange::Limited : RgbQuantRange::Full));

    // FIXME: Setting the infoframe leads to flickering in the applet client when docked
    R_TRY(nvioctlNvDisp_SetAviInfoframe(Man::disp1_fd, Man::infoframe));
    return ams::ResultSuccess();
}

ams::Result Man::disable() {
    Man::cmu.reset(false);
    R_TRY(nvioctlNvDisp_SetCmu(Man::disp0_fd, Man::cmu));
    R_TRY(nvioctlNvDisp_SetCmu(Man::disp1_fd, Man::cmu));
    R_TRY(set_hdmi_color_range(DEFAULT_LIMITED_RANGE, true));
    return ams::ResultSuccess();
}

} // namespace fz
