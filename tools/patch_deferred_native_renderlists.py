from pathlib import Path

p = Path('src/game/client/deferred/viewrender_deferred.cpp')
s = p.read_text(encoding='utf-8')

# Replace ASW viewmodel collection API with SDK 2013's two native vectors while
# retaining the deferred G-buffer/composition setup around the actual draws.
fn_start = s.index('void CDeferredViewRender::DrawViewModels( const CViewSetup &view, bool drawViewmodel, bool bGBuffer )')
fn_end = s.index('\n//-----------------------------------------------------------------------------\n// Purpose: This renders the entire 3D view', fn_start)
viewmodels = r'''void CDeferredViewRender::DrawViewModels( const CViewSetup &view, bool drawViewmodel, bool bGBuffer )
{
	VPROF( "CViewRender::DrawViewModel" );

	bool bShouldDrawPlayerViewModel = ShouldDrawViewModel( drawViewmodel );
	bool bShouldDrawToolViewModels = ToolsEnabled();
	if ( !bShouldDrawPlayerViewModel && !bShouldDrawToolViewModels )
		return;

	CMatRenderContextPtr pRenderContext( materials );
	MDLCACHE_CRITICAL_SECTION();
	PIXEVENT( pRenderContext, "DrawViewModels()" );

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();

	CViewSetup viewModelSetup( view );
	viewModelSetup.zNear = view.zNearViewmodel;
	viewModelSetup.zFar = view.zFarViewmodel;
	viewModelSetup.fov = view.fovViewmodel;
	viewModelSetup.m_flAspectRatio = engine->GetScreenAspectRatio( view.width, view.height );
	render->Push3DView( viewModelSetup, 0, NULL, GetFrustum() );

	if ( bGBuffer )
	{
		const float flViewmodelScale = view.zFarViewmodel / view.zFar;
		CGBufferView::PushGBuffer( false, flViewmodelScale, false );
	}
	else
	{
		pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_DEFERRED_RENDER_STAGE,
			DEFERRED_RENDER_STAGE_COMPOSITION );
	}

	pRenderContext->DepthRange( 0.0f, 0.1f );

	CUtlVector< IClientRenderable * > opaqueList;
	CUtlVector< IClientRenderable * > translucentList;
	ClientLeafSystem()->CollateViewModelRenderables( opaqueList, translucentList );

	if ( ToolsEnabled() && ( !bShouldDrawPlayerViewModel || !bShouldDrawToolViewModels ) )
	{
		for ( int i = opaqueList.Count() - 1; i >= 0; --i )
		{
			IClientRenderable *pRenderable = opaqueList[i];
			bool bEntity = pRenderable->GetIClientUnknown()->GetBaseEntity() != NULL;
			if ( ( bEntity && !bShouldDrawPlayerViewModel ) || ( !bEntity && !bShouldDrawToolViewModels ) )
				opaqueList.FastRemove( i );
		}

		for ( int i = translucentList.Count() - 1; i >= 0; --i )
		{
			IClientRenderable *pRenderable = translucentList[i];
			bool bEntity = pRenderable->GetIClientUnknown()->GetBaseEntity() != NULL;
			if ( ( bEntity && !bShouldDrawPlayerViewModel ) || ( !bEntity && !bShouldDrawToolViewModels ) )
				translucentList.FastRemove( i );
		}
	}

	bool bUpdatedRefractForOpaque = UpdateRefractIfNeededByList( opaqueList );
	DrawRenderablesInList( opaqueList );

	if ( !bGBuffer )
	{
		if ( !bUpdatedRefractForOpaque )
			UpdateRefractIfNeededByList( translucentList );
		DrawRenderablesInList( translucentList, STUDIO_TRANSPARENCY );
	}
	else
	{
		pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_DEFERRED_RENDER_STAGE,
			DEFERRED_RENDER_STAGE_INVALID );
	}

	pRenderContext->DepthRange( 0.0f, 1.0f );
	if ( bGBuffer )
		CGBufferView::PopGBuffer();

	render->PopView( GetFrustum() );
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}
'''
s = s[:fn_start] + viewmodels + s[fn_end:]

# Drop the ASW-specific renderable instance/model batching block entirely.
# SDK 2013's inherited renderer already handles its bucketed render lists,
# static props, NPC ordering, translucent alpha and DrawModel(int flags).
block_start = s.index('//-----------------------------------------------------------------------------\n// Unified bit of draw code for opaque and translucent renderables')
block_end = s.index('\nvoid CBaseWorldViewDeferred::DrawWorldDeferred( float waterZAdjust )', block_start)
s = s[:block_start] + s[block_end:]

# Replace the old ASW opaque categorizer with the SDK2013 native draw path. The
# deferred material/render-stage state is already active when this executes.
fn_start = s.index('void CBaseWorldViewDeferred::DrawOpaqueRenderablesDeferred( bool bNoDecals )')
fn_end = s.index('\n\n\nstatic ConVar r_unlimitedrefract', fn_start)
opaque = r'''void CBaseWorldViewDeferred::DrawOpaqueRenderablesDeferred( bool bNoDecals )
{
	(void)bNoDecals;
	const ERenderDepthMode depthMode = ( CurrentViewID() == VIEW_DEFERRED_SHADOW )
		? DEPTH_MODE_SHADOW : DEPTH_MODE_NORMAL;
	DrawOpaqueRenderables( depthMode );
}'''
s = s[:fn_start] + opaque + s[fn_end:]

# Normalize a bool-to-enum call left over from the ASW copy.
s = s.replace('DrawOpaqueRenderables( false );', 'DrawOpaqueRenderables( DEPTH_MODE_NORMAL );')

# The removed shims must no longer be needed anywhere in this translation unit.
forbidden = [
    'RenderableInstance_t', 'CViewModelRenderablesList', 'm_InstanceData',
    'm_nModelType', 'IClientModelRenderable', 'ModelRenderSystemData_t',
    'GetClientModelRenderable', 'g_pModelRenderSystem', 'cl_modelfastpath',
    'ShouldDrawForSplitScreenUser', 'GET_ACTIVE_SPLITSCREEN_SLOT',
    'g_ShaderEditorSystem', 'm_FreezeParams'
]
leftovers = [name for name in forbidden if name in s]
if leftovers:
    raise SystemExit('ASW-only renderer symbols remain: ' + ', '.join(leftovers))

p.write_text(s, encoding='utf-8')
