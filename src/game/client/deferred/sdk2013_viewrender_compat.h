// SDK 2013 single-player compatibility shims for the Alien Swarm-derived
// deferred view renderer.
//
// Keep this header local to viewrender_deferred.cpp.  The goal is to bridge
// APIs that only existed in the Alien Swarm renderer without changing the
// deferred G-buffer, shadow, lighting, radiosity or volumetric passes.

#ifndef DEFERRED_SDK2013_VIEWRENDER_COMPAT_H
#define DEFERRED_SDK2013_VIEWRENDER_COMPAT_H
#ifdef _WIN32
#pragma once
#endif

// -----------------------------------------------------------------------------
// Alien Swarm split-screen helpers.
// SDK 2013 SP has one active local player/view, so slot zero is the only slot.
// -----------------------------------------------------------------------------
#ifndef ASSERT_LOCAL_PLAYER_RESOLVABLE
#define ASSERT_LOCAL_PLAYER_RESOLVABLE() ((void)0)
#endif

#ifndef GET_ACTIVE_SPLITSCREEN_SLOT
#define GET_ACTIVE_SPLITSCREEN_SLOT() 0
#endif

#ifndef IsSplitScreenSupported
#define IsSplitScreenSupported() false
#endif

// IClientRenderable in SDK 2013 has no per-split-screen visibility method.
#define ShouldDrawForSplitScreenUser( slot ) ShouldDraw()

// Alien Swarm's IClientEntity exposed this debug-only state.  Dormant entities
// are not expected in the active render list, making IsDormant the closest
// SDK2013-side debug guard without changing release rendering behavior.
#define IsBlurred() IsDormant()

// ASW's SetupMain3DView accepted slot/HUD/render-target arguments. SDK2013 SP
// owns those details internally and takes only the world view and clear flags.
#define SetupMain3DView( slot, worldView, hudView, clearFlags, saveRenderTarget ) \
	SetupMain3DView( worldView, clearFlags )

// The ASW call only updates split-screen shadow exclusion state. In SP there is
// no exclusion list to update; PreRender is the SDK2013 shadow-manager setup
// point and is safe here before deferred global/light setup.
#define UpdateSplitscreenLocalPlayerShadowSkip() PreRender()

// ASW stored freeze-frame state per split-screen slot in m_FreezeParams.
// SDK2013 uses a different stereo-eye freeze implementation. The deferred
// renderer does not depend on freeze-frame capture, so keep this legacy branch
// inert rather than coupling it to lighting/depth state.
struct DeferredFreezeParamsCompat_t
{
	DeferredFreezeParamsCompat_t() : m_bTakeFreezeFrame( false ) {}
	bool m_bTakeFreezeFrame;
};
static DeferredFreezeParamsCompat_t g_DeferredFreezeParamsCompat[1];
#define m_FreezeParams g_DeferredFreezeParamsCompat

// -----------------------------------------------------------------------------
// Shader Editor integration.
// The external Shader Editor module is not part of Source SDK 2013. These calls
// are editor/post-process hooks and are not required for the deferred pipeline.
// -----------------------------------------------------------------------------
class CDeferredShaderEditorCompat
{
public:
	void UpdateSkymask( bool bForce = false ) { (void)bForce; }
	void CustomPostRender() {}
};
static CDeferredShaderEditorCompat g_DeferredShaderEditorCompat;
#define g_ShaderEditorSystem (&g_DeferredShaderEditorCompat)

// -----------------------------------------------------------------------------
// Alien Swarm renderable instance/model fast path.
// SDK2013's render list has no RenderableInstance_t/m_InstanceData and no
// modelrendersystem fast path. Keep the imported code type-correct, but force it
// onto SDK2013's normal per-renderable/static-prop path.
// -----------------------------------------------------------------------------
#define RenderableInstance_t ClientRenderHandle_t
#define m_InstanceData m_RenderHandle

// ASW added DrawModel( flags, instance ); SDK2013 exposes DrawModel( flags ).
// The instance value is only batching metadata, so dropping it preserves the
// actual renderable draw and its current deferred material state.
#define DrawModel( flags, ... ) DrawModel( flags )

// ASW's static-prop API also accepted an instance-data array. SDK2013 does not.
#define DrawStaticProps( props, instances, count, shadowDepth, wireframe ) \
	DrawStaticProps( props, count, shadowDepth, wireframe )

#define IClientModelRenderable IClientRenderable
#define GetClientModelRenderable() GetClientRenderable()

enum ModelRenderMode_t
{
	MODEL_RENDER_MODE_NORMAL = 0,
	MODEL_RENDER_MODE_SHADOW_DEPTH
};

struct ModelRenderSystemData_t
{
	IClientRenderable *m_pRenderable;
	IClientRenderable *m_pModelRenderable;
	ClientRenderHandle_t m_RenderHandle;
};

class CDeferredModelRenderSystemCompat
{
public:
	void DrawModels( ModelRenderSystemData_t *pModels, int nCount, ModelRenderMode_t mode )
	{
		(void)pModels;
		(void)nCount;
		(void)mode;
	}
};
static CDeferredModelRenderSystemCompat g_DeferredModelRenderSystemCompatImpl;
static CDeferredModelRenderSystemCompat *g_pModelRenderSystem = &g_DeferredModelRenderSystemCompatImpl;

// These names are referenced by the imported renderer but are not SDK2013 SP
// client cvars. Use private cvars and pin the unsupported fast path off.
static ConVar deferred_compat_modelfastpath(
	"deferred_compat_modelfastpath", "0", FCVAR_CHEAT,
	"Internal: keep the Alien Swarm model fast path disabled on SDK2013." );
static ConVar deferred_compat_skipslowpath(
	"deferred_compat_skipslowpath", "0", FCVAR_CHEAT,
	"Internal: keep SDK2013's normal model rendering path enabled." );

#define cl_modelfastpath deferred_compat_modelfastpath
#define cl_skipslowpath deferred_compat_skipslowpath

#endif // DEFERRED_SDK2013_VIEWRENDER_COMPAT_H
