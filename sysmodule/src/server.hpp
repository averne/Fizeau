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

#include <string_view>

#include "context.hpp"
#include "ipc_server.h"
#include "profile.hpp"

namespace fz {

class Server: public IpcServer {
    public:
        constexpr static inline std::string_view ServiceName = "fizeau";
        constexpr static inline int ServiceNumSessions = 2;

    public:
        Server(Context &context, ProfileManager &profile): context(context), profile(profile) { }

        Result initialize() {
            return ipcServerInit(this, Server::ServiceName.data(), Server::ServiceNumSessions);
        }

        Result finalize() {
            return ipcServerExit(this);
        }

        Result process() {
            return ipcServerProcess(this, &command_handler, this);
        }

        Result loop() {
            while (true) {
                switch (auto rc = this->process()) {
                    case 0:
                    case KERNELRESULT(ConnectionClosed):
                        break;
                    default:
                        return rc;
                }
            }
        }

    private:
        static Result command_handler(void *userdata, const IpcServerRequest *r, u8 *out_data, size_t *out_datasize);

    private:
        Context &context;
        ProfileManager &profile;

        bool running = false;
};

} // namespace fz
