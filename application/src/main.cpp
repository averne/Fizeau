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

#include <cstdio>
#include <vector>
#include <string>
#include <switch.h>
#include <stratosphere.hpp>
#include <common.hpp>

#include "fizeau.h"
#include "config.hpp"
#include "gl.hpp"
#include "imgui.hpp"
#include "swkbd.hpp"

static constexpr int width  = 1280;
static constexpr int height = 720;

extern "C" void userAppInit() {
#ifdef DEBUG
    socketInitializeDefault();
    nxlinkStdio();
#endif
    plInitialize();
}

extern "C" void userAppExit(void) {
#ifdef DEBUG
    socketExit();
#endif
    SERV_EXIT(pl);
}

int main(int argc, char **argv) {
    Result rc;

    LOG("Starting Fizeau\n");

    auto *window = fz::gl::init_glfw(width, height);
    if (!window || R_FAILED(fz::gl::init_glad()))
        return 1;
    glViewport(0, 0, width, height);

    fz::imgui::init(window, width, height);

    auto config = fz::read_config(fz::config_path);

    TRY_GOTO(rc = fizeauInitialize(), error);

    // Restore state after reboot
    bool tmp_active;
    Time tmp_dawn, tmp_dusk;
    TRY_GOTO(rc = fizeauGetIsActive(&tmp_active),              error);
    TRY_GOTO(rc = fizeauGetDawnTime(&tmp_dawn),                error);
    TRY_GOTO(rc = fizeauGetDuskTime(&tmp_dusk),                error);
    if (!tmp_active && (tmp_dawn == Time{0, 0, 0}) && (tmp_dusk == Time{0, 0, 0})) {
        TRY_GOTO(rc = fizeauSetIsActive(config.active),        error);
        TRY_GOTO(rc = fizeauSetDuskTime(config.dusk),          error);
        TRY_GOTO(rc = fizeauSetDawnTime(config.dawn),          error);
        if (config.has_active_override) // Reapply override
            TRY_GOTO(rc = fizeauSetIsActive(config.active),    error);
        TRY_GOTO(rc = fizeauSetColor(config.color),            error);
    }
    TRY_GOTO(rc = fizeauSetBrightness(config.brightness),      error);

    while (!glfwWindowShouldClose(window)) {
        // Update state
        TRY_GOTO(rc = fizeauGetIsActive(&config.active),       error);
        TRY_GOTO(rc = fizeauGetDuskTime(&config.dusk),         error);
        TRY_GOTO(rc = fizeauGetDawnTime(&config.dawn),         error);
        TRY_GOTO(rc = fizeauGetColor(&config.color.rgba),      error);
        TRY_GOTO(rc = fizeauGetBrightness(&config.brightness), error);

        // New frame
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_NX_KEY_PLUS))
            break;
        glClearColor(0.18f, 0.2f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        constexpr std::uint64_t easter_egg_combo = KEY_ZR | KEY_ZL;
        if (((hidKeysDown(CONTROLLER_P1_AUTO) | hidKeysHeld(CONTROLLER_P1_AUTO)) & easter_egg_combo) == easter_egg_combo)
            TRY_GOTO(rc = fizeauEasterEgg(), error);

        // Leave edit field, or it will pop the swkbd again
        ImGui::GetCurrentContext()->TempInputTextId = 0;

        fz::imgui::begin_frame(window);
        fz::imgui::draw_controls_window();
        fz::imgui::draw_help_window();

        // Main window
        bool has_changed;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {10.0f, 10.0f});
        if (ImGui::Begin("Fizeau, version " VERSION "-" COMMIT, nullptr, ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove)) {

            ImGui::SetWindowPos({300.0f, 120.0f}, ImGuiCond_Once);
            ImGui::SetWindowSize({700.0f, 520.0f}, ImGuiCond_Once);

            // Time & FPS
            auto time = fz::get_time();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 180.0f);
            ImGui::Text("Time: %02d:%02d:%02d - Fps: %.2f", time.hour, time.minute, time.second, ImGui::GetIO().Framerate);

            // Active checkbox
            ImGui::Separator();
            if (ImGui::Checkbox(!config.has_active_override ? "Overlay active" : "Overlay active (overridden)", &config.active)) {
                TRY_GOTO(rc = fizeauSetIsActive(config.active), error);
                config.has_active_override = true;
            }

            // Activation/deactivation times
            ImGui::Separator();
            static std::uint8_t min_h = 0, max_h = 23, min_m = 0, max_m = 59;
            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.2f);
            ImGui::TextUnformatted("Hours (drag to set):");
            ImGui::TextUnformatted("Dusk:");
            ImGui::SameLine();
            ImGui::SetCursorPosX(100.0f);
            has_changed  = ImGui::DragScalar("##bh", ImGuiDataType_U8, &config.dusk.h, 0.05f, &min_h, &max_h, "%02dh");
            has_changed |= fz::handle_swkbd( "##bh", fz::swkbd_input_u8, &config.dusk.h, min_h, max_h);
            ImGui::SameLine();
            has_changed |= ImGui::DragScalar("##bm", ImGuiDataType_U8, &config.dusk.m, 0.05f, &min_m, &max_m, "%02dm");
            has_changed |= fz::handle_swkbd( "##bm", fz::swkbd_input_u8, &config.dusk.m, min_m, max_m);
            if (has_changed) {
                TRY_GOTO(rc = fizeauSetDuskTime(config.dusk), error);
                config.has_active_override = false;
            }
            ImGui::TextUnformatted("Dawn:  ");
            ImGui::SameLine();
            ImGui::SetCursorPosX(100.0f);
            has_changed  = ImGui::DragScalar("##eh", ImGuiDataType_U8, &config.dawn.h, 0.05f, &min_h, &max_h, "%02dh");
            has_changed |= fz::handle_swkbd( "##eh", fz::swkbd_input_u8, &config.dawn.h, min_h, max_h);
            ImGui::SameLine();
            has_changed |= ImGui::DragScalar("##em", ImGuiDataType_U8, &config.dawn.m, 0.05f, &min_m, &max_m, "%02dm");
            has_changed |= fz::handle_swkbd( "##em", fz::swkbd_input_u8, &config.dawn.m, min_m, max_m);
            if (has_changed) {
                TRY_GOTO(rc = fizeauSetDawnTime(config.dawn), error);
                config.has_active_override = false;
            }
            ImGui::PopItemWidth();

            // Temperature slider
            ImGui::Separator();
            ImGui::TextUnformatted("Temperature");
            ImGui::PushItemWidth(ImGui::GetWindowWidth() - 50.0f);
            has_changed  = ImGui::SliderFloat("##temp", &config.temp, fz::min_temp, fz::max_temp, "%.1f°K");
            has_changed |= fz::handle_swkbd(  "##temp", fz::swkbd_input_f, &config.temp, fz::min_temp, fz::max_temp);
            if (has_changed) {
                TRY_GOTO(rc = fizeauSetColor(fz::temp_to_col(config.temp, config.color.a).rgba), error);
            }
            ImGui::PopItemWidth();

            // RGBA sliders
            std::uint16_t max = fz::rgba4444_t::member_max - 1; // Clamp to max - 1 to prevent the layer from being completely opaque
            std::array<std::uint16_t, 4> overlay_col = {config.color.r, config.color.g, config.color.b, config.color.a};
            ImGui::Separator();
            ImGui::TextUnformatted("RGBA");
            ImGui::PushItemWidth(-100.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg,         static_cast<ImVec4>(ImColor::HSV(0.0f, 0.5f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,  static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive,   static_cast<ImVec4>(ImColor::HSV(0.0f, 0.7f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab,      static_cast<ImVec4>(ImColor::HSV(0.0f, 0.9f, 0.9f)));
            has_changed  = ImGui::SliderScalar("Red", ImGuiDataType_U16, &overlay_col[0],
                &fz::rgba4444_t::member_min, &max);
            has_changed |= fz::handle_swkbd(   "Red", fz::swkbd_input_u16, &overlay_col[0],
                fz::rgba4444_t::member_min, max);
            ImGui::PopStyleColor(4);
            ImGui::PushStyleColor(ImGuiCol_FrameBg,         static_cast<ImVec4>(ImColor::HSV(0.33f, 0.5f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,  static_cast<ImVec4>(ImColor::HSV(0.33f, 0.6f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive,   static_cast<ImVec4>(ImColor::HSV(0.33f, 0.7f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab,      static_cast<ImVec4>(ImColor::HSV(0.33f, 0.9f, 0.9f)));
            has_changed |= ImGui::SliderScalar("Green", ImGuiDataType_U16, &overlay_col[1],
                &fz::rgba4444_t::member_min, &max);
            has_changed |= fz::handle_swkbd(   "Green", fz::swkbd_input_u16, &overlay_col[1],
                fz::rgba4444_t::member_min, max);
            ImGui::PopStyleColor(4);
            ImGui::PushStyleColor(ImGuiCol_FrameBg,         static_cast<ImVec4>(ImColor::HSV(0.66f, 0.5f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,  static_cast<ImVec4>(ImColor::HSV(0.66f, 0.6f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive,   static_cast<ImVec4>(ImColor::HSV(0.66f, 0.7f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab,      static_cast<ImVec4>(ImColor::HSV(0.66f, 0.9f, 0.9f)));
            has_changed |= ImGui::SliderScalar("Blue", ImGuiDataType_U16, &overlay_col[2],
                &fz::rgba4444_t::member_min, &max);
            has_changed |= fz::handle_swkbd(   "Blue", fz::swkbd_input_u16, &overlay_col[2],
                fz::rgba4444_t::member_min, max);
            ImGui::PopStyleColor(4);
            ImGui::PushStyleColor(ImGuiCol_FrameBg,         static_cast<ImVec4>(ImColor::HSV(0.0f, 0.0f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,  static_cast<ImVec4>(ImColor::HSV(0.0f, 0.0f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive,   static_cast<ImVec4>(ImColor::HSV(0.0f, 0.0f, 0.5f)));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab,      static_cast<ImVec4>(ImColor::HSV(0.0f, 0.0f, 0.9f)));
            has_changed |= ImGui::SliderScalar("Alpha", ImGuiDataType_U16, &overlay_col[3],
                &fz::rgba4444_t::member_min, &max);
            has_changed |= fz::handle_swkbd(   "Alpha", fz::swkbd_input_u16, &overlay_col[3],
                fz::rgba4444_t::member_min, max);
            ImGui::PopStyleColor(4);
            ImGui::PopItemWidth();
            if (has_changed) {
                config.color = overlay_col;
                TRY_GOTO(rc = fizeauSetColor(config.color.rgba), error);
            }

            // Brightness slider
            ImGui::Separator();
            ImGui::TextUnformatted("Brightness");
            ImGui::PushItemWidth(ImGui::GetWindowWidth() - 50.0f);
            has_changed  = ImGui::SliderFloat("##bright", &config.brightness, 0.0f, 1.0f, "%.1f");
            has_changed |= fz::handle_swkbd(  "##bright", fz::swkbd_input_f, &config.brightness, 0.0f, 1.0f);
            if (has_changed)
                TRY_GOTO(rc = fizeauSetBrightness(config.brightness), error);
        }
        ImGui::End();
        ImGui::PopStyleVar();

        fz::imgui::end_frame();

        glfwSwapBuffers(window);
    }

    fz::dump_config(fz::config_path, config);

error:
    while (!glfwWindowShouldClose(window)) {
        // New frame
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_NX_KEY_PLUS))
            break;
        glClearColor(0.18f, 0.2f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (R_SUCCEEDED(rc))
            break;

        fz::imgui::begin_frame(window);
        fz::imgui::draw_error_window(rc);
        fz::imgui::end_frame();

        glfwSwapBuffers(window);
    }

    LOG("Exiting Fizeau\n");
    fizeauExit();
    fz::imgui::exit();
    fz::gl::exit_glfw(window);

    return 0;
}
