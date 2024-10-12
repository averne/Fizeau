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

#include "gui.hpp"

namespace fz {

namespace {

template <typename ...Args>
std::string format(const std::string_view &fmt, Args &&...args) {
    std::string str(std::snprintf(nullptr, 0, fmt.data(), args...) + 1, 0);
    std::snprintf(str.data(), str.capacity(), fmt.data(), args...);
    return str;
}

bool is_full(const ColorRange &range) {
    return (range.lo == MIN_RANGE) && (range.hi == MAX_RANGE);
}

} // namespace

tsl::elm::Element *ErrorGui::createUI() {
    auto *frame = new tsl::elm::OverlayFrame("Fizeau", VERSION);

    auto *drawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString(format("%#x (%04d-%04d)", this->rc, R_MODULE(this->rc) + 2000, R_DESCRIPTION(this->rc)).c_str(),
                                                                     false, x, y +  50, 20, renderer->a(0xffff));
        renderer->drawString("An error occurred",                    false, x, y +  80, 20, renderer->a(0xffff));
        renderer->drawString("Please make sure you are using the",   false, x, y + 110, 20, renderer->a(0xffff));
        renderer->drawString("latest release.",                      false, x, y + 130, 20, renderer->a(0xffff));
        renderer->drawString("Otherwise, make an issue on github:",  false, x, y + 150, 20, renderer->a(0xffff));
        renderer->drawString("https://www.github.com/averne/Fizeau", false, x, y + 170, 18, renderer->a(0xffff));
    });

    frame->setContent(drawer);
    return frame;
}

FizeauOverlayGui::FizeauOverlayGui() {
    tsl::hlp::doWithSmSession([this] {
        this->rc = fizeauInitialize();
    });
    if (R_FAILED(rc))
        return;

    tsl::hlp::doWithSDCardHandle([this] { this->config.read(); });

    ApmPerformanceMode perf_mode;
    if (this->rc = apmGetPerformanceMode(&perf_mode); R_FAILED(this->rc))
        return;

    if (this->rc = this->config.open_profile(perf_mode == ApmPerformanceMode_Normal ?
                                 config.internal_profile : config.external_profile); R_FAILED(this->rc))
        return;

    this->is_day = Clock::is_in_interval(this->config.profile.dawn_begin, this->config.profile.dusk_begin);
}

FizeauOverlayGui::~FizeauOverlayGui() {
    tsl::hlp::doWithSDCardHandle([this] { this->config.write(); });
    fizeauExit();
}

tsl::elm::Element *FizeauOverlayGui::createUI() {
    this->info_header = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString(format("Editing profile: %u", static_cast<std::uint32_t>(this->config.cur_profile_id) + 1).c_str(),
            false, x, y + 20, 20, renderer->a(0xffff));
        renderer->drawString(format("In period: %s", this->is_day ? "day" : "night").c_str(),
            false, x, y + 45, 20, renderer->a(0xffff));
    });

    this->active_button = new tsl::elm::ListItem("Correction active");
    this->active_button->setClickListener([this](std::uint64_t keys) {
        if (keys & HidNpadButton_A) {
            this->config.active ^= 1;
            this->rc = fizeauSetIsActive(this->config.active);
            this->active_button->setValue(this->config.active ? "Active": "Inactive");
            return true;
        }
        return false;
    });
    this->active_button->setValue(this->config.active ? "Active": "Inactive");

    this->apply_button = new tsl::elm::ListItem("Apply settings");
    this->apply_button->setClickListener([this](std::uint64_t keys) {
        if (keys & HidNpadButton_A) {
            this->rc = this->config.apply();
            return true;
        }
        return false;
    });

    static bool enable_extra_hot_temps = false;
    if ((this->is_day ? this->config.profile.day_settings.temperature : this->config.profile.night_settings.temperature) > D65_TEMP)
        enable_extra_hot_temps = true;

    this->temp_slider = new tsl::elm::TrackBar("");
    this->temp_slider->setProgress(((this->is_day ? this->config.profile.day_settings.temperature : this->config.profile.night_settings.temperature) - MIN_TEMP)
        * 100 / ((enable_extra_hot_temps ? MAX_TEMP : D65_TEMP) - MIN_TEMP));
    this->temp_slider->setClickListener([&, this](std::uint64_t keys) {
        if (keys & HidNpadButton_Y) {
            this->temp_slider->setProgress((DEFAULT_TEMP - MIN_TEMP) * 100 / ((enable_extra_hot_temps ? MAX_TEMP : D65_TEMP) - MIN_TEMP));
            (this->is_day ? this->config.profile.day_settings.temperature : this->config.profile.night_settings.temperature) = DEFAULT_TEMP;
            return true;
        }
        return false;
    });
    this->temp_slider->setValueChangedListener([this](std::uint8_t val) {
        (this->is_day ? this->config.profile.day_settings.temperature : this->config.profile.night_settings.temperature) =
            val * ((enable_extra_hot_temps ? MAX_TEMP : D65_TEMP) - MIN_TEMP) / 100 + MIN_TEMP;
    });

    this->gamma_slider = new tsl::elm::TrackBar("");
    this->gamma_slider->setProgress(((this->is_day ? this->config.profile.day_settings.gamma : this->config.profile.night_settings.gamma) - MIN_GAMMA)
        * 100 / (MAX_GAMMA - MIN_GAMMA));
    this->gamma_slider->setClickListener([this](std::uint64_t keys) {
        if (keys & HidNpadButton_Y) {
            this->gamma_slider->setProgress((DEFAULT_GAMMA - MIN_GAMMA) * 100 / (MAX_GAMMA - MIN_GAMMA));
            (this->is_day ? this->config.profile.day_settings.gamma : this->config.profile.night_settings.gamma) = DEFAULT_GAMMA;
            return true;
        }
        return false;
    });
    this->gamma_slider->setValueChangedListener([this](std::uint8_t val) {
        (this->is_day ? this->config.profile.day_settings.gamma : this->config.profile.night_settings.gamma) =
            val * (MAX_GAMMA - MIN_GAMMA) / 100 + MIN_GAMMA;
    });

    this->sat_slider = new tsl::elm::TrackBar("");
    this->sat_slider->setProgress(((this->is_day ? this->config.profile.day_settings.saturation : this->config.profile.night_settings.saturation) - MIN_SAT)
        * 100 / (MAX_SAT - MIN_SAT));
    this->sat_slider->setClickListener([this](std::uint64_t keys) {
        if (keys & HidNpadButton_Y) {
            this->sat_slider->setProgress((DEFAULT_SAT - MIN_SAT) * 100 / (MAX_SAT - MIN_SAT));
            (this->is_day ? this->config.profile.day_settings.saturation : this->config.profile.night_settings.saturation) = DEFAULT_SAT;
            return true;
        }
        return false;
    });
    this->sat_slider->setValueChangedListener([this](std::uint8_t val) {
        (this->is_day ? this->config.profile.day_settings.saturation : this->config.profile.night_settings.saturation) =
            val * (MAX_SAT - MIN_SAT) / 100 + MIN_SAT;
    });

    this->hue_slider = new tsl::elm::TrackBar("");
    this->hue_slider->setProgress(((this->is_day ? this->config.profile.day_settings.hue : this->config.profile.night_settings.hue) - MIN_HUE)
        * 100 / (MAX_HUE - MIN_HUE));
    this->hue_slider->setClickListener([this](std::uint64_t keys) {
        if (keys & HidNpadButton_Y) {
            this->hue_slider->setProgress((DEFAULT_HUE - MIN_HUE) * 100 / (MAX_HUE - MIN_HUE));
            (this->is_day ? this->config.profile.day_settings.hue : this->config.profile.night_settings.hue) = DEFAULT_HUE;
            return true;
        }
        return false;
    });
    this->hue_slider->setValueChangedListener([this](std::uint8_t val) {
        (this->is_day ? this->config.profile.day_settings.hue : this->config.profile.night_settings.hue) =
            val * (MAX_HUE - MIN_HUE) / 100 + MIN_HUE;
    });

    this->luma_slider = new tsl::elm::TrackBar("");
    this->luma_slider->setProgress(((this->is_day ? this->config.profile.day_settings.luminance : this->config.profile.night_settings.luminance) - MIN_LUMA)
        * 100 / (MAX_LUMA - MIN_LUMA));
    this->luma_slider->setClickListener([this](std::uint64_t keys) {
        if (keys & HidNpadButton_Y) {
            this->luma_slider->setProgress((DEFAULT_LUMA - MIN_LUMA) * 100 / (MAX_LUMA - MIN_LUMA));
            (this->is_day ? this->config.profile.day_settings.luminance : this->config.profile.night_settings.luminance) = DEFAULT_LUMA;
            return true;
        }
        return false;
    });
    this->luma_slider->setValueChangedListener([this](std::uint8_t val) {
        (this->is_day ? this->config.profile.day_settings.luminance : this->config.profile.night_settings.luminance) =
            val * (MAX_LUMA - MIN_LUMA) / 100 + MIN_LUMA;
    });

    this->filter_bar = new tsl::elm::NamedStepTrackBar("", { "None", "Red", "Green", "Blue" });
    this->filter_bar->setProgress(static_cast<u8>(this->is_day ? this->config.profile.day_settings.filter : this->config.profile.night_settings.filter));
    this->filter_bar->setClickListener([this](std::uint64_t keys) {
        if (keys & HidNpadButton_Y) {
            this->filter_bar->setProgress(0);
            (this->is_day ? this->config.profile.day_settings.filter : this->config.profile.night_settings.filter) = ColorFilter_None;
            return true;
        }
        return false;
    });
    this->filter_bar->setValueChangedListener([this](u8 val) {
        (this->is_day ? this->config.profile.day_settings.filter : this->config.profile.night_settings.filter) = static_cast<ColorFilter>(val);
    });

    this->range_button = new tsl::elm::ListItem("Color range");
    this->range_button->setClickListener([this](std::uint64_t keys) {
        if (keys & HidNpadButton_A) {
            auto &range = (this->is_day ? this->config.profile.day_settings.range : this->config.profile.night_settings.range);
            if (is_full(range))
                range = DEFAULT_LIMITED_RANGE;
            else
                range = DEFAULT_RANGE;
            this->range_button->setValue(is_full(range) ? "Full" : "Limited");
            return true;
        }
        return false;
    });
    this->range_button->setValue(is_full(this->is_day ? this->config.profile.day_settings.range : this->config.profile.night_settings.range) ? "Full" : "Limited");

    this->temp_header       = new tsl::elm::CategoryHeader("");
    this->filter_header     = new tsl::elm::CategoryHeader("Filter");
    this->gamma_header      = new tsl::elm::CategoryHeader("");
    this->sat_header        = new tsl::elm::CategoryHeader("");
    this->hue_header        = new tsl::elm::CategoryHeader("");
    this->luma_header       = new tsl::elm::CategoryHeader("");

    auto *frame = new tsl::elm::OverlayFrame("Fizeau", VERSION "-" COMMIT);
    auto *list = new tsl::elm::List();

    list->addItem(this->info_header, 60);
    list->addItem(this->active_button);
    list->addItem(this->apply_button);
    list->addItem(this->temp_header);
    list->addItem(this->temp_slider);
    list->addItem(this->sat_header);
    list->addItem(this->sat_slider);
    list->addItem(this->hue_header);
    list->addItem(this->hue_slider);

    list->addItem(this->gamma_header);
    list->addItem(this->gamma_slider);
    list->addItem(this->luma_header);
    list->addItem(this->luma_slider);
    list->addItem(this->filter_header);
    list->addItem(this->filter_bar);
    list->addItem(this->range_button);
    frame->setContent(list);
    return frame;
}

void FizeauOverlayGui::update() {
    if (R_FAILED(this->rc))
        tsl::changeTo<ErrorGui>(this->rc);

    this->is_day = Clock::is_in_interval(this->config.profile.dawn_begin, this->config.profile.dusk_begin);

    this->temp_header->setText(format("Temperature: %u°K",
        this->is_day ? this->config.profile.day_settings.temperature : this->config.profile.night_settings.temperature));
    this->gamma_header->setText(format("Gamma: %.2f",
        this->is_day ? this->config.profile.day_settings.gamma       : this->config.profile.night_settings.gamma));
    this->sat_header->setText(format("Saturation: %.2f",
        this->is_day ? this->config.profile.day_settings.saturation  : this->config.profile.night_settings.saturation));
    this->hue_header->setText(format("Hue: %.2f",
        this->is_day ? this->config.profile.day_settings.hue         : this->config.profile.night_settings.hue));
    this->luma_header->setText(format("Luminance: %.2f",
        this->is_day ? this->config.profile.day_settings.luminance   : this->config.profile.night_settings.luminance));
}

} // namespace fz
