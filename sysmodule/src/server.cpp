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
#include <type_traits>
#include <utility>

#include <common.hpp>

#include "server.hpp"

namespace fz {

#define SET_OUTDATA(v) ({                               \
    *(std::remove_cvref_t<decltype(v)> *)out_data = v;  \
    *out_datasize = sizeof(v);                          \
})

Result Server::command_handler(void *userdata, const IpcServerRequest *r, u8 *out_data, size_t *out_datasize) {
    auto *self = static_cast<Server *>(userdata);

    switch (r->data.cmdId) {
        case FizeauCommandId_GetIsActive: {
            SET_OUTDATA(self->context.is_active);
            break;
        }
        case FizeauCommandId_SetIsActive: {
            auto prev_active = std::exchange(self->context.is_active, *(bool *)r->data.ptr);

            if (prev_active != self->context.is_active)
                self->profile.update_active();

            break;
        }
        case FizeauCommandId_GetProfile: {
            auto id = *(FizeauProfileId *)r->data.ptr;
            if (id < FizeauProfileId_Profile1 || id > FizeauProfileId_Profile4)
                return FIZEAU_MAKERESULT(INVALID_PROFILEID);

            SET_OUTDATA(self->context.profiles[id]);
            break;
        }
        case FizeauCommandId_SetProfile: {
            auto id = *(FizeauProfileId *)r->data.ptr;
            if (id < FizeauProfileId_Profile1 || id > FizeauProfileId_Profile4)
                return FIZEAU_MAKERESULT(INVALID_PROFILEID);

            self->context.profiles[id] = *(FizeauProfile *)((std::uint8_t *)r->data.ptr + std::max(alignof(FizeauProfileId), alignof(FizeauProfile)));

            if (id == self->context.internal_profile || id == self->context.external_profile) {
                if (auto rc = self->profile.apply(); R_FAILED(rc))
                    return rc;
            }

            break;
        }
        case FizeauCommandId_GetActiveProfileId: {
            auto external = *(bool *)r->data.ptr;
            SET_OUTDATA(!external ? self->context.internal_profile : self->context.external_profile);
            break;
        }
        case FizeauCommandId_SetActiveProfileId: {
            auto external = *(bool *)r->data.ptr;
            auto id = *(FizeauProfileId *)((std::uint8_t *)r->data.ptr + std::max(alignof(bool), alignof(FizeauProfileId)));
            if (id < FizeauProfileId_Profile1 || id > FizeauProfileId_Profile4)
                return FIZEAU_MAKERESULT(INVALID_PROFILEID);

            (!external ? self->context.internal_profile : self->context.external_profile) = id;

            if (auto rc = self->profile.apply(); R_FAILED(rc))
                return rc;

            break;
        }
        default:
            return MAKERESULT(10, 221);
    }

    return 0;
}

} // namespace fz
