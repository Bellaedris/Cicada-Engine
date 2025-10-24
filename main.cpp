/// TOUT CE CODE EST A MOI
/// RIEN QU'A MOI
/// DROIT ET LICENCE MOI

// Disable explicit constructor and permit great level of malleability by allowing aggregate init
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS // Before vulkan includes

#if defined( __INTELLISENSE__ ) || !defined( USE_CPP20_MODULES )
// #include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif // defined( __INTELLISENSE__ ) || !defined( USE_CPP20_MODULES )

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

constexpr uint32_t WIDTH  = 800;
constexpr uint32_t HEIGHT = 600;

class HelloTriangleApplication {
  public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

  private:
    void initWindow() {
        glfwInit();                                     // Init API
        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API ); // Disable OpenGL Context
        glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );   // Disable Resize
        window = glfwCreateWindow( WIDTH, HEIGHT, "Cicada-Engine", nullptr, nullptr );
    }

    void initVulkan() { createInstance(); }

    void createInstance() {
        constexpr vk::ApplicationInfo appInfo = { .pApplicationName   = "Hello Triangle",
                                                  .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
                                                  .pEngineName        = "No Engine",
                                                  .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
                                                  .apiVersion         = vk::ApiVersion14 };

        // Get the required instance extensions from GLFW
        uint32_t     glfwExtensionCount = 0;
        const char **glfwExtensions     = glfwGetRequiredInstanceExtensions( &glfwExtensionCount ); // GLFW Extensions

        // Check if the required GLFW extensions are supported by the Vulkan implementation
        auto extensionProperties = context.enumerateInstanceExtensionProperties(); // Application agnostic extensions
        for ( uint32_t i = 0; i < glfwExtensionCount; ++i ) {
            if ( std::ranges::none_of( extensionProperties, [ glfwExtension = glfwExtensions[ i ] ]
                ( auto const &extensionProperty ) {
                     return strcmp( extensionProperty.extensionName, glfwExtension ) == 0; // Verification
                 } ) ) {
                throw std::runtime_error( "Required GLFW extension not supported: " + std::string( glfwExtensions[ i ] ) );
            }
        }

        ///
        /// VALIDATION LAYER TO ADD LATER
        ///

        const vk::InstanceCreateInfo createInfo{
            .pApplicationInfo = &appInfo, .enabledExtensionCount = glfwExtensionCount, .ppEnabledExtensionNames = glfwExtensions };

        instance = vk::raii::Instance( context, createInfo );
    }

    void mainLoop() {
        while ( !glfwWindowShouldClose( window ) ) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        glfwDestroyWindow( window );
        glfwTerminate();
    }

    /// GLFW Window
    GLFWwindow *window = nullptr;

    vk::raii::Context  context;
    vk::raii::Instance instance = nullptr;
};

int main() {
    HelloTriangleApplication app;
    try {
        app.run();
    } catch ( const std::exception &exception ) {
        std::cerr << exception.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
