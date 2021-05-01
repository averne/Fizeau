#include <switch.h>
#include <stratosphere.hpp>

#include <common.hpp>

extern "C" {
    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE (0x2 * ams::os::MemoryPageSize)
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char nx_inner_heap[INNER_HEAP_SIZE];

    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
}

namespace ams {
    ncm::ProgramId CurrentProgramId = { 0x010000000000cf12 };
    namespace result { bool CallFatalOnResultAssertion = false; }
} // namespace ams

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

    R_ABORT_UNLESS(smInitialize());
    R_ABORT_UNLESS(pmshellInitialize());
    R_ABORT_UNLESS(fsInitialize());
    R_ABORT_UNLESS(fsdevMountSdmc());

#ifdef DEBUG
    R_ABORT_UNLESS(twiliInitialize());
    R_ABORT_UNLESS(twiliCreateNamedOutputPipe(&g_twlPipe, "fzout"));
#endif
}

extern "C" void __appExit(void) {
#ifdef DEBUG
    twiliClosePipe(&g_twlPipe);
    twiliExit();
#endif

    fsdevUnmountAll();
    fsExit();
    pmshellExit();
    smExit();
}

int main() {
    LOG("Launching program\n");
    constexpr NcmProgramLocation fizeau_program_location = { 0x0100000000000f12ul, NcmStorageId_None };
    R_ABORT_UNLESS(pmshellLaunchProgram(PmLaunchFlag_None, &fizeau_program_location, nullptr));

    LOG("Initializing service\n");
    R_ABORT_UNLESS(fizeauInitialize());
    ON_SCOPE_EXIT { fizeauExit(); };

    LOG("Applying settings\n");
    static auto config = fz::cfg::read(fz::cfg::path);
    R_ABORT_UNLESS(fizeauSetIsActive(true));
    fizeauProfileClose(&config.cur_profile);

    LOG("Done, exiting\n");
    return 0;
}
