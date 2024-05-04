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

#include <cmath>
#include <chrono>
#include <switch.h>

#include <common.hpp>
#include <omm.h>

#include "disp1_regs.hpp"
#include "nvdisp.hpp"

#include "profile.hpp"

using namespace std::chrono_literals;

namespace fz {

namespace {

constexpr std::uint32_t ins_evt_id = 0;

FizeauSettings interpolate_profile(FizeauProfile &in, float factor, bool from_day) {
    FizeauSettings out,
        &from =  from_day ? in.day_settings : in.night_settings,
        &to   = !from_day ? in.day_settings : in.night_settings;

    out = {
        .temperature     = static_cast<Temperature>(std::lerp(from.temperature, to.temperature, factor)),
        .gamma           = std::lerp(from.gamma,      to.gamma,      factor),
        .saturation      = std::lerp(from.saturation, to.saturation, factor),
        .luminance       = std::lerp(from.luminance,  to.luminance,  factor),
        .range           = {
                           std::lerp(from.range.lo,   to.range.lo,   factor),
                           std::lerp(from.range.hi,   to.range.hi,   factor),
        },
        .filter          = from.filter,
    };

    return out;
}

} // namespace

void ProfileManager::transition_thread_func(void *args) {
    auto *self = static_cast<ProfileManager *>(args);

    UTimer timer;
    utimerCreate(&timer, std::chrono::nanoseconds(100ms).count(), TimerType_Repeating);
    utimerStart(&timer);

    while (true) {
        int idx;
        auto rc = waitMulti(&idx, UINT64_MAX,
            waiterForUTimer(&timer),
            waiterForUEvent(&self->thread_exit_event));
        if (R_FAILED(rc))
            return;

        switch (idx) {
            case 0:
                break;
            case 1:
            default:
                return;
        }

        if (!self->context.is_active)
            continue;

        bool need_apply = false, is_handheld = self->operation_mode == AppletOperationMode_Handheld;

        {
            mutexLock(&self->commit_mutex);
            FZ_SCOPEGUARD([self] { mutexUnlock(&self->commit_mutex); });

            if (self->mmio_available) {
                // Poll DISPLAY_A in handheld mode, DISPLAY_B in docked mode
                std::uint64_t iobase = self->disp_va_base + (is_handheld ? 0 : 0x40000);

                auto &csc = is_handheld ? self->context.saved_internal_csc : self->context.saved_external_csc;
                for (std::size_t i = 0; i < csc.size(); ++i) {
                    if (csc[i] != READ(iobase + DC_COM_CMU_CSC_KRR + i * sizeof(std::uint32_t))) {
                        need_apply = true;
                        break;
                    }
                }
            }
        }

        auto profile_id = is_handheld ? self->context.internal_profile : self->context.external_profile;
        if (!need_apply && profile_id != FizeauProfileId_Invalid) {
            auto &profile = self->context.profiles[profile_id];

            auto time = Clock::get_current_time();
            if (Clock::is_in_interval(time, profile.dusk_begin, profile.dusk_end) ||
                    Clock::is_in_interval(time, profile.dawn_begin, profile.dawn_end))
                need_apply = true;

            std::uint64_t timeout = armNsToTicks(to_timestamp(profile.dimming_timeout) * 1e9),
                delta = armGetSystemTick() - self->activity_tick;

            if (!self->is_dimming && delta > timeout)
                need_apply = true, self->is_dimming = true;
            else if (self->is_dimming && delta <= timeout)
                need_apply = true, self->is_dimming = false;
        }

        if (need_apply) {
            self->apply();

            // Wait a second to avoid calculating/applying the coefficients too frequently
            svcSleepThread(std::chrono::nanoseconds(1s).count());
        }
    }
}

void ProfileManager::event_monitor_thread_func(void *args) {
    auto *self = static_cast<ProfileManager *>(args);

    while (true) {
        int idx;
        auto rc = waitMulti(&idx, UINT64_MAX,
            waiterForEvent(&self->operation_mode_event),
            waiterForEvent(&self->activity_event),
            waiterForEvent(&self->psc_module.event),
            waiterForUEvent(&self->thread_exit_event));
        if (R_FAILED(rc))
            return;

        switch (idx) {
            case 0: {
                ommGetOperationMode(&self->operation_mode);
                break;
            }
            case 1: {
                insrGetLastTick(ins_evt_id, &self->activity_tick);
                break;
            }
            case 2: {
                PscPmState state;
                if (R_SUCCEEDED(pscPmModuleGetRequest(&self->psc_module, &state, nullptr)))
                    self->mmio_available = state == PscPmState_Awake;

                pscPmModuleAcknowledge(&self->psc_module, state);
                break;
            }
            case 3:
            default:
                return;
        }
    }
}

Result ProfileManager::initialize() {
    std::uint64_t size;
    if (auto rc = svcQueryMemoryMapping(&this->disp_va_base, &size, DISP_IO_BASE, DISP_IO_SIZE); R_FAILED(rc))
        diagAbortWithResult(rc);

    std::array deps = { u32(PscPmModuleId_Display) };
    if (auto rc = pscmGetPmModule(&this->psc_module, PscPmModuleId(125), deps.data(), deps.size(), true); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = ommGetOperationModeChangeEvent(&this->operation_mode_event, false); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = ommGetOperationMode(&this->operation_mode); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = insrGetReadableEvent(ins_evt_id, &this->activity_event); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = insrGetLastTick(ins_evt_id, &this->activity_tick); R_FAILED(rc))
        diagAbortWithResult(rc);

    ueventCreate(&this->thread_exit_event, false);

    // The event monitor thread should have a higher priority than the transition thread to ensure it wins on mutex races
    if (auto rc = threadCreate(&this->event_monitor_thread, &ProfileManager::event_monitor_thread_func, this,
            this->event_monitor_thread_stack, sizeof(this->event_monitor_thread_stack), 0x3d, -2); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = threadStart(&this->event_monitor_thread); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = threadCreate(&this->transition_thread, &ProfileManager::transition_thread_func, this,
            this->transition_thread_stack, sizeof(this->transition_thread_stack), 0x3e, -2); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = threadStart(&this->transition_thread); R_FAILED(rc))
        diagAbortWithResult(rc);

    return 0;
}

Result ProfileManager::finalize() {
    ueventSignal(&this->thread_exit_event);

    threadWaitForExit(&this->event_monitor_thread);
    threadWaitForExit(&this->transition_thread);
    threadClose(&this->event_monitor_thread);
    threadClose(&this->transition_thread);

    pscPmModuleFinalize(&this->psc_module);
    pscPmModuleClose(&this->psc_module);
    eventClose(&this->psc_module.event);

    eventClose(&this->operation_mode_event);

    return 0;
}

Result ProfileManager::apply() {
    if (!this->context.is_active)
        return 0;

    auto apply_profile = [this](FizeauProfileId profile_id, bool dim, bool external) -> Result {
        auto &profile = this->context.profiles[profile_id];

        auto time = Clock::get_current_time();
        FizeauSettings settings;

        if (Clock::is_in_interval(time, profile.dusk_begin, profile.dusk_end)) {
            float factor = static_cast<float>(to_timestamp(profile.dusk_end - time))
                / static_cast<float>(to_timestamp(profile.dusk_end - profile.dusk_begin));
            settings = interpolate_profile(profile, factor, false);
        } else if (Clock::is_in_interval(time, profile.dawn_begin, profile.dawn_end)) {
            float factor = static_cast<float>(to_timestamp(profile.dawn_end - time))
                / static_cast<float>(to_timestamp(profile.dawn_end - profile.dawn_begin));
            settings = interpolate_profile(profile, factor, true);
        } else if (Clock::is_in_interval(time, profile.dawn_end, profile.dusk_begin)) {
            settings = profile.day_settings;
        } else {
            settings = profile.night_settings;
        }

        if (dim)
            settings.luminance = !external ? dimmed_luma_internal : dimmed_luma_external;

        auto &csc = !external ? this->context.saved_internal_csc : this->context.saved_external_csc;
        if (auto rc = this->disp.apply_color_profile(external, settings, csc); R_FAILED(rc))
            return rc;

        if (auto rc = this->disp.set_hdmi_color_range(external, settings.range); R_FAILED(rc))
            return rc;

        return 0;
    };

    auto should_dim = [this](auto profile_id, auto timeout) {
        if (profile_id == FizeauProfileId_Invalid)
            return false;

        auto ts = to_timestamp(this->context.profiles[profile_id].dimming_timeout);
        return ts && (timeout >= ts);
    };

    auto timeout = armTicksToNs(armGetSystemTick() - this->activity_tick) / 1'000'000'000;
    bool should_dim_internal = should_dim(this->context.internal_profile, timeout);
    bool should_dim_external = should_dim(this->context.external_profile, timeout);

    mutexLock(&this->commit_mutex);
    FZ_SCOPEGUARD([this] { mutexUnlock(&this->commit_mutex); });

    if (this->context.internal_profile != FizeauProfileId_Invalid) {
        if (auto rc = apply_profile(this->context.internal_profile, should_dim_internal, false); R_FAILED(rc))
            return rc;
    }

    if (this->context.external_profile != FizeauProfileId_Invalid && !this->context.is_lite) {
        if (auto rc = apply_profile(this->context.external_profile, should_dim_external, true); R_FAILED(rc))
            return rc;
    }

    return 0;
}

Result ProfileManager::update_active() {
    if (this->context.is_active) {
        return this->apply();
    } else {
        if (auto rc = this->disp.disable(false); R_FAILED(rc))
            return rc;

        if (!this->context.is_lite) {
            if (auto rc = this->disp.disable(true); R_FAILED(rc))
                return rc;
        }
    }

    return 0;
}

} // namespace fz
