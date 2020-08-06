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

#include <cstdint>
#include <array>
#include <common.hpp>

namespace fz {

enum class ProfileId: std::uint32_t {
    Profile1,
    Profile2,
    Profile3,
    Profile4,
    Total,
    Invalid = 0xffff,
};

struct Profile {
    Temperature temperature_day = DEFAULT_TEMP,  temperature_night = DEFAULT_TEMP;
    Gamma       gamma_day       = DEFAULT_GAMMA, gamma_night       = DEFAULT_GAMMA;
    Luminance   luminance_day   = DEFAULT_LUMA,  luminance_night   = DEFAULT_LUMA;
    ColorRange  range_day       = DEFAULT_RANGE, range_night       = DEFAULT_RANGE;
    Brightness  brightness_day  = 1.0f,          brightness_night  = 1.0f;

    Time dusk_begin = {21, 00, 00}, dusk_end = {21, 30, 00};
    Time dawn_begin = {07, 00, 00}, dawn_end = {07, 30, 00};

    bool is_transitionning = false;

    Profile interpolate(float factor, bool from_day);
};

class ProfileManager {
    public:
        static ams::Result initialize();
        static ams::Result finalize();

        static ams::Result commit(bool force_apply_brightness = true);
        static ams::Result on_profile_updated(ProfileId id);

        static bool get_is_active();
        static ams::Result set_is_active(bool active);

        static Profile &get_profile(ProfileId id) {
            return ProfileManager::profiles[static_cast<std::size_t>(id)];
        }

        static Profile &get_active_internal_profile() {
            return get_profile(ProfileManager::active_internal_profile);
        }

        static Profile &get_active_external_profile() {
            return get_profile(ProfileManager::active_external_profile);
        }

        static ProfileId get_active_internal_profile_id() {
            return ProfileManager::active_internal_profile;
        }

        static void set_active_internal_profile_id(ProfileId id) {
            ProfileManager::active_internal_profile = id;
        }

        static ProfileId get_active_external_profile_id() {
            return ProfileManager::active_external_profile;
        }

        static void set_active_external_profile_id(ProfileId id) {
            ProfileManager::active_external_profile = id;
        }

    private:
        static void transition_thread_func(void *args);

    private:
        static inline bool is_active = false;

        static inline ams::os::StaticThread<2 * ams::os::MemoryPageSize> thread;
        static inline std::stop_source stop;

        static inline ProfileId active_internal_profile = ProfileId::Profile1, active_external_profile = ProfileId::Profile2;
        static inline std::array<Profile, static_cast<std::size_t>(ProfileId::Total)> profiles;

        static inline ams::os::Mutex commit_mutex;
};

} // namespace fz
