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
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace fz::gl {

static inline GLFWwindow *init_glfw(int width, int height) {
    if (!glfwInit())
        return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    auto *window = glfwCreateWindow(width, height, "", NULL, NULL);

    if (window) {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync
    }

    return window;
}

static inline void exit_glfw(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

static inline Result init_glad() {
    return !gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
}

} // namespace fz::gl
