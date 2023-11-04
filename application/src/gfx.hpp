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

#include <cstdint>
#include <atomic>
#include <string_view>
#include <thread>
#include <deko3d.hpp>
#include <nvjpg.hpp>

namespace fz::gfx {

bool init();
bool loop();
void render();
void exit();

void register_texture(dk::MemBlock memblk, dk::Image &image, const nj::Surface &surf, std::uint32_t sampler_id, std::uint32_t image_id);

} // namespace fz::gfx
