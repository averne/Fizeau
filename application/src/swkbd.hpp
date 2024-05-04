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

#include <string>
#include <utility>
#include <type_traits>
#include <switch.h>
#include <imgui.h>

namespace fz::swkbd {

std::string show(const std::string &in, SwkbdArgCommon *args = nullptr);
bool input_f(float *val, float min, float max, const std::string &fmt, bool negative = false);
bool input_int(int *val, int min, int max);

template <typename T, typename ...Args>
static bool handle(const char *label, T *val, T min, T max, Args &&...args) {
    if (ImGui::TempInputIsActive(ImGui::GetCurrentWindow()->GetID(label))) {
        if constexpr (std::is_integral_v<T>)
            return input_int(reinterpret_cast<int *>(val), min, max, std::forward<Args>(args)...);
        else if constexpr (std::is_same_v<T, float>)
            return input_f(val, min, max, std::forward<Args>(args)...);
    }
    return false;
}

} // namespace fz::swkbd
