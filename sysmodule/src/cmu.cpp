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

#include "cmu.hpp"

namespace fz {

ams::Result CmuManager::set_cmu(std::uint32_t fd, Temperature temp, Gamma gamma, Luminance luma, ColorRange range) {
    auto &cmu = CmuManager::cmu;
    cmu.reset();

    // Calculate color correction coefficients
    auto [krr, kgg, kbb] = whitepoint(temp);
    krr = degamma(krr, 2.4f), kgg = degamma(kgg, 2.4f), kbb = degamma(kbb, 2.4f);
    cmu.krr = krr, cmu.kgg = kgg, cmu.kbb = kbb;

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

ams::Result CmuManager::disable() {
    CmuManager::cmu.reset(false);
    R_TRY(nvioctlNvDisp_SetCmu(CmuManager::disp0_fd, CmuManager::cmu));
    R_TRY(nvioctlNvDisp_SetCmu(CmuManager::disp1_fd, CmuManager::cmu));
    return ams::ResultSuccess();
}

} // namespace fz
