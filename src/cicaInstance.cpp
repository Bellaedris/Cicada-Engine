#include "cicaInstance.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//==============================================================================================================================================================
//==============================================================================================================================================================
cicaInstance::cicaInstance() {
    // Validation layers activation
#ifdef NDEBUG // This ...
    enableValidationLayers = false;
#else
    enableValidationLayers = true;
#endif // NDEBUG

    // Create the vk::instance
    createInstance();
}

//==============================================================================================================================================================
//==============================================================================================================================================================
void cicaInstance::createInstance() {
    constexpr vk::ApplicationInfo appInfo = { .pApplicationName   = "Editor Window",
                                              .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
                                              .pEngineName        = "Cicada Engine",
                                              .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
                                              .apiVersion         = vk::ApiVersion14 };

    // Validation layer verification
    // Get the required layers
    std::vector<const char *> requiredLayers;
    if ( enableValidationLayers ) {
        requiredLayers.assign( validationLayers.begin(), validationLayers.end() );
    }

    // Check if the required layers are supported by the Vulkan implementation
    const std::vector<vk::LayerProperties> layerProperties = context.enumerateInstanceLayerProperties();
    if ( std::ranges::any_of( requiredLayers, [ &layerProperties ]( auto const &requiredLayer ) {
             return std::ranges::none_of( layerProperties,
                                          [ requiredLayer ]( auto const &layerProperty ) { return strcmp( layerProperty.layerName, requiredLayer ) == 0; } );
         } ) ) {
        throw std::runtime_error( "One or more required layers are not supported!" );
    }

    const std::vector<char const *> requiredExtensions = getRequiredExtensions(); // GLFW extensions are now included in requiredExtensions

    // Check if the required extensions are supported by the Vulkan implementation.
    std::vector<vk::ExtensionProperties> extensionProperties = context.enumerateInstanceExtensionProperties();
    for ( auto const &requiredExtension : requiredExtensions ) {
        if ( std::ranges::none_of( extensionProperties, [ requiredExtension ]( auto const &extensionProperty ) {
                 return strcmp( extensionProperty.extensionName, requiredExtension ) == 0;
             } ) ) {
            throw std::runtime_error( "Required extension not supported: " + std::string( requiredExtension ) );
        }
    }

#ifndef NDEBUG // This ...
    std::cout << "Required extensions:\n";
    for ( const auto &extension : requiredExtensions ) {
        std::cout << '\t' << extension << '\n';
    }
    std::cout << '\n';

    std::cout << "Available extensions:\n";
    for ( const auto &extension : extensionProperties ) {
        std::cout << '\t' << extension.extensionName << '\n';
    }
    std::cout << '\n';
#endif // !NDEBUG

    const vk::InstanceCreateInfo createInfo{ .pApplicationInfo        = &appInfo,
                                             .enabledLayerCount       = static_cast<uint32_t>( requiredLayers.size() ),
                                             .ppEnabledLayerNames     = requiredLayers.data(),
                                             .enabledExtensionCount   = static_cast<uint32_t>( requiredExtensions.size() ),
                                             .ppEnabledExtensionNames = requiredExtensions.data() };

    instance = vk::raii::Instance( context, createInfo );
}

//==============================================================================================================================================================
// Return a required list of extensions based on whether validation layers are enabled or not
// GLFW Extensions are required anyway but those are depending of Debug / Release mod
//==============================================================================================================================================================
std::vector<const char *> cicaInstance::getRequiredExtensions() const {
    uint32_t     glfwExtensionCount = 0;
    const char **glfwExtensions     = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

    std::vector extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );
    if ( enableValidationLayers ) {
        extensions.push_back( vk::EXTDebugUtilsExtensionName ); // == to VK_EXT_debug_utils
    }
    return extensions;
}