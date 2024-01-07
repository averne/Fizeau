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
#include <thread>
#include <common.hpp>
#include <omm.h>

#include "brightness.hpp"
#include "disp1_regs.hpp"
#include "nvdisp.hpp"
#include "profile.hpp"

using namespace std::chrono_literals;

namespace fz {

namespace {

constexpr std::uint32_t ins_evt_id = 0;
std::uint64_t g_disp_va_base;

} // namespace

using Man = ProfileManager;

Profile Profile::interpolate(float factor, bool from_day) {
    Profile p;
    if (from_day) {
        p.temperature_day   = std::lerp(this->temperature_day,   this->temperature_night, factor);
        p.gamma_day         = std::lerp(this->gamma_day,         this->gamma_night,       factor);
        p.sat_day           = std::lerp(this->sat_day,           this->sat_night,         factor);
        p.luminance_day     = std::lerp(this->luminance_day,     this->luminance_night,   factor);
        p.brightness_day    = std::lerp(this->brightness_day,    this->brightness_night,  factor);
        p.range_day         = { std::lerp(this->range_day.lo,   this->range_night.lo, factor),
            std::lerp(this->range_day.hi,   this->range_night.hi, factor) };
    } else {
        p.temperature_night = std::lerp(this->temperature_night, this->temperature_day,   factor);
        p.gamma_night       = std::lerp(this->gamma_night,       this->gamma_day,         factor);
        p.sat_night         = std::lerp(this->sat_day,           this->sat_night,         factor);
        p.luminance_night   = std::lerp(this->luminance_night,   this->luminance_day,     factor);
        p.brightness_night  = std::lerp(this->brightness_night,  this->brightness_day,    factor);
        p.range_night       = { std::lerp(this->range_night.lo, this->range_day.lo,   factor),
            std::lerp(this->range_night.hi, this->range_day.hi,   factor) };
    }
    p.dusk_begin = this->dusk_begin, p.dusk_end     = this->dusk_end;
    p.dawn_begin = this->dawn_begin, p.dawn_end     = this->dawn_end;
    p.filter_day = this->filter_day, p.filter_night = this->filter_night;
    return p;
}

void ProfileManager::transition_thread_func([[maybe_unused]] void *args) {
    while (!Man::stop.stop_requested()) {
        std::this_thread::sleep_for(50ms);

        std::scoped_lock lk(Man::mmio_mutex);
        if (!Man::is_active || !Man::should_poll_mmio)
            continue;

        bool need_apply = false, is_handheld = Man::operation_mode == AppletOperationMode_Handheld;

        // Poll DISPLAY_A in handheld mode, DISPLAY_B in docked mode
        std::uint64_t iobase = g_disp_va_base + (is_handheld ? 0 : 0x40000);

        {
            std::scoped_lock lk(Man::commit_mutex);

            auto &csc = is_handheld ? Man::saved_internal_csc : Man::saved_external_csc;

            for (std::size_t i = 0; i < csc.size(); ++i) {
                if (csc[i] != READ(iobase + DC_COM_CMU_CSC_KRR + i * sizeof(std::uint32_t))) {
                    need_apply = true;
                    break;
                }
            }
        }

        auto &profile = is_handheld ? Man::get_active_internal_profile() : Man::get_active_external_profile();
        auto time = Clock::get_current_time();
        if (!need_apply && (profile.is_transitionning || Clock::is_in_interval(time, profile.dusk_begin, profile.dusk_end) ||
                Clock::is_in_interval(time, profile.dawn_begin, profile.dawn_end))) {
            // First wait a second to avoid calculating/applying the coefficients too much
            std::this_thread::sleep_for(1s);
            need_apply = true;
        }

        if (!need_apply) {
            std::uint64_t timeout = armNsToTicks(to_timestamp(profile.dimming_timeout) * 1e9),
                delta = armGetSystemTick() - Man::activity_tick;

            if (!Man::is_dimming && delta > timeout)
                need_apply = true, Man::is_dimming = true;
            else if (Man::is_dimming && delta <= timeout)
                need_apply = true, Man::is_dimming = false;
        }

        if (need_apply)
            Man::commit(false);
    }
}

void ProfileManager::event_monitor_thread_func([[maybe_unused]] void *args) {
    while (!Man::stop.stop_requested()) {
        int idx;
        auto rc = waitMulti(&idx, UINT64_MAX,
            waiterForEvent(&Man::psc_module.event),
            waiterForEvent(&Man::operation_mode_event),
            waiterForEvent(&Man::activity_event));
        if (R_FAILED(rc))
            continue;

        std::scoped_lock lk(Man::mmio_mutex);

        switch (idx) {
            case 0:
                PscPmState state;
                if (R_SUCCEEDED(pscPmModuleGetRequest(&Man::psc_module, &state, nullptr)))
                    Man::should_poll_mmio = state == PscPmState_Awake;

                pscPmModuleAcknowledge(&Man::psc_module, state);
                break;
            case 1:
                ommGetOperationMode(&Man::operation_mode);
                break;
            case 2:
                insrGetLastTick(ins_evt_id, &Man::activity_tick);
                break;
            default:
                break;
        }
    }
}

ams::Result ProfileManager::initialize() {
    ams::sm::DoWithSession([]() {
        if (auto rc = splInitialize(); R_FAILED(rc))
            Man::is_lite = false;
        ON_SCOPE_EXIT { splExit(); };

        ams::spl::HardwareType type;
        if (auto rc = splGetConfig(SplConfigItem_HardwareType, reinterpret_cast<u64 *>(&type)); R_FAILED(rc))
            Man::is_lite = false;

        Man::is_lite = type == ams::spl::HardwareType::Hoag;

        R_ABORT_UNLESS(ommInitialize());
    });

    std::uint64_t size;
    R_TRY(svcQueryIoMapping(reinterpret_cast<std::uint64_t *>(&g_disp_va_base), &size, DISP_IO_BASE, DISP_IO_SIZE));

    std::array deps = {u32(PscPmModuleId_Display)};
    R_TRY(pscmGetPmModule(&Man::psc_module, PscPmModuleId(125), deps.data(), deps.size(), true));

    R_TRY(ommGetOperationMode(&Man::operation_mode));
    R_TRY(ommGetOperationModeChangeEvent(&Man::operation_mode_event, false));

    R_TRY(insrGetReadableEvent(ins_evt_id, &Man::activity_event));
    R_TRY(insrGetLastTick(ins_evt_id, &Man::activity_tick));

    R_TRY(Man::transition_thread.Initialize(Man::transition_thread_func, nullptr, 0x3f));
    R_TRY(Man::transition_thread.Start());

    R_TRY(Man::event_monitor_thread.Initialize(Man::event_monitor_thread_func, nullptr, 0x3f));
    R_TRY(Man::event_monitor_thread.Start());

    return 0;
}

ams::Result ProfileManager::finalize() {
    ommExit();
    pscPmModuleFinalize(&Man::psc_module);
    pscPmModuleClose(&Man::psc_module);
    eventClose(&Man::psc_module.event);
    eventClose(&Man::operation_mode_event);

    Man::stop.request_stop();
    R_TRY(Man::transition_thread.Join());
    R_TRY(Man::event_monitor_thread.Join());

    return 0;
}

bool ProfileManager::get_is_active() {
    return Man::is_active;
}

ams::Result ProfileManager::set_is_active(bool active) {
    Man::is_active = active;
    if (active) {
        R_TRY(Man::commit());
    } else {
        R_TRY(DispControlManager::disable(true));
        if (!Man::is_lite)
            R_TRY(DispControlManager::disable(false));
        if (hosversionBefore(14, 0, 0))
            R_TRY(BrightnessManager::disable());
    }
    return ams::ResultSuccess();
}

ams::Result ProfileManager::commit(bool force_brightness) {
    std::scoped_lock lk(Man::commit_mutex);

    auto apply_profile = [&force_brightness](Profile profile, bool dim, bool internal) -> ams::Result {
        auto time = Clock::get_current_time();
        bool apply_brightness = true;
        Luminance luminance_day = profile.luminance_day, luminance_night = profile.luminance_night;

        if (Clock::is_in_interval(time, profile.dusk_begin, profile.dusk_end)) {
            if (!profile.is_transitionning) {
                if (hosversionBefore(14, 0, 0))
                    R_TRY(BrightnessManager::get_brightness(profile.brightness_day));
                profile.is_transitionning = true;
            }
            float factor = static_cast<float>(to_timestamp(profile.dusk_end - time))
                / static_cast<float>(to_timestamp(profile.dusk_end - profile.dusk_begin));
            profile = profile.interpolate(factor, false);
        } else if (Clock::is_in_interval(time, profile.dawn_begin, profile.dawn_end)) {
            if (!profile.is_transitionning) {
                if (hosversionBefore(14, 0, 0))
                    R_TRY(BrightnessManager::get_brightness(profile.brightness_night));
                profile.is_transitionning = true;
            }
            float factor = static_cast<float>(to_timestamp(profile.dawn_end - time))
                / static_cast<float>(to_timestamp(profile.dawn_end - profile.dawn_begin));
            profile = profile.interpolate(factor, true);
        } else {
            profile.is_transitionning = false;
            apply_brightness = force_brightness;
        }

        if (dim) {
            luminance_day = luminance_night =
                internal ? dimmed_luma_internal : dimmed_luma_external;

            apply_brightness = false;
            if (internal && hosversionBefore(14, 0, 0))
                R_TRY(BrightnessManager::enable_dimming());
        } else if (internal && hosversionBefore(14, 0, 0)) {
            bool is_dimming;
            R_TRY(BrightnessManager::is_dimming(is_dimming));

            if (is_dimming)
                R_TRY(BrightnessManager::disable_dimming());
        }

        if (Clock::is_in_interval(profile.dawn_begin, profile.dusk_begin)) {
            if (internal) {
                R_TRY(DispControlManager::set_cmu_internal(profile.temperature_day, profile.filter_day,
                    profile.gamma_day, profile.sat_day, luminance_day, profile.range_day,
                    Man::saved_internal_csc));
            } else {
                R_TRY(DispControlManager::set_cmu_external(profile.temperature_day, profile.filter_day,
                    profile.gamma_day, profile.sat_day, luminance_day, profile.range_day,
                    Man::saved_external_csc));
                R_TRY(DispControlManager::set_hdmi_color_range(profile.range_day));
            }
            if (internal && apply_brightness && hosversionBefore(14, 0, 0))
                R_TRY(BrightnessManager::set_brightness(profile.brightness_day));
        } else {
            if (internal) {
                R_TRY(DispControlManager::set_cmu_internal(profile.temperature_night, profile.filter_night,
                    profile.gamma_night, profile.sat_night, luminance_night, profile.range_night,
                    Man::saved_internal_csc));
            } else {
                R_TRY(DispControlManager::set_cmu_external(profile.temperature_night, profile.filter_night,
                    profile.gamma_night, profile.sat_night, luminance_night, profile.range_night,
                    Man::saved_external_csc));
                R_TRY(DispControlManager::set_hdmi_color_range(profile.range_night));
            }
            if (internal && apply_brightness && hosversionBefore(14, 0, 0))
                R_TRY(BrightnessManager::set_brightness(profile.brightness_night));
        }
        return ams::ResultSuccess();
    };

    bool should_dim_internal = false, should_dim_external = false;
    if (hosversionAtLeast(9, 0, 0)) {
        auto timeout = armTicksToNs(armGetSystemTick() - Man::activity_tick) / static_cast<u64>(1e9);

        auto should_dim = [](auto profile, auto timeout) {
            auto ts = to_timestamp(profile.dimming_timeout);
            return ts && (timeout >= ts);
        };

        should_dim_internal = should_dim(Man::get_active_internal_profile(), timeout);
        should_dim_external = should_dim(Man::get_active_external_profile(), timeout);
    }

    R_TRY(apply_profile(Man::get_active_internal_profile(), should_dim_internal, true));
    if (!Man::is_lite)
        R_TRY(apply_profile(Man::get_active_external_profile(), should_dim_external, false));

    return ams::ResultSuccess();
}

ams::Result ProfileManager::on_profile_updated(ProfileId id) {
    if (!Man::is_active || ((id != Man::active_internal_profile) && (id != Man::active_external_profile)))
        return ams::ResultSuccess();
    return Man::commit();
}

} // namespace fz
