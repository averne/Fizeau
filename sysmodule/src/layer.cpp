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

#include <array>

#include "layer.hpp"

namespace fz {

static inline constexpr Timestamp to_timestamp(const Time &time) {
    return time.h * 60 * 60 + time.m * 60 + time.s;
}

void Layer::activate() {
    std::scoped_lock lk(this->screen_mutex);
    if (this->is_active)
        return;
    this->is_active = true;
    set_color(this->color);
}

void Layer::deactivate() {
    std::scoped_lock lk(this->screen_mutex);
    if (!this->is_active)
        return;
    auto c = this->color;
    set_color(transparent);
    this->color = c;
    this->is_active = false;
}

void Layer::update(const Time &time) {
    std::scoped_lock lk(this->screen_mutex);
    if (this->is_active_overriden)
        return;
    auto dusk = to_timestamp(this->dusk_time), dawn = to_timestamp(this->dawn_time), ts = to_timestamp(time);
    if (dusk > dawn) {
        if ((dusk > ts) && (ts >= dawn))
            deactivate();
        else
            activate();
    } else {
        if ((dawn >= ts) && (ts > dusk))
            activate();
        else
            deactivate();
    }
}

void Layer::set_color(const rgba4444_t &col) {
    std::scoped_lock lk(this->screen_mutex);
    this->color = col;
    if (!this->is_active)
        return;
    this->screen->dequeue();
    this->screen->fill(col);
    this->screen->flush();
}

void Layer::easter_egg() {
    std::scoped_lock lk(this->screen_mutex);

    // Force active
    this->is_active = true;

    auto *buf = this->screen->dequeue();
    auto size = this->screen->get_framebuffer_size() / Screen::fb_bpp;

    static constexpr std::array<rgba4444_t, 8> colors = {
        rgba4444_t{6,  0, 10, 0},
        rgba4444_t{0,  0,  6, 0},
        rgba4444_t{0,  6, 10, 0},
        rgba4444_t{0,  8,  1, 0},
        rgba4444_t{8,  8,  2, 0},
        rgba4444_t{12, 6,  0, 0},
        rgba4444_t{12, 0,  0, 0},
        rgba4444_t{12, 1,  6, 0},
    };

    // Here we don't need to care about framebuffer tiling
    const auto *cur_color = colors.begin();
    for (std::uint32_t i = 0; i < size; ++i) {
        buf[i] = *cur_color;
        if ((i + 1) % (size / colors.size()) == 0)
            ++cur_color;
    }
    this->screen->flush();
}

} // namespace fz
