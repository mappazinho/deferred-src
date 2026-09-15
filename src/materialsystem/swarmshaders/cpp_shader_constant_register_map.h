//=============================================================================
// Deferred shader compatibility shim.
//
// C++ shader code and generated HLSL must agree on the SDK 2013 constant
// register map.  Pull the engine-owned map from stdshaders rather than keeping
// the Alien Swarm-era duplicate here.
//=============================================================================

#ifndef DEFERRED_SDK2013_CPP_SHADER_CONSTANT_REGISTER_MAP_SHIM_H
#define DEFERRED_SDK2013_CPP_SHADER_CONSTANT_REGISTER_MAP_SHIM_H

#include "../stdshaders/cpp_shader_constant_register_map.h"

#endif // DEFERRED_SDK2013_CPP_SHADER_CONSTANT_REGISTER_MAP_SHIM_H
