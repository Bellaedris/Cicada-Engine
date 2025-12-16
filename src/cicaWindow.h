#ifndef CICADA_ENGINE_CICAWINDOW_H
#define CICADA_ENGINE_CICAWINDOW_H

#include "cicaCore_Minimal.h"

#include "cicaInstance.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

constexpr uint32_t WIDTH  = 800;
constexpr uint32_t HEIGHT = 600;

class cicaWindow {
  public:
    cicaWindow();

    void createSurface( const cicaInstance &_cicaInstance );

    [[nodiscard]] GLFWwindow    *GrabWindow() const;
    [[nodiscard]] vk::SurfaceKHR GrabSurface() const { return *m_surface; }

  private:
    void initWindow();

    GLFWwindow          *m_window  = nullptr; // GLFW Window
    vk::raii::SurfaceKHR m_surface = nullptr;
};

#endif // !CICADA_ENGINE_CICAWINDOW_H
