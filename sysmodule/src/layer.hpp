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

#include <mutex>
#include <memory>
#include <stratosphere.hpp>
#include <common.hpp>

#include "screen.hpp"

namespace fz {

class Layer {
    public:
        static inline Layer *instance = nullptr;

        Layer() {
            this->instance = this;
            this->set_color(this->color);
        }

        ~Layer() {
            this->deactivate();
            this->instance = nullptr;
        }

        void activate();
        void deactivate();
        void update(const Time &time);

        inline const bool get_is_active() {
            std::scoped_lock lk(this->screen_mutex);
            return this->is_active;
        }

        inline void set_is_active(bool active) {
            std::scoped_lock lk(this->screen_mutex);
            this->is_active_overriden = true;
            if (this->is_active == active)
                return;
            active ? this->activate() : this->deactivate();
        }

        inline const Time get_dusk_time() {
            std::scoped_lock lk(this->screen_mutex);
            return this->dusk_time;
        }

        inline const void set_dusk_time(Time dusk_time) {
            std::scoped_lock lk(this->screen_mutex);
            this->is_active_overriden = false;
            if (this->dusk_time == dusk_time)
                return;
            this->dusk_time = dusk_time;
            this->update(get_time());
        }

        inline const Time get_dawn_time() {
            std::scoped_lock lk(this->screen_mutex);
            return this->dawn_time;
        }

        inline const void set_dawn_time(Time dawn_time) {
            std::scoped_lock lk(this->screen_mutex);
            this->is_active_overriden = false;
            if (this->dawn_time == dawn_time)
                return;
            this->dawn_time = dawn_time;
            this->update(get_time());
        }

        inline const rgba4444_t &get_color() {
            std::scoped_lock lk(this->screen_mutex);
            return this->color;
        }

        void set_color(const rgba4444_t &col);

        inline float get_cur_brightness() const {
            float brightness = 0.0f;
            lblGetCurrentBrightnessSetting(&brightness);
            return brightness;
        }

        inline void set_cur_brightness(float brightness) const {
            lblSetCurrentBrightnessSetting(brightness);
        }

        inline float get_brightness() {
            std::scoped_lock lk(this->screen_mutex);
            return this->brightness;
        }

        inline void set_brightness(float brightness) {
            std::scoped_lock lk(this->screen_mutex);
            this->brightness = brightness;
            if (this->is_active)
                this->set_cur_brightness(brightness);
        }

        void easter_egg();

    private:
        Screen screen;

        ams::os::Mutex screen_mutex = ams::os::Mutex(true);

        bool is_active = false, is_active_overriden = true;
        rgba4444_t color = transparent;
        Time dusk_time{}, dawn_time{};
        float prev_brightness = 0.0f, brightness = 0.0f;
};

} // namespace fz
