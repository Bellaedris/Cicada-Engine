//
// Created by Bellaedris on 07/01/2026.
//

#include <stdexcept>
#include "Window.h"

namespace cica
{
    Window::Window(int width, int height)
            : m_width(width)
            , m_height(height)
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_window = glfwCreateWindow(m_width, m_height, "Cicada", nullptr, nullptr);

        if (m_window == nullptr)
        {
            glfwTerminate();
            throw std::runtime_error("Couldn't create GLFW window");
        }
        glfwMakeContextCurrent(m_window);

        glfwSetFramebufferSizeCallback(m_window, Window::FramebufferSizeCallback);
        glfwSetWindowUserPointer(m_window, this);
    }

    bool Window::ShouldClose()
    {
        return glfwWindowShouldClose(m_window);
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    void Window::FramebufferSizeCallback(GLFWwindow *w, int width, int height)
    {
        Window *instance = static_cast<Window *>(glfwGetWindowUserPointer(w));
        instance->m_width = width;
        instance->m_height = height;
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
} // cica