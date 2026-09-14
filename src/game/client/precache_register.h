// Compatibility wrapper for Alien Swarm-derived client renderer sources.
// Source SDK 2013 renamed precache_register.h to clienteffectprecachesystem.h.

#ifndef SDK2013_PRECACHE_REGISTER_COMPAT_H
#define SDK2013_PRECACHE_REGISTER_COMPAT_H
#ifdef _WIN32
#pragma once
#endif

#include "clienteffectprecachesystem.h"

// viewrender_deferred.cpp includes these immediately after precache_register.h.
// Pull them in before the compatibility macros below so declarations in the
// native SDK headers are not affected by the local method-name shims.
#include "rendertexture.h"
#include "viewpostprocess.h"
#include "viewdebug.h"
#include "deferred/deferred_shared_common.h"

#include "deferred/sdk2013_viewrender_compat.h"

#endif // SDK2013_PRECACHE_REGISTER_COMPAT_H
