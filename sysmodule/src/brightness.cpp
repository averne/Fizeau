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

#include <switch.h>

#include "brightness.hpp"

namespace fz {

ams::Result BrightnessManager::initialize() {
    return BrightnessManager::get_brightness(BrightnessManager::saved_brightness);
}

ams::Result BrightnessManager::finalize() {
    return ams::ResultSuccess();
}

ams::Result BrightnessManager::get_brightness(Brightness &brightness) {
    return lblGetCurrentBrightnessSetting(&brightness);
}

ams::Result BrightnessManager::set_brightness(Brightness brightness) {
    R_TRY(BrightnessManager::get_brightness(BrightnessManager::saved_brightness));
    return lblSetCurrentBrightnessSetting(brightness);
}

ams::Result BrightnessManager::disable() {
    return lblSetCurrentBrightnessSetting(BrightnessManager::saved_brightness);
}

} // namespace fz
