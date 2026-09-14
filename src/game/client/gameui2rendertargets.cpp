//========= Copyright mappazinho, all rights reserved. ============//
//
// Purpose: Render targets for the gameui2 menu background effect.
//
//=============================================================================//
#include "cbase.h"
#include "gameui2rendertargets.h"

#include "materialsystem/itexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( GAMEUI2 )

//-----------------------------------------------------------------------------
// Purpose: Called by the engine inside the render target allocation block.
//-----------------------------------------------------------------------------
void CGameUI2RenderTargets::InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig )
{
	BaseClass::InitClientRenderTargets( pMaterialSystem, pHardwareConfig );

	int nWidth, nHeight;
	pMaterialSystem->GetBackBufferDimensions( nWidth, nHeight );
	nWidth /= 4;
	nHeight /= 4;

	m_GameUI2SnapTexture.Init( pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_GameUI2BG_Snap", nWidth, nHeight, RT_SIZE_LITERAL,
		IMAGE_FORMAT_RGBA8888, MATERIAL_RT_DEPTH_NONE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOMIP, 0 ) );

	m_GameUI2TempTexture.Init( pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_GameUI2BG_Temp", nWidth, nHeight, RT_SIZE_LITERAL,
		IMAGE_FORMAT_RGBA8888, MATERIAL_RT_DEPTH_NONE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOMIP, 0 ) );

	m_GameUI2BlurTexture.Init( pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_GameUI2BG_Blur", nWidth, nHeight, RT_SIZE_LITERAL,
		IMAGE_FORMAT_RGBA8888, MATERIAL_RT_DEPTH_NONE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOMIP, 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine on material system shutdown / mode changes.
//-----------------------------------------------------------------------------
void CGameUI2RenderTargets::ShutdownClientRenderTargets( void )
{
	m_GameUI2SnapTexture.Shutdown();
	m_GameUI2TempTexture.Shutdown();
	m_GameUI2BlurTexture.Shutdown();

	BaseClass::ShutdownClientRenderTargets();
}

static CGameUI2RenderTargets g_GameUI2RenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CGameUI2RenderTargets, IClientRenderTargets, CLIENTRENDERTARGETS_INTERFACE_VERSION, g_GameUI2RenderTargets );

#endif // GAMEUI2
