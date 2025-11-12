#ifndef CICADA_ENGINE_CICAINSTANCE_H
#define CICADA_ENGINE_CICAINSTANCE_H

#include "cicaCore_Minimal.h"

class cicaInstance {
  public:
    cicaInstance();

    const bool               &GetValidationLayers() const { return enableValidationLayers; }
    const vk::raii::Instance &GetInstance() const { return instance; }

    const cicaInstance &operator=( const cicaInstance &_arg ) const { return _arg; } // Required for clang ???
  private:
    void                      createInstance();
    std::vector<const char *> getRequiredExtensions() const;

    const std::vector<char const *> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    vk::raii::Context               context;
    vk::raii::Instance              instance = nullptr;
    bool                            enableValidationLayers;
};

#endif // !CICADA_ENGINE_CICAINSTANCE_H
