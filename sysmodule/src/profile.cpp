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
#include <mutex>
#include <common.hpp>

#include "brightness.hpp"
#include "cmu.hpp"
#include "profile.hpp"

namespace fz {

using Man = ProfileManager;

Profile Profile::interpolate(float factor, bool from_day) {
    Profile p;
    if (from_day) {
        p.temperature_day   = std::lerp(this->temperature_day,   this->temperature_night, factor);
        p.gamma_day         = std::lerp(this->gamma_day,         this->gamma_night,       factor);
        p.luminance_day     = std::lerp(this->luminance_day,     this->luminance_night,   factor);
        p.brightness_day    = std::lerp(this->brightness_day,    this->brightness_night,  factor);
        p.range_day         = { std::lerp(this->range_day.lo,   this->range_night.lo, factor),
            std::lerp(this->range_day.hi,   this->range_night.hi, factor)};
    } else {
        p.temperature_night = std::lerp(this->temperature_night, this->temperature_day,   factor);
        p.gamma_night       = std::lerp(this->gamma_night,       this->gamma_day,         factor);
        p.luminance_night   = std::lerp(this->luminance_night,   this->luminance_day,     factor);
        p.brightness_night  = std::lerp(this->brightness_night,  this->brightness_day,    factor);
        p.range_night       = { std::lerp(this->range_night.lo, this->range_day.lo,   factor),
            std::lerp(this->range_night.hi, this->range_day.hi,   factor)};
    }
    p.dusk_begin = this->dusk_begin, p.dusk_end = this->dusk_end;
    p.dawn_begin = this->dawn_begin, p.dawn_end = this->dawn_end;
    return p;
}

void ProfileManager::transition_thread_func([[maybe_unused]] void *args) {
    while (!Man::stop.stop_requested()) {
        svcSleepThread(1e9l);
        if (!Man::is_active)
            continue;

        // Regularly reapply the settings, as they are overriden when waking from sleeping/dithering, etc
        Man::commit(false);
    }
}

ams::Result ProfileManager::initialize() {
    R_TRY(Man::thread.Initialize(Man::transition_thread_func, nullptr, 0x3f));
    return Man::thread.Start();
}

ams::Result ProfileManager::finalize() {
    Man::stop.request_stop();
    return Man::thread.Join();
}

bool ProfileManager::get_is_active() {
    return Man::is_active;
}

ams::Result ProfileManager::set_is_active(bool active) {
    Man::is_active = active;
    if (active) {
        R_TRY(Man::commit());
    } else {
        R_TRY(CmuManager::disable());
        R_TRY(BrightnessManager::disable());
    }
    return ams::ResultSuccess();
}

ams::Result ProfileManager::commit(bool force_brightness) {
    std::scoped_lock lk(Man::commit_mutex);

    auto apply_profile = [&force_brightness](Profile profile, bool internal) -> ams::Result {
        auto time = Clock::get_current_time();
        bool apply_brightness = true;

        if (Clock::is_in_interval(time, profile.dusk_begin, profile.dusk_end)) {
            if (!profile.is_transitionning) {
                BrightnessManager::get_brightness(profile.brightness_day);
                profile.is_transitionning = true;
            }
            float factor = static_cast<float>(to_timestamp(profile.dusk_end - time))
                / static_cast<float>(to_timestamp(profile.dusk_end - profile.dusk_begin));
            profile = profile.interpolate(factor, false);
        } else if (Clock::is_in_interval(time, profile.dawn_begin, profile.dawn_end)) {
            if (!profile.is_transitionning) {
                BrightnessManager::get_brightness(profile.brightness_night);
                profile.is_transitionning = true;
            }
            float factor = static_cast<float>(to_timestamp(profile.dawn_end - time))
                / static_cast<float>(to_timestamp(profile.dawn_end - profile.dawn_begin));
            profile = profile.interpolate(factor, true);
        } else {
            profile.is_transitionning = false;
            apply_brightness = force_brightness;
        }

        if (Clock::is_in_interval(profile.dawn_begin, profile.dusk_begin)) {
            if (internal)
                R_TRY(CmuManager::set_cmu_internal(profile.temperature_day, profile.gamma_day,
                    profile.luminance_day, profile.range_day));
            else
                R_TRY(CmuManager::set_cmu_external(profile.temperature_day, profile.gamma_day,
                    profile.luminance_day, profile.range_day));
            if (internal && apply_brightness)
                R_TRY(BrightnessManager::set_brightness(profile.brightness_day));
        } else {
            if (internal)
                R_TRY(CmuManager::set_cmu_internal(profile.temperature_night, profile.gamma_night,
                    profile.luminance_night, profile.range_night));
            else
                R_TRY(CmuManager::set_cmu_external(profile.temperature_night, profile.gamma_night,
                    profile.luminance_night, profile.range_night));
            if (internal && apply_brightness)
                R_TRY(BrightnessManager::set_brightness(profile.brightness_night));
        }
        return ams::ResultSuccess();
    };

    R_TRY(apply_profile(Man::get_active_internal_profile(), true));
    R_TRY(apply_profile(Man::get_active_external_profile(), false));
    return ams::ResultSuccess();
}

ams::Result ProfileManager::on_profile_updated(ProfileId id) {
    if (!Man::is_active || ((id != Man::active_internal_profile) && (id != Man::active_external_profile)))
        return ams::ResultSuccess();
    return Man::commit();
}

} // namespace fz
