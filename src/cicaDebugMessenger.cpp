#include "cicaDebugMessenger.h"

//==========================================================================================================================================================
//==========================================================================================================================================================
cicaDebugMessenger::cicaDebugMessenger() {}

//==========================================================================================================================================================
//==========================================================================================================================================================
cicaDebugMessenger::cicaDebugMessenger( const cicaInstance &_cicaInst ) { setupDebugMessenger( _cicaInst ); }

//==========================================================================================================================================================
//==========================================================================================================================================================
void cicaDebugMessenger::setupDebugMessenger( const cicaInstance &_cicaInst ) {
    if ( !_cicaInst.GetValidationLayers() ) {
        return;
    }

    // Choose which type of messages you want to listen with the debug callback
    const vk::DebugUtilsMessageSeverityFlagsEXT severityFlags( vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                                               /*vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo */
                                                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eError );

    // Similar to above
    const vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags( vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                              vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                              vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation );

    // Specify the pointer to the callback function
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags, .messageType = messageTypeFlags, .pfnUserCallback = &debugCallback };

    debugMessenger = _cicaInst.GetInstance().createDebugUtilsMessengerEXT( debugUtilsMessengerCreateInfoEXT );
}

///=========================================================================================================================================================
/// Error message handler
/// @param severity :
/// - ::eVerbose: Diagnostic message
/// - ::eInfo : Informational message like the creation of a resource
/// - ::eWarning : Message about behavior that is not necessarily an error, but very likely a bug in your application
/// - ::eError : Message about behavior that is invalid and may cause crashes
/// @param type :
/// - VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Some event has happened that is unrelated to the specification or performance
/// - VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something has happened that violates the specification or indicates a possible mistake
/// - VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential non-optimal use of Vulkan
/// @param pCallbackData :
/// - pMessage: The debug message as a null-terminated string
/// - pObjects: Array of Vulkan object handles related to the message
/// - objectCount: Number of objects in the array
/// @return always return vk::False
///=========================================================================================================================================================
VKAPI_ATTR vk::Bool32 VKAPI_CALL cicaDebugMessenger::debugCallback( vk::DebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                                    const vk::DebugUtilsMessageTypeFlagsEXT       type,
                                                                    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void * ) {
    std::cerr << "validation layer: type " << to_string( type ) << " msg: " << pCallbackData->pMessage << std::endl;
    return vk::False;
}