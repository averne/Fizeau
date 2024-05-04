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

#include <cstdio>
#include <algorithm>

#include "swkbd.hpp"

namespace fz::swkbd {

namespace {

std::string fmt_float(float f, const std::string &fmt) {
    std::string str(std::snprintf(nullptr, 0, fmt.c_str(), f) + 1, 0);
    std::snprintf(str.data(), str.capacity(), fmt.c_str(), f);
    return str;
}

} // namespace

std::string show(const std::string &in, SwkbdArgCommon *args) {
    std::string out(0x10, 0);

    SwkbdConfig kbd;
    if (R_FAILED(swkbdCreate(&kbd, 0)))
        return {};
    if (args)
        kbd.arg.arg.arg = *args;
    swkbdConfigSetType(&kbd, SwkbdType_NumPad);
    swkbdConfigSetBlurBackground(&kbd, true);
    swkbdConfigSetInitialText(&kbd, in.c_str());
    swkbdConfigSetInitialCursorPos(&kbd, 1);
    if (R_FAILED(swkbdShow(&kbd, out.data(), out.capacity())))
        return {};
    return out;
}

bool input_f(float *val, float min, float max, const std::string &fmt, bool negative) {
    SwkbdArgCommon args{};
    args.leftButtonText  = (negative) ? u'-' : u'.';
    args.rightButtonText = u'.';
    auto str = show(fmt_float(*val, fmt), &args);
    float f = std::atof(str.c_str());
    if (!str.empty()) {
        *val = std::clamp(f, min, max);
        return true;
    }
    return false;
}

bool input_int(int *val, int min, int max) {
    SwkbdArgCommon args{};
    auto str = show(std::to_string(*val), &args);
    int i = std::atoi(str.c_str());
    if (!str.empty()) {
        *val = std::clamp(i, min, max);
        return true;
    }
    return false;
}

} // namespace fz::swkbd
