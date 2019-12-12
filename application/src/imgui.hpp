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

#include <switch.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.hpp>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

namespace fz::imgui {

namespace impl {

static inline bool is_controls_focused = false, is_help_focused = false;

} // namespace impl


static void init(GLFWwindow *window, int width, int height) {
    ImGui::CreateContext();
    ImGui::GlfwImpl::InitForOpengGl(window);
    ImGui::Gl3Impl::Init("#version 430");
    ImGui::StyleColorsDark();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.BackendPlatformName = "cmw";

    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

    io.KeyMap[ImGuiKey_LeftArrow]   = GLFW_NX_KEY_DLEFT;
    io.KeyMap[ImGuiKey_RightArrow]  = GLFW_NX_KEY_DRIGHT;
    io.KeyMap[ImGuiKey_UpArrow]     = GLFW_NX_KEY_DUP;
    io.KeyMap[ImGuiKey_DownArrow]   = GLFW_NX_KEY_DDOWN;
    io.KeyMap[ImGuiKey_Space]       = GLFW_NX_KEY_A;
    io.KeyMap[ImGuiKey_Enter]       = GLFW_NX_KEY_X;
    io.KeyMap[ImGuiKey_Escape]      = GLFW_NX_KEY_B;
    io.KeyMap[ImGuiKey_Tab]         = GLFW_NX_KEY_R;

    // Load nintendo font
    PlFontData standard, extended;
    static ImWchar extended_range[] = {0xe000, 0xe152};
    if (R_SUCCEEDED(plGetSharedFontByType(&standard, PlSharedFontType_Standard)) &&
            R_SUCCEEDED(plGetSharedFontByType(&extended, PlSharedFontType_NintendoExt))) {
        // Delete previous font atlas
        glDeleteTextures(1, reinterpret_cast<GLuint *>(&io.Fonts->TexID));

        // Load font data & build atlas
        std::uint8_t *px;
        int w, h, bpp;
        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 20.0f, &font_cfg, io.Fonts->GetGlyphRangesDefault());
        // Merge second font (cannot set it before)
        font_cfg.MergeMode            = true;
        io.Fonts->AddFontFromMemoryTTF(extended.address, extended.size, 20.0f, &font_cfg, extended_range);
        io.Fonts->GetTexDataAsAlpha8(&px, &w, &h, &bpp);

        // Upload atlas to VRAM, tell imgui about it
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, px);
        io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(tex));
    }

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ScaleAllSizes(1.5f);
}

static void exit() {
    ImGui::Gl3Impl::Shutdown();
    ImGui::GlfwImpl::Shutdown();
    ImGui::DestroyContext();
}

static void begin_frame(GLFWwindow *window) {
    // Emulate mod keys
    auto &io = ImGui::GetIO();
    if (glfwGetKey(window, GLFW_NX_KEY_L))
        io.KeyCtrl  = true;
    ImGui::Gl3Impl::NewFrame();
    ImGui::GlfwImpl::NewFrame();
    ImGui::NewFrame();
}

static void end_frame() {
    ImGui::Render();
    ImGui::Gl3Impl::RenderDrawData(ImGui::GetDrawData());
}

static void draw_controls_window() {
    if (!impl::is_controls_focused)
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_Always);
    ImGui::SetNextWindowPos({10.0f, 10.0f}, ImGuiCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {10.0f, 10.0f});
    if (ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        impl::is_controls_focused = ImGui::IsWindowFocused();
        ImGui::TextUnformatted(
"\ue121 Touchscreen:\n"
"    - Drag the sliders to set the values to your liking.\n"
"\n"
"\ue122 Controller:\n"
"    - Use the \ue0aa keypad to navigate the window.\n"
"    - Use \ue000 to select an item, then \ue0b1 \ue0b2 to modify its value.\n"
"    - Use \ue001 to deselect an item.\n"
"    - Use \ue002 to input a value using the keyboard.\n"
"    - Use \ue0b3 to exit (\ue0b9 is not recommended).\n"
"    - Use both \ue0a4 and \ue0a5 to switch between windows.\n"
"    - ??? ???? \ue0a6 ??? \ue0a7 ?? ???????? ??? ?????? ???.\n"
        );
    }
    ImGui::PopStyleVar();
    ImGui::End();
}

static void draw_help_window() {
    if (!impl::is_help_focused)
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_Always);
    ImGui::SetNextWindowPos({10.0f, 50.0f}, ImGuiCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {10.0f, 10.0f});
    if (ImGui::Begin("Help", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        impl::is_help_focused = ImGui::IsWindowFocused();
        ImGui::TextUnformatted(
R"(Temperature/color:
    - Use the temperature slider to select the desired shading.
    - You can override the color by using the RGBA sliders.
    - The "Alpha" value controls the opacity of the layer.
    - Due to the way Fizeau works, it is not possible to directly modify the
        screen color. Instead, it creates a transparent layer and applies
        color to it. However, this can lead to an overall increase of luminosity,
        especially visible with dark colors.
    - Additionally, the layer is not visible in screenshots/recordings.

Dawn/dusk:
    - When set, Fizeau will start at the "dusk" time and shutdown at the "dawn"
        time.
    - You can override this by using the checkbox. Fizeau will then stay in the
        specified state.

Settings:
    - Settings are saved at /switch/Fizeau/config.ini, which you can also edit.
    - To reduce the memory usage of the sysmodule, settings are not read
        continually. Instead, they are applied on application launch.
    - Thus, you will need to launch the client after a reboot to restart Fizeau.

Report issues/submit suggestions at: https://www.github.com/averne/Fizeau)"
        );
    }
    ImGui::PopStyleVar();
    ImGui::End();
}

static void draw_error_window(Result error) {
    if (ImGui::Begin("Fizeau, version " VERSION "-" COMMIT, nullptr, ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove)) {
        ImGui::SetWindowPos({300.0f, 200.0f}, ImGuiCond_Once);
        ImGui::Text("Error: %#x (%04d-%04d)", error, R_MODULE(error) + 2000, R_DESCRIPTION(error));
        ImGui::TextUnformatted(
R"(An error happened.
Please try rebooting your console, and checking your setup:
    - updating Fizeau.
    - updating your CFW.
Only the latest version of the Atmosphère CFW is supported.

You can submit an issue at https://www.github.com/averne/Fizeau.
If you do so, please be as descriptive as possible.)"
        );
    }
    ImGui::End();
}

} // namespace fz::imgui
