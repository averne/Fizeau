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

#include "brightness.hpp"
#include "cmu.hpp"
#include "service.hpp"

// libnx tweaking
extern "C" {
    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE (12 * ams::os::MemoryPageSize)
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char nx_inner_heap[INNER_HEAP_SIZE];

    // 32kib is the smallest possible transfer memory size
    u32 __nx_nv_transfermem_size = 8 * ams::os::MemoryPageSize;

    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
}

// libstratosphere tweaking
namespace ams {
    ncm::ProgramId CurrentProgramId = { 0x0100000000000f12 };
    namespace result {
        bool CallFatalOnResultAssertion = false;
    }
}

#ifdef DEBUG
TwiliPipe g_twlPipe;
#endif

extern "C" void __libnx_initheap(void) {
    // Newlib
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    fake_heap_start = nx_inner_heap;
    fake_heap_end   = nx_inner_heap + nx_inner_heap_size;
}

extern "C" void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
}

extern "C" void __appInit(void) {
    ams::hos::InitializeForStratosphere();

    ams::sm::DoWithSession([] {
        R_ABORT_UNLESS(nvInitialize());
        R_ABORT_UNLESS(lblInitialize());
#ifdef DEBUG
        R_ABORT_UNLESS(twiliInitialize());
        R_ABORT_UNLESS(twiliCreateNamedOutputPipe(&g_twlPipe, "fzout"));
#endif
    });

    ams::CheckApiVersion();
}

extern "C" void __appExit(void) {
    nvExit();
    lblExit();
#ifdef DEBUG
    twiliClosePipe(&g_twlPipe);
    twiliExit();
#endif
}

namespace {

constexpr auto service_name = ams::sm::ServiceName::Encode("fizeau");
constexpr std::size_t num_servers  = 1;
constexpr std::size_t max_sessions = 2;
ams::sf::hipc::ServerManager<num_servers, ams::sf::hipc::DefaultServerManagerOptions, max_sessions> server_manager;

} // namespace

int main(int argc, char **argv) {
    LOG("Initializing\n");
    R_ABORT_UNLESS(fz::CmuManager::initialize());
    R_ABORT_UNLESS(fz::BrightnessManager::initialize());
    R_ABORT_UNLESS(fz::ProfileManager::initialize());
    R_ABORT_UNLESS(fz::Clock::initialize());

    LOG("Starting server\n");
    R_ABORT_UNLESS(server_manager.RegisterServer<fz::FizeauService>(service_name, max_sessions));
    server_manager.LoopProcess();

    LOG("Exiting\n");
    fz::CmuManager::finalize();
    fz::BrightnessManager::finalize();
    return 0;
}
