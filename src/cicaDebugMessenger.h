#ifndef CICADA_ENGINE_CICADEBUGMESSENGER_H
#define CICADA_ENGINE_CICADEBUGMESSENGER_H

#include "cicaCore_Minimal.h"
#include "cicaInstance.h"

class cicaDebugMessenger {
  public:
    cicaDebugMessenger( const cicaInstance &_cicaInst );

  private:
    void              setupDebugMessenger( const cicaInstance &_cicaInst );
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback( vk::DebugUtilsMessageSeverityFlagBitsEXT severity, const vk::DebugUtilsMessageTypeFlagsEXT type,
                                                           const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void * );

    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
};

#endif // !CICADA_ENGINE_CICADEBUGMESSENGER_H
