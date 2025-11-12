#ifndef CICADA_ENGINE_CICACORE_MINIMAL_H
#define CICADA_ENGINE_CICACORE_MINIMAL_H

// Disable explicit constructor and permit great level of malleability by allowing aggregate init
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS // Before vulkan includes

#if defined( __INTELLISENSE__ ) || !defined( USE_CPP20_MODULES )
// #include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif // defined( __INTELLISENSE__ ) || !defined( USE_CPP20_MODULES )

#include <iostream>

#endif // !CICADA_ENGINE_CICACORE_MINIMAL_H
