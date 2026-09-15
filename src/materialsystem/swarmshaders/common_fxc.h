//=============================================================================
// Deferred shader compatibility shim.
//
// Resolve the engine-owned common HLSL helpers from Source SDK 2013.  Keeping
// a second Alien Swarm-era copy in swarmshaders lets nested deferred includes
// silently compile against a different register/helper ABI.
//=============================================================================

#ifndef DEFERRED_SDK2013_COMMON_FXC_SHIM_H
#define DEFERRED_SDK2013_COMMON_FXC_SHIM_H

#include "../stdshaders/common_fxc.h"

#endif // DEFERRED_SDK2013_COMMON_FXC_SHIM_H
