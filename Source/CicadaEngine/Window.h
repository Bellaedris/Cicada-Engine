//
// Created by Bellaedris on 11/12/2025.
//

#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace cica
{
    class Window
    {
    private:
        #pragma region Callbacks
        static void FramebufferSizeCallback(GLFWwindow *w, int width, int height);
        #pragma endregion Callbacks

        #pragma region Members
        GLFWwindow *m_window;
        int m_width;
        int m_height;
        #pragma endregion Members

    public:
        Window(int width, int height);
        //TODO rule of 5, we probably don't want copy constructors here
        ~Window();

        #pragma region Methods
        bool ShouldClose();
        void PollEvents();
        void SwapBuffers();
        #pragma endregion Methods

        #pragma region Accessors
        [[nodiscard]] inline int const Width() const { return m_width; };
        [[nodiscard]] inline int const Height() const { return m_height; };
        [[nodiscard]] inline float const AspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); };
        [[nodiscard]] inline GLFWwindow* const GetWindow() const { return m_window; };
        #pragma endregion Accessors
    };
}