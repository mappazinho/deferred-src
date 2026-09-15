//=============================================================================
//
// Deferred game-shader compatibility layer for Source SDK 2013's BaseVSShader.
//
// The deferred renderer was originally imported with an Alien Swarm-era copy
// of BaseVSShader.  Keep the deferred-only helpers here, but inherit the actual
// SDK 2013 implementation so studio/model shader state stays in sync with the
// engine this DLL is loaded into.
//
//=============================================================================

#ifndef DEFERRED_BASEVSSHADER_COMPAT_H
#define DEFERRED_BASEVSSHADER_COMPAT_H
#ifdef _WIN32
#pragma once
#endif

#include "../stdshaders/BaseVSShader.h"

class CDeferredBaseVSShader : public CBaseVSShader
{
public:
	FORCEINLINE void SkipPass()
	{
		Draw( false );
	}

	FORCEINLINE static IMaterialVar *GetParam( const int index )
	{
		return CBaseShader::s_ppParams[ index ];
	}

	// SDK 2013 keeps this helper behind !GAME_SHADER_DLL in BaseVSShader even
	// though the deferred model passes need the same morph accumulator setup.
	// Mirror the SDK 2013 implementation here rather than retaining the old
	// Alien Swarm BaseVSShader implementation for the whole shader DLL.
	FORCEINLINE void SetHWMorphVertexShaderState( int nDimConst, int nSubrectConst,
		VertexTextureSampler_t morphSampler )
	{
#ifndef _X360
		if ( !s_pShaderAPI->IsHWMorphingEnabled() )
			return;

		int nMorphWidth, nMorphHeight;
		s_pShaderAPI->GetStandardTextureDimensions( &nMorphWidth, &nMorphHeight,
			TEXTURE_MORPH_ACCUMULATOR );

		int nDim = s_pShaderAPI->GetIntRenderingParameter(
			INT_RENDERPARM_MORPH_ACCUMULATOR_4TUPLE_COUNT );
		float pMorphAccumSize[4] = { nMorphWidth, nMorphHeight, nDim, 0.0f };
		s_pShaderAPI->SetVertexShaderConstant( nDimConst, pMorphAccumSize );

		int nXOffset = s_pShaderAPI->GetIntRenderingParameter(
			INT_RENDERPARM_MORPH_ACCUMULATOR_X_OFFSET );
		int nYOffset = s_pShaderAPI->GetIntRenderingParameter(
			INT_RENDERPARM_MORPH_ACCUMULATOR_Y_OFFSET );
		int nWidth = s_pShaderAPI->GetIntRenderingParameter(
			INT_RENDERPARM_MORPH_ACCUMULATOR_SUBRECT_WIDTH );
		int nHeight = s_pShaderAPI->GetIntRenderingParameter(
			INT_RENDERPARM_MORPH_ACCUMULATOR_SUBRECT_HEIGHT );
		float pMorphAccumSubrect[4] = { nXOffset, nYOffset, nWidth, nHeight };
		s_pShaderAPI->SetVertexShaderConstant( nSubrectConst, pMorphAccumSubrect );

		s_pShaderAPI->BindStandardVertexTexture( morphSampler,
			TEXTURE_MORPH_ACCUMULATOR );
#endif
	}

	// Preserve the old deferred helper for any pass that only needs the morph
	// constants and deliberately binds its vertex texture separately.
	FORCEINLINE void SetHWMorphVertexShaderState_NoTex( int nDimConst,
		int nSubrectConst )
	{
#ifndef _X360
		if ( !s_pShaderAPI->IsHWMorphingEnabled() )
			return;

		int nMorphWidth, nMorphHeight;
		s_pShaderAPI->GetStandardTextureDimensions( &nMorphWidth, &nMorphHeight,
			TEXTURE_MORPH_ACCUMULATOR );

		int nDim = s_pShaderAPI->GetIntRenderingParameter(
			INT_RENDERPARM_MORPH_ACCUMULATOR_4TUPLE_COUNT );
		float pMorphAccumSize[4] = { nMorphWidth, nMorphHeight, nDim, 0.0f };
		s_pShaderAPI->SetVertexShaderConstant( nDimConst, pMorphAccumSize );

		int nXOffset = s_pShaderAPI->GetIntRenderingParameter(
			INT_RENDERPARM_MORPH_ACCUMULATOR_X_OFFSET );
		int nYOffset = s_pShaderAPI->GetIntRenderingParameter(
			INT_RENDERPARM_MORPH_ACCUMULATOR_Y_OFFSET );
		int nWidth = s_pShaderAPI->GetIntRenderingParameter(
			INT_RENDERPARM_MORPH_ACCUMULATOR_SUBRECT_WIDTH );
		int nHeight = s_pShaderAPI->GetIntRenderingParameter(
			INT_RENDERPARM_MORPH_ACCUMULATOR_SUBRECT_HEIGHT );
		float pMorphAccumSubrect[4] = { nXOffset, nYOffset, nWidth, nHeight };
		s_pShaderAPI->SetVertexShaderConstant( nSubrectConst, pMorphAccumSubrect );
#endif
	}
};

// Deferred shader sources historically name their base type CBaseVSShader.
// Keep that source surface stable while making the actual base implementation
// the SDK 2013 one above.
#undef BEGIN_VS_SHADER_FLAGS
#undef BEGIN_VS_SHADER
#define BEGIN_VS_SHADER_FLAGS(_name, _help, _flags) \
	__BEGIN_SHADER_INTERNAL( CDeferredBaseVSShader, _name, _help, _flags )
#define BEGIN_VS_SHADER(_name, _help) \
	__BEGIN_SHADER_INTERNAL( CDeferredBaseVSShader, _name, _help, 0 )

#define CBaseVSShader CDeferredBaseVSShader

#endif // DEFERRED_BASEVSSHADER_COMPAT_H
