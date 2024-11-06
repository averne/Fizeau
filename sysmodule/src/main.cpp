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

#include <cstring>
#include <ini.h>
#include <switch.h>

#include <omm.h>
#include <common.hpp>

#include "context.hpp"
#include "profile.hpp"
#include "nvdisp.hpp"
#include "server.hpp"

#if defined(DEBUG) && defined(TWILI)
TwiliPipe g_twlPipe;
#endif

// libnx tweaking
extern "C" {

u32 __nx_applet_type = AppletType_None;

// 32kiB is the smallest possible transfer memory size (nvnflinger uses 400kiB)
#define NVDRV_TMEM_SIZE (8 * 0x1000)
char nvdrv_tmem_data[NVDRV_TMEM_SIZE] alignas(0x1000);

// The sysmodule does not use heap allocation
void __libnx_initheap(void) { }
void *__libnx_alloc(size_t size) { return nullptr; }
void *__libnx_aligned_alloc(size_t alignment, size_t size) { return nullptr; }
void __libnx_free(void* p) { }

Result __nx_nv_create_tmem(TransferMemory *t, u32 *out_size, Permission perm) {
    *out_size = NVDRV_TMEM_SIZE;
    return tmemCreateFromMemory(t, nvdrv_tmem_data, NVDRV_TMEM_SIZE, perm);
}

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    diagAbortWithResult(ctx->error_desc);
}

void __appInit(void) {
#if defined(DEBUG) && !defined(TWILI)
    u64 has_debugger = 0;
    while (!has_debugger) {
        if (auto rc = svcGetInfo(&has_debugger, InfoType_DebuggerAttached, INVALID_HANDLE, 0); R_FAILED(rc))
            diagAbortWithResult(rc);

        if (has_debugger)
            break;
        svcSleepThread(1e6);
    }
#endif

    if (auto rc = smInitialize(); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = splInitialize(); R_FAILED(rc))
        diagAbortWithResult(rc);

    u64 exo_api_ver;
    if (auto rc = splGetConfig(static_cast<SplConfigItem>(65000), &exo_api_ver); R_FAILED(rc))
        diagAbortWithResult(rc);

    hosversionSet(BIT(31) | ((exo_api_ver >> 8) & 0xffffff));

    if (auto rc = nvInitialize(); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = ommInitialize(); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = insrInitialize(); R_FAILED(rc))
        diagAbortWithResult(rc);

#if defined(DEBUG) && defined(TWILI)
    if (auto rc = twiliInitialize(); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = twiliCreateNamedOutputPipe(&g_twlPipe, "fzout"); R_FAILED(rc))
        diagAbortWithResult(rc);
#endif
}

void __appExit(void) {
    nvExit();
    ommExit();
    insrExit();

#if defined(DEBUG) && defined(TWILI)
    twiliClosePipe(&g_twlPipe);
    twiliExit();
#endif

    splExit();
    smExit();
}

} // extern "C"

static constinit fz::Context           context = {};
static constinit fz::DisplayController disp    = {};
static constinit fz::ProfileManager    profile(context, disp);
static constinit fz::Server            server (context, profile);

FsFile find_config_file(FsFileSystem fs) {
    FsFile fp = {};
    char buf[FS_MAX_PATH];
    for (auto path: fz::Config::config_locations) {
        std::strncpy(buf, path.data(), sizeof(buf) - 1);
        if (auto rc = fsFsOpenFile(&fs, buf, FsOpenMode_Read, &fp); R_SUCCEEDED(rc))
            break;
    }
    return fp;
}

bool parse_config() {
    auto rc = fsInitialize();
    FZ_SCOPEGUARD([] { fsExit(); });

    FsFileSystem fs;
    if (R_SUCCEEDED(rc))
        rc = fsOpenSdCardFileSystem(&fs);
    FZ_SCOPEGUARD([&fs] { fsFsClose(&fs); });

    FsFile fp;
    FZ_SCOPEGUARD([&fp] { fsFileClose(&fp); });
    if (R_SUCCEEDED(rc))
        fp = find_config_file(fs);

    if (fp.s.session == INVALID_HANDLE)
        return false;

    struct FileReadContext {
        FsFile fp;
        std::uint64_t off = 0;
    } read_ctx = { fp };

    fz::Config config;
    config.parse_profile_switch_action = +[](fz::Config *self, FizeauProfileId profile_id) {
        if (self->cur_profile_id == FizeauProfileId_Invalid)
            return;
        context.profiles[self->cur_profile_id] = self->profile;
        self->profile = {};
    };

    ini_reader reader = +[](char *str, int num, void *stream) -> char * {
        auto *p = str;
        auto *read_ctx = static_cast<FileReadContext *>(stream);

        while (--num > 1) {
            char dat;
            std::uint64_t read;
            if (auto rc = fsFileRead(&read_ctx->fp, read_ctx->off, &dat, sizeof(dat), FsReadOption_None, &read); R_FAILED(rc) || !read)
                return nullptr;

            read_ctx->off += read;
            if (dat == '\n' || dat == '\r')
                break;

            *p++ = dat;
        }

        *p = '\0';
        return str;
    };

    if (auto res = ini_parse_stream(reader, &read_ctx, fz::Config::ini_handler, &config); !res) {
        context.is_active        = config.active;
        context.internal_profile = config.internal_profile;
        context.external_profile = config.external_profile;
    }

    return true;
}

int main(int argc, char **argv) {
    LOG("Initializing\n");

    u64 hw_type;
    if (auto rc = splGetConfig(SplConfigItem_HardwareType, reinterpret_cast<u64 *>(&hw_type)); R_FAILED(rc))
        diagAbortWithResult(rc);
    context.is_lite = hw_type == 2; // Hoag

    if (auto rc = fz::Clock::initialize(); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = disp.initialize(); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (auto rc = profile.initialize(); R_FAILED(rc))
        diagAbortWithResult(rc);

    if (parse_config())
        profile.apply();

    LOG("Starting server\n");
    if (auto rc = server.initialize(); R_FAILED(rc))
        diagAbortWithResult(rc);

    server.loop();

    server .finalize();
    profile.finalize();
    disp   .finalize();

    LOG("Exiting\n");
    return 0;
}
