# Deferred renderer SDK 2013 compatibility audit

Reference: `54ac/source-sdk-vs2022-deferred` (`master`) versus this repository's `singleplayer` deferred port.

## Result

The highest-impact remaining mismatch is not in the deferred lighting math or render-target layout. It is at the engine/shader ABI boundary.

The C++ side of the deferred game shader was already moved to Source SDK 2013's `BaseVSShader` implementation, but the HLSL sources under `swarmshaders` were still resolving copied Alien Swarm-era versions of `common_vs_fxc.h`, `common_ps_fxc.h`, and `common_fxc.h` from their own directory.

The 54ac SDK 2013 port keeps the deferred shader sources in `stdshaders`, so the same `#include "common_vs_fxc.h"` / `#include "common_ps_fxc.h"` statements resolve to the SDK 2013 engine-owned headers. In this repository, the directory split caused those names to resolve to stale local copies instead.

The vertex mismatch is concrete: the old local common vertex header places values such as the obsolete light index / flex scale in Alien Swarm-era registers, while SDK 2013's common vertex header uses the SDK 2013 register layout. Deferred studio-model passes use the shared helpers for compressed vertices, skinning, morphing, model matrices, and projection, so compiling those helpers against a different register contract can corrupt model output even when BSP/world rendering remains valid.

## Changes made by this audit

The following `swarmshaders` files are now compatibility shims to the repository's SDK 2013 `stdshaders` copies:

- `common_fxc.h`
- `common_vs_fxc.h`
- `common_ps_fxc.h`

Those three SDK 2013 targets match the corresponding 54ac files byte-for-byte in this checkout.

The local `cpp_shader_constant_register_map.h` and `shader_constant_register_map.h` were also audited. They are intentionally retained rather than redirected: the deferred copy carries additional register aliases/extensions, while the stock SDK 2013 register-map file in this repository is not byte-identical to 54ac's deferred SDK copy. Replacing those maps would broaden this fix beyond the proven ABI mismatch and could remove deferred-specific definitions.

This deliberately does **not** replace deferred-specific headers such as `common_deferred_fxc.h`, `common_lighting_fxc.h`, `common_shadowmapping_fxc.h`, or `deferred_global_common.h`. Those contain deferred-renderer functionality rather than the engine-owned common VS/PS ABI and should remain local unless a specific behavioral difference is being ported.

`game_shader_deferred.vpc` lists the audited shared shader headers explicitly so the project makes the SDK-owned/deferred-specific boundary visible.

## Comparison notes

### Game shader / model path

- The 54ac port uses SDK 2013 `BaseVSShader.cpp`; this repository now does the same.
- The 54ac HLSL model passes resolve SDK 2013 `common_vs_fxc.h`; this repository previously did not. This audit fixes that.
- The deferred G-buffer, composite, and shadow model passes retain this repository's newer/deferred-specific combinations such as MultiBlend instead of blindly replacing them with the 54ac versions.

### Deferred configuration and algorithms

The deferred-specific codebases are not identical, and those differences are not all bugs. For example, this repository carries different cascade target sizing and newer shader combinations, while 54ac carries some configuration switches not present here. The audit therefore treats 54ac as the SDK 2013 integration reference, not as a file-for-file replacement source for deferred lighting behavior.

### Material-system routing

- Both ports replace stock material shaders with deferred material shaders.
- 54ac places `CreateMaterial` / `FindProceduralMaterial` handling directly in its material-system wrapper. This repository currently reaches the same runtime/procedural paths through `CDeferredMaterialSystemRuntimeHooks` in `cdeferred_manager_client.cpp` and also routes `FindMaterialEx` through replacement logic.
- This repository intentionally retains `MultiBlend -> DEFERRED_BRUSH`; the 54ac snapshot does not carry that newer path.
- 54ac disables its `DecalModulate` replacement as problematic; this repository has an explicit deferred decal path plus SCell/SM3 compatibility work. That is a feature difference, not part of the model ABI failure, so it is not removed here.

### View/render-list path

PR #13 changed the G-buffer `g_CurrentViewID` while drawing. The 54ac reference assigns the requested `viewID` directly in `DrawExecute`, and the local runtime test showed no visual change from PR #13. That makes the view-ID theory non-causal for the current model corruption. The shader ABI fix is kept independent so it can be evaluated without mixing another renderer-list rewrite into the same test.

### Shader build path

This repository uses SCell555 rather than 54ac's original generated-header workflow. The generated C++ combo interface has already been adapted for that compiler. The important requirement here is that the HLSL source and the C++ game-shader side now consume the same SDK 2013 engine-owned common shader ABI.

## Validation required locally

Regenerate/rebuild the deferred shader products rather than only rebuilding the client DLL:

1. run `src\materialsystem\swarmshaders\bdef.bat` to regenerate the deferred shader headers/VCS files;
2. regenerate projects if needed after the VPC change;
3. rebuild `game_shader_dx9.dll`;
4. test the same camera position/angle that toggles studio/static props between correct and flat/corrupt output;
5. then re-check skinned/morphed models, deferred shadows, volumetrics, decals, and MultiBlend materials.

The GitHub-side audit cannot execute the Windows Source/DirectX shader toolchain or perform an in-engine render test, so the runtime visual result remains the decisive validation step.
