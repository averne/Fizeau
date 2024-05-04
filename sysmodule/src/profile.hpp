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

#pragma once

#include <cstdint>
#include <array>

#include <common.hpp>

#include "context.hpp"
#include "nvdisp.hpp"

namespace fz {

constexpr float dimmed_luma_internal = -0.1f, dimmed_luma_external = -0.7f; // Official values used in 6.0.0 am

class ProfileManager {
    public:
        ProfileManager(Context &context, DisplayController &disp): context(context), disp(disp) { }

        Result initialize();
        Result finalize();

        Result apply();
        Result update_active();

    private:
        static void transition_thread_func(void *args);
        static void event_monitor_thread_func(void *args);

    private:
        Context &context;
        DisplayController &disp;

        UEvent thread_exit_event = {};
        Thread transition_thread, event_monitor_thread;
        std::uint8_t transition_thread_stack[0x2000] alignas(0x1000),
            event_monitor_thread_stack[0x1000] alignas(0x1000);

        PscPmModule psc_module;
        bool mmio_available = true;
        std::uint64_t disp_va_base = 0;

        Event operation_mode_event;
        AppletOperationMode operation_mode;

        Event activity_event;
        std::uint64_t activity_tick;
        bool is_dimming = false;

        Mutex commit_mutex = {};
};

} // namespace fz
