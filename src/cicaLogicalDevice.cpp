#include "cicaLogicalDevice.h"

//==========================================================================================================================================================
//==========================================================================================================================================================
cicaLogicalDevice::cicaLogicalDevice( const cicaPhysicalDevice &_cicaPhysicalDevice, const cicaWindow &_cicaWindow ) {
    createLogicalDevice( _cicaPhysicalDevice, _cicaWindow );
}

//==========================================================================================================================================================
// Create a logical device (Features that we will use based on our physical device)
// Also specify wich queue family to create
//==========================================================================================================================================================
void cicaLogicalDevice::createLogicalDevice( const cicaPhysicalDevice &_cicaPhysicalDevice, const cicaWindow &_cicaWindow ) {
    // Find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = _cicaPhysicalDevice.GetPhysicalDevice().getQueueFamilyProperties();

    // Get the first index into queueFamilyProperties which supports graphics
    auto graphicsQueueFamilyProperty = std::ranges::find_if(
        queueFamilyProperties, []( auto const &qfp ) { return ( qfp.queueFlags & vk::QueueFlagBits::eGraphics ) != static_cast<vk::QueueFlags>( 0 ); } );

    uint32_t graphicsIndex = static_cast<uint32_t>( std::distance( queueFamilyProperties.begin(), graphicsQueueFamilyProperty ) );

    // Determine a queueFamilyIndex that supports present
    // First check if the graphicsIndex is good enough
    uint32_t presentIndex = _cicaPhysicalDevice.GetPhysicalDevice().getSurfaceSupportKHR( graphicsIndex, _cicaWindow.GrabSurface() )
                                ? graphicsIndex
                                : static_cast<uint32_t>( queueFamilyProperties.size() );

    if ( presentIndex == queueFamilyProperties.size() ) {
        // The graphicsIndex doesn't support present -> look for another family index that supports both
        // Graphics and Present
        for ( size_t i = 0; i < queueFamilyProperties.size(); i++ ) {
            if ( ( queueFamilyProperties[ i ].queueFlags & vk::QueueFlagBits::eGraphics ) &&
                 _cicaPhysicalDevice.GetPhysicalDevice().getSurfaceSupportKHR( static_cast<uint32_t>( i ), _cicaWindow.GrabSurface() ) ) {
                graphicsIndex = static_cast<uint32_t>( i );
                presentIndex  = graphicsIndex;
                break;
            }
        }
        if ( presentIndex == queueFamilyProperties.size() ) {
            // There's nothing like a single family index that supports both graphics and present -> look for another
            // family index that supports present
            for ( size_t i = 0; i < queueFamilyProperties.size(); i++ ) {
                if ( _cicaPhysicalDevice.GetPhysicalDevice().getSurfaceSupportKHR( static_cast<uint32_t>( i ), _cicaWindow.GrabSurface() ) ) {
                    presentIndex = static_cast<uint32_t>( i );
                    break;
                }
            }
        }
    }
    if ( ( graphicsIndex == queueFamilyProperties.size() ) || ( presentIndex == queueFamilyProperties.size() ) ) {
        throw std::runtime_error( "Could not find a queue for graphics or present -> terminating" );
    }

    // Query for Vulkan 1.3 features
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        { _cicaPhysicalDevice.GetPhysicalDevice().getFeatures2() }, // vk::PhysicalDeviceFeatures2
        { .dynamicRendering = true },                               // Enable dynamic rendering from Vulkan 1.3
        { .extendedDynamicState = true }                            // Enable extended dynamic state from the extension
    };

    // Create a Device
    // You can submit on multiples queue that come from the same family (multi thread). Even if there is only 1 queue, it is mendatory
    float                     queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
    vk::DeviceCreateInfo      deviceCreateInfo{ .pNext = &featureChain, .queueCreateInfoCount = 1, .pQueueCreateInfos = &deviceQueueCreateInfo };
    deviceCreateInfo.enabledExtensionCount   = _cicaPhysicalDevice.deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = _cicaPhysicalDevice.deviceExtensions.data();

    m_device        = vk::raii::Device( _cicaPhysicalDevice.GetPhysicalDevice(), deviceCreateInfo );
    m_graphicsQueue = vk::raii::Queue( m_device, graphicsIndex, 0 );
    m_presentQueue  = vk::raii::Queue( m_device, presentIndex, 0 );
}