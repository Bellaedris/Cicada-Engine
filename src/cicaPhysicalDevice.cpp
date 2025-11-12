#include "cicaPhysicalDevice.h"

//==========================================================================================================================================================
//==========================================================================================================================================================
cicaPhysicalDevice::cicaPhysicalDevice() {}

//==========================================================================================================================================================
//==========================================================================================================================================================
cicaPhysicalDevice::cicaPhysicalDevice( const cicaInstance &_cicaInst ) { PickPhysicalDevice( _cicaInst ); }

//==========================================================================================================================================================
//==========================================================================================================================================================
void cicaPhysicalDevice::PickPhysicalDevice( const cicaInstance &_cicaInst ) {
    const std::vector<vk::raii::PhysicalDevice> devices = _cicaInst.GetInstance().enumeratePhysicalDevices(); // Like the extension enumeration

#ifndef NDEBUG                                                 // This ...
    for ( const vk::raii::PhysicalDevice &device : devices ) { // Could create a scoring method based on properties / features of each Physical devices
        std::cout << '\t' << device.getProperties().deviceName << '\n';
    }
    std::cout << '\n';
#endif
    const auto devIter = std::ranges::find_if( devices, [ & ]( auto const &device ) {
        std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
        bool                                   isSuitable    = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

        const auto qfpIter = std::ranges::find_if( queueFamilies, []( vk::QueueFamilyProperties const &qfp ) {
            return ( qfp.queueFlags & vk::QueueFlagBits::eGraphics ) != static_cast<vk::QueueFlags>( 0 );
        } );

        isSuitable                                      = isSuitable && ( qfpIter != queueFamilies.end() );
        std::vector<vk::ExtensionProperties> extensions = device.enumerateDeviceExtensionProperties();
        bool                                 found      = true;

        for ( char const *const &extension : deviceExtensions ) {
            auto extensionIter =
                std::ranges::find_if( extensions, [ extension ]( auto const &ext ) { return std::strcmp( ext.extensionName, extension ) == 0; } );
            found = found && extensionIter != extensions.end();
        }

        isSuitable = isSuitable && found;
        if ( isSuitable ) {
            m_physicalDevice = device;
        }
        return isSuitable;
    } );
    if ( devIter == devices.end() ) {
        throw std::runtime_error( "failed to find a suitable GPU!" );
    }
}