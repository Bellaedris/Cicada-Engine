import vulkan_hpp;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#pragma region Constants
constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
#pragma endregion Constants

class HelloTriangleApplication
{
public:
    void run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:
    #pragma region Members
    GLFWwindow* m_window;
    #pragma endregion Members

    void InitWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Cicada", nullptr, nullptr);

    }

    void InitVulkan()
    {

    }

    void MainLoop()
    {
        while(!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
        }
    }

    void Cleanup()
    {
        glfwDestroyWindow(m_window);

        glfwTerminate();
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}