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
#include <vector>
#include <string>
#include <switch.h>
#include <stratosphere.hpp>
#include <common.hpp>

#include "gfx.hpp"
#include "gui.hpp"

extern "C" void userAppInit() {
#ifdef DEBUG
#   ifdef TWILI
    twiliInitialize();
    twiliBindStdio();
#   else
    socketInitializeDefault();
    nxlinkStdio();
#   endif
#endif
    plInitialize(PlServiceType_User);
    romfsInit();
    appletLockExit();
}

extern "C" void userAppExit(void) {
#ifdef DEBUG
#   ifdef TWILI
    twiliExit();
#   else
    socketExit();
#   endif
#endif
    romfsExit();
    plExit();
    appletUnlockExit();
}

int main(int argc, char **argv) {
    LOG("Starting Fizeau\n");

    fz::cfg::Config config;

    if (!fz::gfx::init() )
        LOG("Failed to init\n");

    fz::gfx::TextureDecoder background_decoder("romfs:/background.jpg", 1, 1),
        preview_decoder("romfs:/preview.jpg", 2, 2);

    fz::gui::init();

    bool is_active;
    Result rc = fizeauIsServiceActive(&is_active);
    if (R_FAILED(rc) || !is_active) {
        LOG("Service not active: rc %#x, active %d\n", rc, is_active);
        rc = 1;
    }

    if (R_SUCCEEDED(rc))
        rc = fizeauInitialize();

    if (R_SUCCEEDED(rc))
        config = fz::cfg::read(fz::cfg::path);

    if (R_SUCCEEDED(rc))
        rc = fz::cfg::open_profile(config, FizeauProfileId_Profile1);

    if (R_SUCCEEDED(rc))
        rc = fz::Clock::initialize();

    while (fz::gfx::loop()) {
        if (R_FAILED(rc)) {
            fz::gui::draw_error_window(config, rc);
            fz::gfx::render();
            continue;
        }

        if (background_decoder.done())
            fz::gui::draw_background(config, background_decoder.get_handle());

        fz::gui::draw_preview_window(config, preview_decoder.done() ? preview_decoder.get_handle() : -1);
        fz::gui::draw_graph_window(config);
        rc = fz::gui::draw_main_window(config);

        static bool prev_editing_day = false;
        if (config.is_editing_day_profile && config.is_editing_night_profile) {
            if (prev_editing_day)
                config.is_editing_day_profile   = false, prev_editing_day = false;
            else
                config.is_editing_night_profile = false, prev_editing_day = true;
        }

        fz::gfx::render();
    }

    fz::cfg::dump(fz::cfg::path, config);

    background_decoder.end(), preview_decoder.end();

    LOG("Exiting Fizeau\n");
    fizeauProfileClose(&config.cur_profile);
    fizeauExit();
    fz::gui::exit();
    fz::gfx::exit();

    return 0;
}
