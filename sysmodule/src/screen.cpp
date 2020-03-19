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

#include <switch.h>
#include <stratosphere.hpp>

#include <common.hpp>

#include "screen.hpp"

extern "C" u64 __nx_vi_layer_id;

namespace fz {

Result Screen::initialize(Vec2u fb_sz, Vec2f layer_pos, Vec2i layer_sz,
        std::uint32_t format, std::uint32_t num_fb) {
    return do_with_sm_session([&]() -> Result {
        Result rc;
        TRY_GOTO(rc = viOpenDefaultDisplay(&this->display), end);
        TRY_GOTO(rc = viCreateManagedLayer(&this->display, static_cast<ViLayerFlags>(0), 0, &__nx_vi_layer_id), close_display); // flag 0 allows non-fullscreen layer
        TRY_GOTO(rc = viCreateLayer(&this->display, &this->layer), close_managed_layer);
        TRY_GOTO(rc = viSetLayerScalingMode(&this->layer, ViScalingMode_FitToLayer), close_managed_layer);
        TRY_GOTO(rc = viSetLayerZ(&this->layer, 100), close_managed_layer); // Arbitrary z index
        TRY_GOTO(rc = viSetLayerSize(&this->layer, layer_sz.w, layer_sz.h), close_managed_layer);
        TRY_GOTO(rc = viSetLayerPosition(&this->layer, layer_pos.x, layer_pos.y), close_managed_layer);
        TRY_GOTO(rc = nwindowCreateFromLayer(&this->window, &this->layer), close_managed_layer);
        TRY_GOTO(rc = framebufferCreate(&this->framebuf, &this->window, fb_sz.w, fb_sz.h, format, num_fb), close_window);

        return 0;

        close_window:
            nwindowClose(&this->window);
        close_managed_layer:
            viDestroyManagedLayer(&this->layer);
        close_display:
            viCloseDisplay(&this->display);
        end:
            return rc;
    });
}

void Screen::finalize() {
    framebufferClose(&this->framebuf);
    nwindowClose(&this->window);
    viDestroyManagedLayer(&this->layer);
    viCloseDisplay(&this->display);
}

} // namespace fz
