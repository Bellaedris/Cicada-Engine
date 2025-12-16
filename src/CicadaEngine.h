#ifndef CICADA_ENGINE_CICADAENGINE_H
#define CICADA_ENGINE_CICADAENGINE_H

#include "cicaCore_Minimal.h"

#include "cicaDebugMessenger.h"
#include "cicaInstance.h"
#include "cicaLogicalDevice.h"
#include "cicaPhysicalDevice.h"
#include "cicaWindow.h"

class CicadaEngine {
  public:
    CicadaEngine() {};
    ~CicadaEngine() = default;

    void run();

  private:
    void initVulkan();
    void mainLoop();
    void cleanup();

    // Init Window Section
    std::unique_ptr<cicaWindow> m_cicaWindow;

    // Init Vulkan Section
    std::unique_ptr<cicaInstance>       m_cicaInstance;
    std::unique_ptr<cicaDebugMessenger> m_cicaDebugMessenger;
    std::unique_ptr<cicaPhysicalDevice> m_cicaPhysicalDevice;
    std::unique_ptr<cicaLogicalDevice>  m_cicaLogicalDevice;
};

#endif // !CICADA_ENGINE_CICADAENGINE_H
