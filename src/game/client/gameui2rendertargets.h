//========= Copyright mappazinho, all rights reserved. ============//
//
// Purpose: Render targets for the gameui2 menu background effect. Created in
//          InitClientRenderTargets so the engine allocates them inside the
//          material system's render target allocation block.
//
//=============================================================================//
#ifndef GAMEUI2RENDERTARGETS_H_
#define GAMEUI2RENDERTARGETS_H_
#ifdef _WIN32
#pragma once
#endif

#include "baseclientrendertargets.h"

#if defined( GAMEUI2 )

class CGameUI2RenderTargets : public CBaseClientRenderTargets
{
	DECLARE_CLASS_GAMEROOT( CGameUI2RenderTargets, CBaseClientRenderTargets );
public:
	virtual void InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig );
	virtual void ShutdownClientRenderTargets( void );

private:
	CTextureReference m_GameUI2SnapTexture;
	CTextureReference m_GameUI2TempTexture;
	CTextureReference m_GameUI2BlurTexture;
};

#endif // GAMEUI2

#endif // GAMEUI2RENDERTARGETS_H_
