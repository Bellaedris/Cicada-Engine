#ifndef CICADA_ENGINE_CICAPHYSICALDEVICE_H
#define CICADA_ENGINE_CICAPHYSICALDEVICE_H

#include "cicaCore_Minimal.h"
#include "cicaInstance.h"

class cicaPhysicalDevice {
  public:
    cicaPhysicalDevice( const cicaInstance &_cicaInst );

    vk::raii::PhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }

    std::vector<const char *> deviceExtensions = { vk::KHRSwapchainExtensionName, vk::KHRSpirv14ExtensionName, vk::KHRSynchronization2ExtensionName,
                                                   vk::KHRCreateRenderpass2ExtensionName };

  private:
    void PickPhysicalDevice( const cicaInstance &_cicaInst );

    vk::raii::PhysicalDevice m_physicalDevice = nullptr; // Store the graphic card we use
};

#endif // !CICADA_ENGINE_CICAPHYSICALDEVICE_H
