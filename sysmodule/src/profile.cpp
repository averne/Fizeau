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

#include "t210_regs.hpp"
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
        .hue             = std::lerp(from.hue,        to.hue,        factor),
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

        // CMU resets
        if (!need_apply) {
            if (!(READ(self->clock_va_base + CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & (CLK_ENB_DISP1 | CLK_ENB_DISP2)) ||
                    !mutexTryLock(&self->commit_mutex))
                goto cmu_end;

            FZ_SCOPEGUARD([self] { mutexUnlock(&self->commit_mutex); });

            // Poll DISPLAY_A in handheld mode, DISPLAY_B in docked mode
            std::uint64_t iobase = self->disp_va_base + (is_handheld ? 0 : 0x40000);

            auto &shadow = is_handheld ? self->context.cmu_shadow_internal : self->context.cmu_shadow_external;
            auto &csc    = shadow.csc;

            // There is a race when waking from reset, where the configuration
            // sometimes gets applied before nvdrv internally disables the CMU
            if (!(READ(iobase + DC_DISP_DISP_COLOR_CONTROL) & CMU_ENABLE)) {
                need_apply = true;
                goto cmu_end;
            }

            for (std::size_t i = 0; i < csc.size(); ++i) {
                if (csc[i] != READ(iobase + DC_COM_CMU_CSC_KRR + i * sizeof(std::uint32_t))) {
                    need_apply = true;
                    goto cmu_end;
                }
            }
        }

cmu_end:
        auto profile_id = is_handheld ? self->context.internal_profile : self->context.external_profile;
        if (profile_id >= FizeauProfileId_Total)
            continue;

        auto &profile = self->context.profiles      [profile_id];
        auto &state   = self->context.profile_states[profile_id];

        // Period transitions
        if (!need_apply) {
            auto dub = to_timestamp(profile.dusk_begin), due = to_timestamp(profile.dusk_end),
                 dab = to_timestamp(profile.dawn_begin), dae = to_timestamp(profile.dawn_end);

            auto ts = Clock::get_current_timestamp();
            if (ts >= due)
                need_apply = state == FizeauProfileState::Day;
            else if (ts >= dub)
                need_apply = true;
            else if (ts >= dae)
                need_apply = state == FizeauProfileState::Night;
            else if (ts >= dab)
                need_apply = true;

            // Increase next timeout to avoid calculating/applying the coefficients too frequently
            if (need_apply)
                timer.next_tick += armNsToTicks(std::chrono::nanoseconds(1s).count());
        }

        // Dimming
        if (!need_apply) {
            std::uint64_t timeout = to_timestamp(profile.dimming_timeout),
                delta = armTicksToNs(armGetSystemTick() - self->activity_tick) / std::chrono::nanoseconds(1s).count();

            if (
                (!self->is_dimming && delta >  timeout) ||
                ( self->is_dimming && delta <= timeout)
            )
                need_apply = true;
        }

        if (need_apply)
            self->apply();
    }
}

void ProfileManager::event_monitor_thread_func(void *args) {
    auto *self = static_cast<ProfileManager *>(args);

    while (true) {
        int idx;
        auto rc = waitMulti(&idx, UINT64_MAX,
            waiterForEvent(&self->operation_mode_event),
            waiterForEvent(&self->activity_event),
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
            case 2:
            default:
                return;
        }
    }
}

Result ProfileManager::initialize() {
    std::uint64_t size;
    if (auto rc = svcQueryMemoryMapping(&this->clock_va_base, &size, CLOCK_IO_BASE, CLOCK_IO_SIZE); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = svcQueryMemoryMapping(&this->disp_va_base, &size, DISP_IO_BASE, DISP_IO_SIZE); R_FAILED(rc))
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

    eventClose(&this->operation_mode_event);

    return 0;
}

Result ProfileManager::apply() {
    if (!this->context.is_active)
        return 0;

    auto apply_profile = [this](FizeauProfileId profile_id, bool dim, bool external) -> Result {
        auto &profile = this->context.profiles      [profile_id];
        auto &state   = this->context.profile_states[profile_id];

        FizeauSettings settings;

        auto dub = to_timestamp(profile.dusk_begin), due = to_timestamp(profile.dusk_end),
             dab = to_timestamp(profile.dawn_begin), dae = to_timestamp(profile.dawn_end);

        auto ts = Clock::get_current_timestamp();
        if (Clock::is_in_interval(ts, dub, due)) {
            float factor = static_cast<float>(due - ts) / static_cast<float>(due - dub);
            settings = interpolate_profile(profile, factor, false);
            state    = FizeauProfileState::Night;
        } else if (Clock::is_in_interval(ts, dab, dae)) {
            float factor = static_cast<float>(dae - ts) / static_cast<float>(dae - dab);
            settings = interpolate_profile(profile, factor, true);
            state    = FizeauProfileState::Day;
        } else if (Clock::is_in_interval(ts, dae, dub)) {
            settings = profile.day_settings;
            state    = FizeauProfileState::Day;
        } else {
            settings = profile.night_settings;
            state    = FizeauProfileState::Night;
        }

        if (dim)
            settings.luminance = !external ? dimmed_luma_internal : dimmed_luma_external;

        auto &shadow = !external ? this->context.cmu_shadow_internal : this->context.cmu_shadow_external;
        if (auto rc = this->disp.apply_color_profile(external, settings, shadow); R_FAILED(rc))
            return rc;

        if (auto rc = this->disp.set_hdmi_color_range(external, settings.range); R_FAILED(rc))
            return rc;

        return 0;
    };

    auto should_dim = [this](auto profile_id, auto timeout) {
        if (profile_id >= FizeauProfileId_Total)
            return false;

        auto ts = to_timestamp(this->context.profiles[profile_id].dimming_timeout);
        return ts && (timeout >= ts);
    };

    auto timeout = armTicksToNs(armGetSystemTick() - this->activity_tick) / 1'000'000'000;
    bool should_dim_internal = should_dim(this->context.internal_profile, timeout);
    bool should_dim_external = should_dim(this->context.external_profile, timeout);

    auto is_handheld = this->operation_mode == AppletOperationMode_Handheld;
    this->is_dimming = is_handheld ? should_dim_internal : should_dim_external;

    mutexLock(&this->commit_mutex);
    FZ_SCOPEGUARD([this] { mutexUnlock(&this->commit_mutex); });

    if (this->context.internal_profile < FizeauProfileId_Total) {
        if (auto rc = apply_profile(this->context.internal_profile, should_dim_internal, false); R_FAILED(rc))
            return rc;
    }

    if (this->context.external_profile < FizeauProfileId_Total && !this->context.is_lite) {
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
