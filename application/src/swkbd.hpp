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
#include <string>
#include <utility>
#include <switch.h>
#include <imgui.hpp>
#include <common.hpp>

namespace fz {

static inline std::string fmt_float(float f) {
    std::string str(std::snprintf(nullptr, 0, "%.1f", f) + 1, 0);
    std::snprintf(str.data(), str.capacity(), "%.1f", f);
    return str;
}

static std::string swkbd_show(const std::string &in, SwkbdArgCommon *args = nullptr) {
    std::string out(0x10, 0);

    SwkbdConfig kbd;
    TRY_RETURNV(swkbdCreate(&kbd, 0), {});
    if (args)
        kbd.arg.arg.arg = *args;
    swkbdConfigSetType(&kbd, SwkbdType_NumPad);
    swkbdConfigSetBlurBackground(&kbd, true);
    swkbdConfigSetInitialText(&kbd, in.c_str());
    swkbdConfigSetInitialCursorPos(&kbd, 1);
    TRY_RETURNV(swkbdShow(&kbd, out.data(), out.capacity()), {});
    return out;
}

static bool swkbd_input_f(float *val, float min, float max) {
    SwkbdArgCommon args{};
    args.leftButtonText = u'.';
    args.rightButtonText = u'.';
    auto str = swkbd_show(fmt_float(*val), &args);
    float f = std::atof(str.c_str());
    if (!str.empty() && (min <= f) && (f <= max)) {
        *val = f;
        return true;
    }
    return false;
}

static bool swkbd_input_u8(std::uint8_t *val, std::uint8_t min, std::uint8_t max) {
    SwkbdArgCommon args{};
    auto str = swkbd_show(std::to_string(*val), &args);
    std::uint8_t i = std::atoi(str.c_str());
    if (!str.empty() && (min <= i) && (i <= max)) {
        *val = i;
        return true;
    }
    return false;
}

static bool swkbd_input_u16(std::uint16_t *val, std::uint16_t min, std::uint16_t max) {
    SwkbdArgCommon args{};
    auto str = swkbd_show(std::to_string(*val), &args);
    std::uint16_t i = std::atoi(str.c_str());
    if (!str.empty() && (min <= i) && (i <= max)) {
        *val = i;
        return true;
    }
    return false;
}

template <typename F, typename ...Args>
static inline bool handle_swkbd(const char *label, F f, Args ...args) {
    if (ImGui::TempInputTextIsActive(ImGui::GetCurrentWindow()->GetID(label)))
        return f(args...);
    return false;
}

} // namespace fz
