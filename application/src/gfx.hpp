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

namespace fz::gfx {

bool init();
bool loop();
void render();
void exit();
DkResHandle create_texture(std::uint8_t *data, int width, int height, std::uint32_t sampler_id, std::uint32_t image_id);

class TextureDecoder {
    public:
        TextureDecoder() = default;

        TextureDecoder(const std::string_view &path, std::uint32_t sampler_id, std::uint32_t image_id) {
            this->start(path, sampler_id, image_id);
        }

        void start(const std::string_view &path, std::uint32_t sampler_id, std::uint32_t image_id);
        void end();

        bool done() const {
            return this->is_done;
        }

        DkResHandle get_handle() {
            this->end();
            return this->handle;
        }

    private:
        std::atomic_bool is_done = false;
        bool has_joined = false;
        std::thread thread;

        std::uint8_t *data;
        int width, height;
        std::uint32_t sampler_id, image_id;
        DkResHandle handle;
};

} // namespace fz::gfx
