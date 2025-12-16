#ifndef CICADA_ENGINE_CICALOGICALDEVICE_H
#define CICADA_ENGINE_CICALOGICALDEVICE_H

#include "cicaCore_Minimal.h"

#include "cicaPhysicalDevice.h"
#include "cicaWindow.h"

class cicaLogicalDevice {
  public:
    cicaLogicalDevice( const cicaPhysicalDevice &_cicaPhysicalDevice, const cicaWindow &_cicaWindow );

  private:
    void createLogicalDevice( const cicaPhysicalDevice &_cicaPhysicalDevice, const cicaWindow &_cicaWindow );

    vk::raii::Device           m_device = nullptr;
    vk::PhysicalDeviceFeatures m_deviceFeatures;          // Serve to specify a set of device features that we are going to use (useless atm)
    vk::raii::Queue            m_graphicsQueue = nullptr; // Handle to interface with the graphics family queue that we have selected
    vk::raii::Queue            m_presentQueue  = nullptr;
};

#endif // !CICADA_ENGINE_CICALOGICALDEVICE_H
