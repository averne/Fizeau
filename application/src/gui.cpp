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

#include <algorithm>
#include <bit>
#include <tuple>
#include <imgui.h>
#include <imgui_deko3d.h>
#include <common.hpp>

#include "swkbd.hpp"

#include "gui.hpp"

namespace fz::gui {

namespace  {

constexpr std::array profile_names = {
    "Profile 1",
    "Profile 2",
    "Profile 3",
    "Profile 4",
};

constexpr std::array filters_names = {
    "None",
    "Red",
    "Green",
    "Blue",
};

} // namespace

void init() {
    auto &style = im::GetStyle();
    style.WindowRounding = 0.0f;
}

void exit() { }

namespace {

template <typename T, typename ...Args>
bool new_slider(auto name, auto label, T &val, T min, T max, auto fmt, Args &&...args) {
    auto [width, height] = im::GetIO().DisplaySize;
    auto slider_pos = im::GetWindowPos().x + 0.04f * width;
    bool ret = false;
    im::PushItemWidth(im::GetWindowWidth() - 0.08f * width);
    FZ_SCOPEGUARD([] { im::PopItemWidth(); });
    im::TextUnformatted(name); im::SameLine(); im::SetCursorPosX(slider_pos);
    if constexpr (std::is_floating_point_v<T>) {
        ret |= im::SliderFloat(label, reinterpret_cast<float *>(&val), min, max, fmt);
        ret |= swkbd::handle(label, &val, min, max, fmt, args...);
    } else {
        ret |= im::SliderInt(label, reinterpret_cast<int *>(&val), min, max, fmt);
        ret |= swkbd::handle(label, &val, min, max);
    }
    return ret;
};

bool new_times(auto name, auto labelh, auto labelm, Time &t) {
    auto [width, height] = im::GetIO().DisplaySize;
    im::PushItemWidth(im::GetWindowWidth() * 0.2f);
    FZ_SCOPEGUARD([] { im::PopItemWidth(); });

    bool ret = false;
    im::TextUnformatted(name);
    im::SameLine(); im::SetCursorPosX(0.08f * width);
    int int_h = t.h, int_m = t.m;
    ret |= im::DragInt(labelh, &int_h, 0.05f, 0, 23, "%02dh");
    ret |= swkbd::handle(labelh, &int_h, 0, 23);
    im::SameLine();
    ret |= im::DragInt(labelm, &int_m, 0.05f, 0, 59, "%02dm");
    ret |= swkbd::handle(labelm, &int_m, 0, 59);
    t.h = static_cast<std::uint8_t>(int_h), t.m = static_cast<std::uint8_t>(int_m);

    return ret;
};

bool new_range(auto name, auto labelcb, auto labello, auto labelhi, ColorRange &range) {
    auto [width, height] = im::GetIO().DisplaySize;
    im::PushItemWidth(im::GetWindowWidth() * 0.2f);
    FZ_SCOPEGUARD([] { im::PopItemWidth(); });

    bool is_full_range = (range.lo == MIN_RANGE) && (range.hi == MAX_RANGE);
    bool ret = false;
    im::TextUnformatted(name);
    im::SameLine(); im::SetCursorPosX(0.08f * width);
    if (im::Checkbox(labelcb, &is_full_range)) {
        ret = true;
        if (is_full_range)
            range = DEFAULT_RANGE;
        else
            range = DEFAULT_LIMITED_RANGE;
    }
    im::SetCursorPosX(0.03f * width);
    im::TextUnformatted("Min:"); im::SameLine();
    im::SetCursorPosX(0.08f * width);
    ret |= im::DragFloat(labello, &range.lo, 0.005f, 0.0f, range.hi, "%.02f");
    ret |= swkbd::handle(labello, &range.lo, 0.0f, range.hi, "%.02f");
    im::SameLine();
    im::TextUnformatted("Max:"); im::SameLine();
    ret |= im::DragFloat(labelhi, &range.hi, 0.005f, range.lo, 1.0f, "%.02f");
    ret |= swkbd::handle(labelhi, &range.hi, range.lo, 1.0f, "%.02f");
    return ret;
}

void new_plot(float *data, std::size_t data_size, const ImVec2 &scale, const ImVec4 &color, const ImRect &bb) {
    ImGuiWindow* window = im::GetCurrentWindow();
    if (window->SkipItems)
        return;

    const float scale_min = scale.x, scale_max = scale.y;

    const int values_count_min = 2;
    if (data_size < values_count_min)
        return;

    int res_w = ImMin((int)(bb.Max.x - bb.Min.x), (int)data_size) - 1;
    int item_count = data_size - 1;

    const float t_step = 1.0f / (float)res_w;
    const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));

    float v0 = data[0];
    float t0 = 0.0f;
    ImVec2 tp0 = ImVec2( t0, 1.0f - ImSaturate((v0 - scale_min) * inv_scale) ); // Point in the normalized space of our target rectangle

    const ImU32 col_base = IM_COL32(color.x, color.y, color.z, color.w);

    for (int n = 0; n < res_w; n++) {
        const float t1 = t0 + t_step;
        const int v1_idx = (int)(t0 * item_count + 0.5f);
        const float v1 = data[v1_idx + 1];
        const ImVec2 tp1 = ImVec2( t1, 1.0f - ImSaturate((v1 - scale_min) * inv_scale) );

        // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
        ImVec2 pos0 = ImLerp(bb.Min, bb.Max, tp0);
        ImVec2 pos1 = ImLerp(bb.Min, bb.Max, tp1);
        window->DrawList->AddLine(pos0, pos1, col_base);

        t0 = t1;
        tp0 = tp1;
    }
}

Result draw_profile_tab(Config &ctx) {
    if (!im::BeginTabItem("Profile"))
        return 0;

    im::TextUnformatted("Active handheld profile:");
    if (im::Combo("##activeintp", reinterpret_cast<int *>(&ctx.internal_profile), profile_names.data(), profile_names.size())) {
        if (auto rc = fizeauSetActiveProfileId(false, ctx.internal_profile); R_FAILED(rc))
            return rc;
    }

    im::TextUnformatted("Active docked profile:");
    if (im::Combo("##activeextp", reinterpret_cast<int *>(&ctx.external_profile), profile_names.data(), profile_names.size())) {
        if (auto rc = fizeauSetActiveProfileId(true, ctx.external_profile); R_FAILED(rc))
            return rc;
    }

    im::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 3.0f);
    im::TextUnformatted("Currently editing profile:");
    if (im::Combo("##editp", reinterpret_cast<int *>(&ctx.cur_profile_id), profile_names.data(), profile_names.size())) {
        if (auto rc = ctx.open_profile(ctx.cur_profile_id); R_FAILED(rc))
            return rc;
    }

    if (im::Button("Apply")) {
        if (auto rc = ctx.apply(); R_FAILED(rc))
            return rc;
        ctx.is_editing_day_profile = ctx.is_editing_night_profile = false;
    }

    im::SameLine();
    if (im::Button("Reset")) {
        ctx.reset();
        ctx.is_editing_day_profile = ctx.is_editing_night_profile = false;
    }

    im::EndTabItem();
    return 0;
}

Result draw_color_tab(Config &ctx) {
    if (!im::BeginTabItem("Colors"))
        return 0;

    auto [width, height] = im::GetIO().DisplaySize;

    static bool enable_extra_hot_temps = false;
    if ((ctx.profile.day_settings.temperature > D65_TEMP) ||(ctx.profile.night_settings.temperature > D65_TEMP))
        enable_extra_hot_temps = true;

    // Temperature sliders
    im::SeparatorText("Temperature");
    auto max_temp = enable_extra_hot_temps ? MAX_TEMP : D65_TEMP;
    ctx.is_editing_day_profile   |= new_slider("Day:",   "##tempd", ctx.profile.day_settings  .temperature, MIN_TEMP, max_temp, "%d°K");
    ctx.is_editing_night_profile |= new_slider("Night:", "##tempn", ctx.profile.night_settings.temperature, MIN_TEMP, max_temp, "%d°K");
    im::Checkbox("Enable blue temperatures", &enable_extra_hot_temps);

    // Saturation sliders
    im::SeparatorText("Saturation");
    ctx.is_editing_day_profile   |= new_slider("Day:",   "##satd", ctx.profile.day_settings  .saturation, MIN_SAT, MAX_SAT, "%.2f");
    ctx.is_editing_night_profile |= new_slider("Night:", "##satn", ctx.profile.night_settings.saturation, MIN_SAT, MAX_SAT, "%.2f");

    // Hue sliders
    im::SeparatorText("Hue");
    ctx.is_editing_day_profile   |= new_slider("Day:",   "##hued", ctx.profile.day_settings  .hue, MIN_HUE, MAX_HUE, "%.2f");
    ctx.is_editing_night_profile |= new_slider("Night:", "##huen", ctx.profile.night_settings.hue, MIN_HUE, MAX_HUE, "%.2f");

    // Components checkboxes
    im::SeparatorText("Channels");

    auto items_pos = im::GetWindowPos().x + 0.09f * width;

    std::uint32_t c = ctx.profile.components;
    im::TextUnformatted("Components:");  im::SameLine(); im::SetCursorPosX(items_pos);
    im::CheckboxFlags("Red##compr",   &c, Component_Red);   im::SameLine();
    im::CheckboxFlags("Green##compg", &c, Component_Green); im::SameLine();
    im::CheckboxFlags("Blue##compb",  &c, Component_Blue);
    ctx.profile.components = static_cast<Component>(c);

    int filter = (ctx.profile.filter == Component_None) ? 0 : std::countr_zero(static_cast<std::uint32_t>(ctx.profile.filter)) + 1;
    im::TextUnformatted("Filter:"); im::SameLine(); im::SetCursorPosX(items_pos);
    im::SetNextItemWidth(0.2f * width);
    if (im::Combo("##filter", &filter, filters_names.data(), filters_names.size()))
        ctx.profile.filter = static_cast<Component>(filter ? BIT(filter - 1) : filter);

    im::EndTabItem();
    return 0;
}

Result draw_correction_tab(Config &ctx) {
    if (!im::BeginTabItem("Correction"))
        return 0;

    // Contrast sliders
    im::SeparatorText("Contrast");
    ctx.is_editing_day_profile   |= new_slider("Day:",   "##contrastd", ctx.profile.day_settings  .contrast, MIN_CONTRAST, MAX_CONTRAST, "%.2f");
    ctx.is_editing_night_profile |= new_slider("Night:", "##contrastn", ctx.profile.night_settings.contrast, MIN_CONTRAST, MAX_CONTRAST, "%.2f");

    // Gamma sliders
    im::SeparatorText("Gamma");
    ctx.is_editing_day_profile   |= new_slider("Day:",   "##gammad", ctx.profile.day_settings  .gamma, MIN_GAMMA, MAX_GAMMA, "%.2f");
    ctx.is_editing_night_profile |= new_slider("Night:", "##gamman", ctx.profile.night_settings.gamma, MIN_GAMMA, MAX_GAMMA, "%.2f");

    // Luminance sliders
    im::SeparatorText("Luminance");
    ctx.is_editing_day_profile   |= new_slider("Day:",   "##lumad", ctx.profile.day_settings  .luminance, MIN_LUMA, MAX_LUMA, "%.2f", true);
    ctx.is_editing_night_profile |= new_slider("Night:", "##luman", ctx.profile.night_settings.luminance, MIN_LUMA, MAX_LUMA, "%.2f", true);

    // Color range sliders
    im::SeparatorText("Color range");
    ctx.is_editing_day_profile   |= new_range("Day:",   "Full range##d", "##rangeld", "##ranghd", ctx.profile.day_settings  .range);
    ctx.is_editing_night_profile |= new_range("Night:", "Full range##n", "##rangeln", "##ranghn", ctx.profile.night_settings.range);

    im::EndTabItem();
    return 0;
}

Result draw_time_tab(Config &ctx) {
    if (!im::BeginTabItem("Time"))
        return 0;

    auto [width, height] = im::GetIO().DisplaySize;

    bool has_changed = false;

    // Dusk
    im::SeparatorText("Dusk");
    has_changed |= new_times("Start:", "##dush", "##dusm", ctx.profile.dusk_begin);
    has_changed |= new_times("End:",   "##dueh", "##duem", ctx.profile.dusk_end);

    if (ctx.profile.dusk_end >= ctx.profile.dusk_begin) {
        auto diff = ctx.profile.dusk_end - ctx.profile.dusk_begin;
        im::Text("Dusk will begin at %02u:%02u and last for %02uh%02um", ctx.profile.dusk_begin.h, ctx.profile.dusk_begin.m, diff.h, diff.m);
    } else {
        im::TextColored({ 1.00f, 0.33f, 0.33f, 1.0f }, "Invalid dusk transition times!");
    }

    // Dawn
    im::SeparatorText("Dawn");
    has_changed |= new_times("Start:", "##dash", "##dasm", ctx.profile.dawn_begin);
    has_changed |= new_times("End:",   "##daeh", "##daem", ctx.profile.dawn_end);

    if (ctx.profile.dawn_end >= ctx.profile.dawn_begin) {
        auto diff = ctx.profile.dawn_end - ctx.profile.dawn_begin;
        im::Text("Dawn will begin at %02u:%02u and last for %02uh%02um", ctx.profile.dawn_begin.h, ctx.profile.dawn_begin.m, diff.h, diff.m);
    } else {
        im::TextColored({ 1.00f, 0.33f, 0.33f, 1.0f }, "Invalid dawn transition times!");
    }

    if (has_changed)
        ctx.has_active_override = false;

    // Dimming timeout
    {
        im::SeparatorText("Dimming timeout");
        im::PushItemWidth(im::GetWindowWidth() * 0.2f);
        FZ_SCOPEGUARD([] { im::PopItemWidth(); });

        im::TextUnformatted("Timeout:");

        im::SameLine(); im::SetCursorPosX(0.08f * width);
        int int_m = ctx.profile.dimming_timeout.m, int_s = ctx.profile.dimming_timeout.s;
        im::DragInt("##dimm", &int_m, 0.05f, 0, 59, "%02dm");
        swkbd::handle("##dimm", &int_m, 0, 59);
        im::SameLine();
        im::DragInt("##dims", &int_s, 0.05f, 0, 59, "%02ds");
        swkbd::handle("##dims", &int_s, 0, 59);

        im::TextUnformatted("Set to 0 to use the system setting");

        ctx.profile.dimming_timeout.m = static_cast<std::uint8_t>(int_m), ctx.profile.dimming_timeout.s = static_cast<std::uint8_t>(int_s);
    }

    im::EndTabItem();
    return 0;
}

Result draw_help_tab(Config &ctx) {
    if (!im::BeginTabItem("Help"))
        return 0;

    if (im::CollapsingHeader("Usage")) {
        im::BulletText("\ue121 Touchscreen:\n"
            "Drag the sliders to set the values to your liking.\n");
        im::BulletText("\ue122 Controller:\n"
            "Use the \ue0aa keypad to navigate the window.\n"
            "Use \ue000 to select an item, then \ue0b1 \ue0b2 to modify\n  its value.\n"
            "Use \ue001 to deselect an item.\n"
            "Use \ue002 to input a value using the keyboard.\n"
            "Use \ue0b3 to exit (\ue0b9 is not recommended).\n");
    }

    if (im::CollapsingHeader("Documentation")) {
        im::BulletText(
R"(The temperature slider adjusts the color
temperature of the screen. Use this as a night
color feature.)");
        im::BulletText(
R"(The filter feature can be used to restrict the color
displayed to one single component (red, green,
or blue).)");
        im::BulletText(
R"(The gamma slider adjusts the regamma ramp.
This can be useful on bad/old monitors.)");
        im::BulletText(
R"(The luminance slider can increase/decrease the
perceived luminance. This is different from the
backlight brightness, but can be used along with
it to lower the luminosity below normal limits.)");
        im::BulletText(
R"(The color range pickers adjust the intensity
range of color components. This can be useful
on certain monitors. Unticking "Full range" is
equivalent to using the "Limited range" official
setting.)");
    }

    im::TextUnformatted("Compiled on " __DATE__ " " __TIME__);

    im::EndTabItem();
    return 0;
}

} // namespace

void draw_background(Config &ctx, DkResHandle background_handle) {
    im::GetBackgroundDrawList()->AddImage(im::deko3d::makeTextureID(background_handle), { 0, 0 }, im::GetIO().DisplaySize);
}

Result draw_main_window(Config &ctx) {
    if (!im::Begin("Fizeau, version " VERSION "-" COMMIT, nullptr, ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
        return 0;
    FZ_SCOPEGUARD([] { im::End(); });

    auto [width, height] = im::GetIO().DisplaySize;
    im::SetWindowPos( { 0.03f * width, 0.05f * height }, ImGuiCond_Always);
    im::SetWindowSize({ 0.40f * width, 0.90f * height }, ImGuiCond_Always);

    auto *imctx = im::GetCurrentContext();
    if (!imctx->NavWindow)
        im::SetWindowFocus();

    // Leave edit field, or it will pop the swkbd again
    im::GetCurrentContext()->TempInputId = 0;

    // Active checkbox
    if (im::Checkbox("Correction active", &ctx.active)) {
        if (auto rc = fizeauSetIsActive(ctx.active); R_FAILED(rc))
            return rc;
        ctx.has_active_override = true;
    }

    // Time & FPS
    auto time = Clock::get_current_time();
    im::SameLine();
    im::SetCursorPosX(im::GetCursorPosX() + 12.0f);
    im::Text("Time: %02d:%02d:%02d - Fps: %.2f", time.h, time.m, time.s, im::GetIO().Framerate);

    im::BeginTabBar("##tab_bar", ImGuiTabBarFlags_NoTooltip);
    FZ_SCOPEGUARD([] { im::EndTabBar(); });

    if (auto rc = draw_profile_tab(ctx); R_FAILED(rc))
        return rc;

    if (auto rc = draw_color_tab(ctx); R_FAILED(rc))
        return rc;

    if (auto rc = draw_correction_tab(ctx); R_FAILED(rc))
        return rc;

    if (auto rc = draw_time_tab(ctx); R_FAILED(rc))
        return rc;

    if (auto rc = draw_help_tab(ctx); R_FAILED(rc))
        return rc;

    return 0;
}

void draw_preview_window(Config &ctx, DkResHandle preview_ref_handle, DkResHandle preview_mat_handle) {
    std::array<char, 0x40> buf;
    std::snprintf(buf.data(), buf.size(), "Preview (%s)###preview",
        ctx.is_editing_day_profile ? "day" : ctx.is_editing_night_profile ? "night" : "none");

    if (!im::Begin(buf.data(), nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
        return;
    FZ_SCOPEGUARD([] { im::End(); });

    auto [width, height] = im::GetIO().DisplaySize;
    im::SetWindowPos( { 0.47f * width, 0.05f * height }, ImGuiCond_Always);
    im::SetWindowSize({ 0.50f * width, 0.50f * height }, ImGuiCond_Always);

    if (preview_ref_handle == static_cast<DkResHandle>(-1) || preview_mat_handle == static_cast<DkResHandle>(-1))
        return;

    im::Image(im::deko3d::makeTextureID(preview_ref_handle), { 0.23f * width, 0.23f * width });
    im::SameLine();
    im::Image(im::deko3d::makeTextureID(preview_mat_handle), { 0.23f * width, 0.23f * width });
}

void draw_graph_window(Config &ctx) {
    std::array<char, 0x40> buf;
    std::snprintf(buf.data(), buf.size(), "Gamma ramps (%s)###gammaramp",
        ctx.is_editing_day_profile ? "day" : ctx.is_editing_night_profile ? "night" : "none");

    if (!im::Begin(buf.data(), nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
        return;
    FZ_SCOPEGUARD([] { im::End(); });

    auto [width, height] = im::GetIO().DisplaySize;
    im::SetWindowPos( { 0.53f * width, 0.60f * height }, ImGuiCond_Always);
    im::SetWindowSize({ 0.38f * width, 0.35f * height }, ImGuiCond_Always);

    FizeauSettings set = ctx.is_editing_day_profile ? ctx.profile.day_settings : ctx.profile.night_settings;

    // Calculate ramps
    std::array<std::uint16_t, 256> lut1;
    std::array<std::uint16_t, 960> lut2;

    float off = (1.0f - set.contrast) / 2.0f;
    degamma_ramp(lut1.data(), lut1.size(), DEFAULT_GAMMA, 8);
    regamma_ramp(lut2.data(), lut2.size(), set.gamma, 8, 0.0f, 1.0f, off);

    apply_luma(lut2.data(), lut2.size(), 8, set.luminance);
    apply_range(lut2.data(), lut2.size(), 8, set.range.lo, std::min(set.range.hi, lut2.back() / 255.0f));

    std::array<float, 2>           linear = { 0, 1 };
    std::array<float, lut1.size()> lut1_float;
    std::array<float, lut2.size()> lut2_float;

    std::transform(lut1.begin(), lut1.end(), lut1_float.begin(), [](std::uint16_t val) { return static_cast<float>(val) / 255.0f; });
    std::transform(lut2.begin(), lut2.end(), lut2_float.begin(), [](std::uint16_t val) { return static_cast<float>(val) / 255.0f; });

    auto &style = im::GetStyle();
    auto *window = im::GetCurrentWindow();

    ImVec2 frame_size = { 0.95f * window->Size.x, 0.75f * window->Size.y };

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    im::ItemSize(frame_bb, style.FramePadding.y);
    if (!im::ItemAdd(frame_bb, 0, &frame_bb))
        return;
    im::RenderFrame(frame_bb.Min, frame_bb.Max, im::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    // Draw grid
    int nb_x_ticks = 7, nb_y_ticks = 5;
    auto grid_col = IM_COL32(127.0f, 127.0f, 127.0f, 200.0f);
    for (int i = 0; i < nb_x_ticks; ++i) {
        float x = ImLerp(inner_bb.Min.x, inner_bb.Max.x, static_cast<float>(i) / static_cast<float>(nb_x_ticks - 1));
        window->DrawList->AddLine({ x, inner_bb.Min.y }, { x, inner_bb.Max.y }, grid_col);
    }
    for (int i = 0; i < nb_x_ticks; ++i) {
        float y = ImLerp(inner_bb.Min.y, inner_bb.Max.y, static_cast<float>(i) / static_cast<float>(nb_y_ticks - 1));
        window->DrawList->AddLine({ inner_bb.Min.x, y }, { inner_bb.Max.x, y }, grid_col);
    }

    // Draw plots
    new_plot(lut1_float.data(), lut1_float.size(), { 0, 1 }, { 255.0f, 128.0f,  56.0f, 255.0f }, inner_bb);
    new_plot(linear.data(),     linear.size(),     { 0, 1 }, {  56.0f, 128.0f, 255.0f, 255.0f }, inner_bb);
    new_plot(lut2_float.data(), lut2_float.size(), { 0, 1 }, {  56.0f, 255.0f, 128.0f, 255.0f }, inner_bb);
}

void draw_error_window(Config &ctx, Result error) {
    if (!im::Begin("Fizeau, version " VERSION "-" COMMIT, nullptr, ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
        return;
    FZ_SCOPEGUARD([] { im::End(); });

    if (error == 1)
        im::Text("The service is not active, check that all files are correctly installed.");
    else
        im::Text("Error: %#x (%04d-%04d)", error, R_MODULE(error) | 2000, R_DESCRIPTION(error));

    im::TextUnformatted(
R"(An error happened.
Please try rebooting your console, and checking your setup:
    - updating Fizeau.
    - updating your CFW.
Only the latest version of the Atmosphère CFW is supported.

You can submit an issue at https://www.github.com/averne/Fizeau.
If you do so, please be as descriptive as possible.)"
    );
}

} // namespace fz::gui
