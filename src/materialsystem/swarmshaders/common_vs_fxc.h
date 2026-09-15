//=============================================================================
// Deferred shader compatibility shim.
//
// The deferred sources live in swarmshaders, but they run inside Source SDK
// 2013's shader API.  Do not keep an Alien Swarm-era copy of the engine-owned
// common vertex shader ABI here: use the SDK 2013 header that the working 54ac
// port compiles against.
//=============================================================================

#ifndef DEFERRED_SDK2013_COMMON_VS_FXC_SHIM_H
#define DEFERRED_SDK2013_COMMON_VS_FXC_SHIM_H

#include "../stdshaders/common_vs_fxc.h"

#endif // DEFERRED_SDK2013_COMMON_VS_FXC_SHIM_H
