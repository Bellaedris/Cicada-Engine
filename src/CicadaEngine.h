#ifndef CICADA_ENGINE_CICADAENGINE_H
#define CICADA_ENGINE_CICADAENGINE_H

#include "cicaCore_Minimal.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "cicaDebugMessenger.h"
#include "cicaInstance.h"
#include "cicaPhysicalDevice.h"

class CicadaEngine {
  public:
    CicadaEngine();
    ~CicadaEngine() = default;

    void run();

  private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    //==========================================================================================================================================================
    //==========================================================================================================================================================
    uint32_t findQueueFamilies( vk::raii::PhysicalDevice physicalDevice ) {
        // find the index of the first queue family that supports graphics
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        // get the first index into queueFamilyProperties which supports graphics
        auto graphicsQueueFamilyProperty = std::find_if( queueFamilyProperties.begin(), queueFamilyProperties.end(),
                                                         []( vk::QueueFamilyProperties const &qfp ) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; } );

        return static_cast<uint32_t>( std::distance( queueFamilyProperties.begin(), graphicsQueueFamilyProperty ) );
    }

    GLFWwindow        *m_window = nullptr;   // GLFW Window
    cicaInstance       m_cicaInstance;       // TODO change to pointer
    cicaDebugMessenger m_cicaDebugMessenger; // TODO change to pointer
    cicaPhysicalDevice m_cicaPhysicalDevice; // TODO change to pointer
};

#endif // !CICADA_ENGINE_CICADAENGINE_H
