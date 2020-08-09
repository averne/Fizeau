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

#include <tesla.hpp>
#include <common.hpp>

#include "gui.hpp"

#ifdef DEBUG
TwiliPipe g_twlPipe;
#endif

class FizeauOverlay: public tsl::Overlay {
    public:
        virtual void initServices() final override {
#ifdef DEBUG
            twiliInitialize();
            twiliCreateNamedOutputPipe(&g_twlPipe, "fzovlout");
#endif
            fz::Clock::initialize();
            apmInitialize();
        }

        virtual void exitServices() final override {
            apmExit();
#ifdef DEBUG
            twiliClosePipe(&g_twlPipe);
            twiliExit();
#endif
        }

        virtual std::unique_ptr<tsl::Gui> loadInitialGui() final override {
            return initially<fz::FizeauOverlayGui>();
        }

        virtual void onHide() final override {
            tsl::Overlay::get()->close();
        }
};

int main(int argc, char **argv) {
    LOG("Starting overlay\n");
    return tsl::loop<FizeauOverlay>(argc, argv);
}
