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

#include <stratosphere.hpp>
#include <common.hpp>

#include "layer.hpp"
#include "screen.hpp"

#include "service.hpp"

namespace fz {

static inline Layer *layer() {
    return Layer::instance;
}

ams::Result FizeauService::GetIsActive(ams::sf::Out<bool> is_active) {
    is_active.SetValue(layer()->get_is_active());
    return ams::ResultSuccess();
}

ams::Result FizeauService::SetIsActive(bool is_active) {
    layer()->set_is_active(is_active);
    return ams::ResultSuccess();
}

ams::Result FizeauService::GetDuskTime(ams::sf::Out<Time> time) {
    time.SetValue(layer()->get_dusk_time());
    return ams::ResultSuccess();
}

ams::Result FizeauService::SetDuskTime(Time time) {
    layer()->set_dusk_time(time);
    return ams::ResultSuccess();
}

ams::Result FizeauService::GetDawnTime(ams::sf::Out<Time> time) {
    time.SetValue(layer()->get_dawn_time());
    return ams::ResultSuccess();
}

ams::Result FizeauService::SetDawnTime(Time time) {
    layer()->set_dawn_time(time);
    return ams::ResultSuccess();
}

ams::Result FizeauService::GetColor(ams::sf::Out<std::uint16_t> color) {
    color.SetValue(layer()->get_color().rgba);
    return ams::ResultSuccess();
}

ams::Result FizeauService::SetColor(std::uint16_t color) {
    layer()->set_color(color);
    return ams::ResultSuccess();
}

ams::Result FizeauService::EasterEgg() {
    layer()->easter_egg();
    return ams::ResultSuccess();
}

} // namespace fz
