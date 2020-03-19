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

#include <cstdio>
#include <memory>
#include <switch.h>
#include <stratosphere.hpp>
#include <common.hpp>

#include "layer.hpp"
#include "screen.hpp"
#include "service.hpp"

// libnx tweaking
extern "C" {
    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE (0x1e * ams::os::MemoryPageSize)
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char nx_inner_heap[INNER_HEAP_SIZE];

    u32 __nx_nv_transfermem_size = 0x15 * ams::os::MemoryPageSize;

    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
}

// libstratosphere tweaking
namespace ams {
ncm::ProgramId CurrentProgramId = {0x0100000000000f12};
namespace result { bool CallFatalOnResultAssertion = false; }
}

extern "C" void __libnx_initheap(void) {
    // Newlib
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    fake_heap_start = nx_inner_heap;
    fake_heap_end   = nx_inner_heap + nx_inner_heap_size;
}

extern "C" void __libnx_exception_handler(ThreadExceptionDump *ctx) {

}

extern "C" void __appInit(void) {
    fz::do_with_sm_session([] {
        R_ABORT_UNLESS(viInitialize(ViServiceType_Manager));
        R_ABORT_UNLESS(timeInitialize());
        R_ABORT_UNLESS(lblInitialize());
    });
}

extern "C" void __appExit(void) {
    SERV_EXIT(vi);
    SERV_EXIT(time);
    SERV_EXIT(lbl);
}

int main(int argc, char **argv) {
    static auto layer = fz::Layer();

    static auto time_update_thread = ams::os::StaticThread<2 * ams::os::MemoryPageSize>(
        +[](void *args) {
            while (true) {
                svcSleepThread(2e+8l);
                static_cast<fz::Layer *>(args)->update(fz::get_time());
            }
        },
        static_cast<void *>(&layer),
        0x3f
    );
    R_ABORT_UNLESS(time_update_thread.Start());

    static constexpr auto service_name = ams::sm::ServiceName::Encode("fizeau");
    static constexpr std::size_t max_sessions = 2;
    static constexpr std::size_t num_servers  = 1;
    static ams::sf::hipc::ServerManager<num_servers, ams::sf::hipc::DefaultServerManagerOptions, max_sessions> server_manager;
    R_ABORT_UNLESS(server_manager.RegisterServer<fz::FizeauService>(service_name, max_sessions));
    server_manager.LoopProcess();

    time_update_thread.Join();

    return 0;
}
