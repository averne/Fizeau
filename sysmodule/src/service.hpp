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

namespace fz::ipc {

class FizeauService final: public ams::sf::IServiceObject {
    public:
        enum class CommandId {
            GetIsActive     = 0,
            SetIsActive     = 1,
            GetDuskTime     = 2,
            SetDuskTime     = 3,
            GetDawnTime     = 4,
            SetDawnTime     = 5,
            GetColor        = 6,
            SetColor        = 7,
            EasterEgg       = 8,
        };

    protected:
        ams::Result GetIsActive(ams::sf::Out<bool> is_active);
        ams::Result SetIsActive(bool is_active);
        ams::Result GetDuskTime(ams::sf::Out<Time> time);
        ams::Result SetDuskTime(Time time);
        ams::Result GetDawnTime(ams::sf::Out<Time> time);
        ams::Result SetDawnTime(Time time);
        ams::Result GetColor(ams::sf::Out<std::uint16_t> color);
        ams::Result SetColor(std::uint16_t color);
        ams::Result EasterEgg();

    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(GetIsActive),
            MAKE_SERVICE_COMMAND_META(SetIsActive),
            MAKE_SERVICE_COMMAND_META(GetDuskTime),
            MAKE_SERVICE_COMMAND_META(SetDuskTime),
            MAKE_SERVICE_COMMAND_META(GetDawnTime),
            MAKE_SERVICE_COMMAND_META(SetDawnTime),
            MAKE_SERVICE_COMMAND_META(GetColor),
            MAKE_SERVICE_COMMAND_META(SetColor),
            MAKE_SERVICE_COMMAND_META(EasterEgg),
        };
};

} // namespace fz::ipc

