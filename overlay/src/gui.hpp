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

#pragma once

#include <memory>
#include <utility>
#include <tesla.hpp>
#include <common.hpp>

namespace fz {

class ErrorGui: public tsl::Gui {
    public:
        ErrorGui(Result rc): rc(rc) { }

        virtual ~ErrorGui() {
            tsl::Overlay::get()->close();
        }

        virtual tsl::elm::Element *createUI() final override;

    private:
        Result rc;
};

class FizeauOverlayGui: public tsl::Gui {
    public:
        FizeauOverlayGui();
        virtual ~FizeauOverlayGui();

        virtual tsl::elm::Element *createUI() final override;

        virtual void update() final override;

        Config &get_config() {
            return this->config;
        }

    private:
        Result rc;
        bool is_day;
        Config config = {};

        tsl::elm::CustomDrawer      *info_header;
        tsl::elm::ListItem          *active_button;
        tsl::elm::ListItem          *apply_button;
        tsl::elm::TrackBar          *temp_slider;
        tsl::elm::TrackBar          *sat_slider;
        tsl::elm::TrackBar          *hue_slider;
        tsl::elm::NamedStepTrackBar *components_bar;
        tsl::elm::NamedStepTrackBar *filter_bar;
        tsl::elm::TrackBar          *contrast_slider;
        tsl::elm::TrackBar          *gamma_slider;
        tsl::elm::TrackBar          *luma_slider;
        tsl::elm::ListItem          *range_button;

        tsl::elm::CategoryHeader *temp_header, *sat_header, *hue_header,
            *components_header, *filter_header, *contrast_header, *gamma_header, *luma_header;
};

} // namespace fz
