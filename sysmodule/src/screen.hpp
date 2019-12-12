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

#pragma once

#include <utility>
#include <algorithm>
#include <switch.h>

#include <common.hpp>

namespace fz {

class Screen {
    public:
        // Handheld and docked
        static constexpr inline std::uint32_t width  = 1920;
        static constexpr inline std::uint32_t height = 1080;

        static constexpr inline std::uint32_t fb_bpp = sizeof(rgba4444_t);

    private:
        ViDisplay   display;
        ViLayer     layer;
        NWindow     window;
        Framebuffer framebuf;

    public:
        Result initialize(utils::Vec2u fb_sz, utils::Vec2f layer_pos, utils::Vec2i layer_sz,
            std::uint32_t format = PIXEL_FORMAT_RGBA_4444, std::uint32_t num_fbs = 2);
        void finalize();

        inline u32    get_window_width()  const { return this->window.width; }
        inline u32    get_window_height() const { return this->window.height; }
        inline Result set_window_pos(utils::Vec2f pos)  { return viSetLayerPosition(&this->layer, pos.x, pos.y); }
        inline Result set_window_size(utils::Vec2ul sz) { return viSetLayerSize(&this->layer, sz.w, sz.h); }

        inline u32 get_framebuffer_width()  const { return this->framebuf.width_aligned; }
        inline u32 get_framebuffer_height() const { return this->framebuf.height_aligned; }
        inline u32 get_framebuffer_size()   const { return this->framebuf.fb_size; }

        inline rgba4444_t *get_cur_buffer() {
            return reinterpret_cast<rgba4444_t *>
                (reinterpret_cast<std::uint8_t *>(this->framebuf.buf) + get_framebuffer_size() * this->window.cur_slot);
        }

        inline rgba4444_t *dequeue() { return reinterpret_cast<rgba4444_t *>(framebufferBegin(&this->framebuf, NULL)); }
        inline void flush() { framebufferEnd(&this->framebuf); }

        inline void fill(rgba4444_t color) {
            std::fill_n(reinterpret_cast<rgba4444_t *>(get_cur_buffer()), get_framebuffer_size(), color);
        }
};

} // namespace fz
