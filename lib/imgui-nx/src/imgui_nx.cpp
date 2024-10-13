// ftpd is a server implementation based on the following:
// - RFC  959 (https://tools.ietf.org/html/rfc959)
// - RFC 3659 (https://tools.ietf.org/html/rfc3659)
// - suggested implementation details from https://cr.yp.to/ftp/filesystem.html
//
// The MIT License (MIT)
//
// Copyright (C) 2020 Michael Theall
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "imgui_nx.h"
#include <imgui.h>

#include <cstring>
#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <string>
#include <utility>
#include <switch.h>

using namespace std::chrono_literals;

namespace {

std::chrono::steady_clock::time_point s_lastMouseUpdate;

float s_width  = 1280.0f;
float s_height = 720.0f;

ImVec2 s_mousePos = ImVec2(0.0f, 0.0f);

AppletHookCookie s_appletHookCookie;

PadState s_pad;

void handleAppletHook(AppletHookType type, void *param) {
    if (type != AppletHookType_OnOperationMode)
        return;

    switch (appletGetOperationMode()) {
        default:
        case AppletOperationMode_Handheld:
            // use handheld mode resolution (720p) and scale
            s_width  = 1280.0f, s_height = 720.0f;
            ImGui::GetStyle().ScaleAllSizes(1.9f / 2.6f);
            ImGui::GetIO().FontGlobalScale = 0.9f;
            break;

        case AppletOperationMode_Console:
            // use docked mode resolution (1080p) and scale
            s_width  = 1920.0f, s_height = 1080.0f;
            ImGui::GetStyle().ScaleAllSizes(2.6f / 1.9f);
            ImGui::GetIO().FontGlobalScale = 1.4f;
            break;
    }
}

void updateTouch(ImGuiIO &io_) {
    // read touch positions
    HidTouchScreenState state = {0};
    auto count = hidGetTouchScreenStates(&state, 1);
    if (count < 1 || state.count < 1) {
        io_.MouseDown[0] = false;
        return;
    }

    // set mouse position to touch point
    s_mousePos = ImVec2(state.touches[0].x, state.touches[0].y);
    io_.MouseDown[0] = true;
}

/// \brief Update gamepad inputs
/// \param io_ ImGui IO
void updateGamepads (ImGuiIO &io_)
{
    // clear navigation inputs
    std::memset (io_.NavInputs, 0, sizeof (io_.NavInputs));

    constexpr static std::array buttonMapping = {
        std::make_pair (HidNpadButton_A,     ImGuiKey_GamepadFaceDown),
        std::make_pair (HidNpadButton_B,     ImGuiKey_GamepadFaceRight),
        std::make_pair (HidNpadButton_X,     ImGuiKey_GamepadFaceUp),
        std::make_pair (HidNpadButton_Y,     ImGuiKey_GamepadFaceLeft),
        std::make_pair (HidNpadButton_L,     ImGuiKey_GamepadL1),
        std::make_pair (HidNpadButton_R,     ImGuiKey_GamepadR1),
        std::make_pair (HidNpadButton_ZL,    ImGuiKey_GamepadL2),
        std::make_pair (HidNpadButton_ZR,    ImGuiKey_GamepadR2),
        std::make_pair (HidNpadButton_Up,    ImGuiKey_GamepadDpadUp),
        std::make_pair (HidNpadButton_Right, ImGuiKey_GamepadDpadRight),
        std::make_pair (HidNpadButton_Down,  ImGuiKey_GamepadDpadDown),
        std::make_pair (HidNpadButton_Left,  ImGuiKey_GamepadDpadLeft),
        std::make_pair (HidNpadButton_Plus,  ImGuiKey_GamepadStart),
        std::make_pair (HidNpadButton_Minus, ImGuiKey_GamepadBack),
    };

    padUpdate (&s_pad);

    // read buttons from primary controller
    auto const keys = padGetButtons (&s_pad);
    for (auto const &[in, out] : buttonMapping)
        io_.AddKeyEvent (out, !!(keys & in));

    // update joystick
    auto const jsLeft = padGetStickPos (&s_pad, 0), jsRight = padGetStickPos (&s_pad, 1);
    constexpr static auto thumb_dead_zone = 10000;
    const std::array analogMapping = {
        std::make_tuple (std::ref (jsLeft.x),  ImGuiKey_GamepadLStickLeft,  -thumb_dead_zone, JOYSTICK_MIN),
        std::make_tuple (std::ref (jsLeft.x),  ImGuiKey_GamepadLStickRight, +thumb_dead_zone, JOYSTICK_MAX),
        std::make_tuple (std::ref (jsLeft.y),  ImGuiKey_GamepadLStickUp,    +thumb_dead_zone, JOYSTICK_MAX),
        std::make_tuple (std::ref (jsLeft.y),  ImGuiKey_GamepadLStickDown,  -thumb_dead_zone, JOYSTICK_MIN),
        std::make_tuple (std::ref (jsRight.x), ImGuiKey_GamepadRStickLeft,  -thumb_dead_zone, JOYSTICK_MIN),
        std::make_tuple (std::ref (jsRight.x), ImGuiKey_GamepadRStickRight, +thumb_dead_zone, JOYSTICK_MAX),
        std::make_tuple (std::ref (jsRight.y), ImGuiKey_GamepadRStickUp,    +thumb_dead_zone, JOYSTICK_MAX),
        std::make_tuple (std::ref (jsRight.y), ImGuiKey_GamepadRStickDown,  -thumb_dead_zone, JOYSTICK_MIN),
    };

    // read right joystick from primary controller
    for (auto const &[in, out, min, max] : analogMapping)
    {
        auto const value = static_cast<float>(in - min) / static_cast<float>(max - min);
        io_.AddKeyAnalogEvent (out, value > 0.1f, std::clamp(value, 0.0f, 1.0f));
    }
}

} // namespace

bool ImGui::nx::init() {
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&s_pad);

    auto &io = ImGui::GetIO();

    // Load nintendo font
    PlFontData standard, extended;
    static ImWchar extended_range[] = {0xe000, 0xe152};
    if (R_SUCCEEDED(plGetSharedFontByType(&standard,     PlSharedFontType_Standard)) &&
            R_SUCCEEDED(plGetSharedFontByType(&extended, PlSharedFontType_NintendoExt))) {
        std::uint8_t *px;
        int w, h, bpp;
        ImFontConfig font_cfg;

        font_cfg.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 20.0f, &font_cfg, io.Fonts->GetGlyphRangesDefault());
        font_cfg.MergeMode            = true;
        io.Fonts->AddFontFromMemoryTTF(extended.address, extended.size, 20.0f, &font_cfg, extended_range);

        // build font atlas
        io.Fonts->GetTexDataAsAlpha8(&px, &w, &h, &bpp);
        io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
        io.Fonts->Build();
    }

    auto &style = ImGui::GetStyle();
    auto mode = appletGetOperationMode();
    if (mode == AppletOperationMode_Handheld) {
        s_width  = 1280.0f, s_height = 720.0f;
        style.ScaleAllSizes(1.9f);
        io.FontGlobalScale = 0.9f;
    } else {
        s_width  = 1920.0f, s_height = 1080.0f;
        style.ScaleAllSizes(2.6f);
        io.FontGlobalScale = 1.4f;
    }

    // initialize applet hooks
    appletHook(&s_appletHookCookie, handleAppletHook, nullptr);

    // disable imgui.ini file
    io.IniFilename = nullptr;

    // setup config flags
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    // disable mouse cursor
    io.MouseDrawCursor = false;

    return true;
}

std::uint64_t ImGui::nx::newFrame() {
    auto &io = ImGui::GetIO();

    // setup display metrics
    io.DisplaySize             = ImVec2(s_width, s_height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // time step
    static auto const start = std::chrono::steady_clock::now();
    static auto prev        = start;
    auto const now          = std::chrono::steady_clock::now();

    io.DeltaTime = std::chrono::duration<float> (now - prev).count();
    prev         = now;

    // update inputs
    updateTouch(io);
    updateGamepads(io);

    // clamp mouse to screen
    s_mousePos.x = std::clamp(s_mousePos.x, 0.0f, s_width);
    s_mousePos.y = std::clamp(s_mousePos.y, 0.0f, s_height);
    io.MousePos  = s_mousePos;

    return padGetButtonsDown(&s_pad);
}

void ImGui::nx::exit() {
    // deinitialize applet hooks
    appletUnhook(&s_appletHookCookie);
}
