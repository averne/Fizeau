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

#include <atomic>
#include <stratosphere.hpp>
#include <common.hpp>

namespace fz {

class BrightnessManager {
    public:
        static ams::Result initialize();
        static ams::Result finalize();

        static ams::Result get_brightness(Brightness &brightness);
        static ams::Result set_brightness(Brightness brightness);
        static ams::Result disable();

    private:
        static inline Brightness saved_brightness;
};

} // namespace fz
