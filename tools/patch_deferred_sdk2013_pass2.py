from pathlib import Path


def replace_once(text, old, new, label):
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"{label}: expected 1 match, got {n}")
    return text.replace(old, new, 1)

p = Path('src/game/client/deferred/viewrender_deferred.cpp')
s = p.read_text(encoding='utf-8')

# SDK2013 has no FrustumCache_t API in the client renderer.
s = s.replace('\n//static FrustumCache_t s_FrustumCache;\nextern FrustumCache_t *FrustumCache( void );\n', '\n')

# SDK2013 does not have the ASW empty begin/end precache macro family.
old = '''PRECACHE_REGISTER_BEGIN( GLOBAL, PrecacheDeferredPostProcessingEffects )
	//PRECACHE( MATERIAL, "dev/blurfiltery_and_add_nohdr" )
PRECACHE_REGISTER_END( )
'''
s = replace_once(s, old, '', 'empty deferred precache block')

# ASW added skip-world/skip-decal flags. SP never requests those modes here;
# leave them as no-op bits so the old control flow collapses to the normal path.
insert = '''\n#ifndef DF_SKIP_WORLD\n#define DF_SKIP_WORLD 0\n#endif\n#ifndef DF_SKIP_WORLD_DECALS_AND_OVERLAYS\n#define DF_SKIP_WORLD_DECALS_AND_OVERLAYS 0\n#endif\n'''
needle = 'extern ConVar r_worldlistcache;\n'
s = replace_once(s, needle, needle + insert, 'SDK2013 draw flag compatibility')

# Native SDK2013 DrawWorldLists treats geometry/decals as the default. Remove the
# newer ASW flags that explicitly enabled those defaults.
old = '''	if ( ( nDrawFlags & DF_SKIP_WORLD ) == 0 )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_WORLD_GEOMETRY;
	}

	if ( ( nDrawFlags & DF_SKIP_WORLD_DECALS_AND_OVERLAYS ) == 0 )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_DECALS_AND_OVERLAYS;
	}

'''
s = replace_once(s, old, '', 'newer DrawWorld default flags')
s = s.replace('\n\t\tnEngineFlags &= ~DRAWWORLDLISTS_DRAW_DECALS_AND_OVERLAYS;', '')

# ASW extended SetupCurrentView with world-normal/cull flags. SDK2013 has the
# native three-argument form.
s = s.replace('extern void SetupCurrentView( const Vector &vecOrigin, const QAngle &angles, view_id_t viewID, bool bDrawWorldNormal = false, bool bCullFrontFaces = false );',
              'extern void SetupCurrentView( const Vector &vecOrigin, const QAngle &angles, view_id_t viewID );')
s = s.replace('SetupCurrentView( view.origin, view.angles, viewID, view.m_bDrawWorldNormal, view.m_bCullFrontFaces );',
              'SetupCurrentView( view.origin, view.angles, viewID );')

# The deferred world views still use this boolean internally to choose their
# two-phase world/entity path, so make it an explicit deferred-view member rather
# than relying on ASW's CViewSetup extension.
needle = '''protected:\n\n\tvoid PushComposite();\n\tvoid PopComposite();\n};'''
repl = '''protected:\n\n\tvoid PushComposite();\n\tvoid PopComposite();\n\tbool m_bDrawWorldNormal;\n};'''
s = replace_once(s, needle, repl, 'deferred world-normal member')

# ASW front-face culling hooks and FlipCulling are not present in SDK2013 SP.
old = '''\n\tif ( view.m_bCullFrontFaces )\n\t{\n\t\tpRenderContext->FlipCulling( false );\n\t}\n'''
s = replace_once(s, old, '\n', 'ASW FlipCulling cleanup')

# Viewmodel setup uses the engine's no-argument aspect ratio query in SDK2013.
s = s.replace('engine->GetScreenAspectRatio( view.width, view.height )', 'engine->GetScreenAspectRatio()')

# Remove ASW weapon pre-render model fixup; SDK2013 weapons do their normal
# model selection in the native renderer path.
start_marker = '\n\t{\n\t\t// HACK: server-side weapons use the viewmodel model'
end_marker = '\n\t}\n\n\tCMatRenderContextPtr pRenderContext( materials );'
start = s.index(start_marker, s.index('void CDeferredViewRender::RenderView'))
end = s.index(end_marker, start) + len('\n\t}')
s = s[:start] + '\n' + s[end:]

# Use the SDK2013 frame lifecycle; these ASW hooks either do not exist or are
# folded into the native scene systems.
for line in [
    '\n\t\tRenderPreScene( worldView );\n',
    '\n\t\tg_pColorCorrectionMgr->UpdateColorCorrection();\n',
    '\n\t\t// Send the current tonemap scalar to the material system\n\t\tUpdateMaterialSystemTonemapScalar();\n',
    '\n\t\tPreViewDrawScene( worldView );\n',
    '\n\t\tPostViewDrawScene( worldView );\n',
]:
    s = s.replace(line, '\n')

# Match native SDK2013 tonemapping setup before main 3D setup.
needle = '\t\t// Must be first \n\t\trender->SceneBegin();\n'
repl = '''\t\t// Must be first \n\t\trender->SceneBegin();\n\n\t\tpRenderContext.GetFrom( materials );\n\t\tpRenderContext->TurnOnToneMapping();\n\t\tpRenderContext.SafeRelease();\n'''
s = replace_once(s, needle, repl, 'native tone mapping setup')

# Remove ASW depth-of-field path and use SDK2013's native motion-blur signature.
start = s.index('\n\t\tif ( !building_cubemaps.GetBool() )\n\t\t{', s.index('void CDeferredViewRender::RenderView'))
end = s.index('\n\t\t#if defined( _X360 )', start)
replacement = '''\n\t\tif ( !building_cubemaps.GetBool() && worldView.m_bDoBloomAndToneMapping )\n\t\t{\n\t\t\tif ( mat_motion_blur_enabled.GetInt() && g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 90 )\n\t\t\t{\n\t\t\t\tpRenderContext.GetFrom( materials );\n\t\t\t\tPIXEVENT( pRenderContext, "DoImageSpaceMotionBlur" );\n\t\t\t\tDoImageSpaceMotionBlur( worldView, worldView.x, worldView.y, worldView.width, worldView.height );\n\t\t\t\tpRenderContext.SafeRelease();\n\t\t\t}\n\t\t}\n'''
s = s[:start] + replacement + s[end:]

# SDK2013 exposes the view-effects singleton and draws the fade directly rather
# than storing ASW post-process fade parameters.
s = s.replace('GetViewEffects()->GetFadeParams( &color[0], &color[1], &color[2], &color[3], &blend );',
              'vieweffects->GetFadeParams( &color[0], &color[1], &color[2], &color[3], &blend );')
s = s.replace('\n\t\t// Store off color fade params to be applied in fullscreen postprocess pass\n\t\tSetViewFadeParams( color[0], color[1], color[2], color[3], blend );\n', '\n')
needle = '\t\t// Overlay screen fade on entire screen\n\t\tPerformScreenOverlay( worldView.x, worldView.y, worldView.width, worldView.height );'
repl = '''\t\t// Overlay screen fade on entire screen\n\t\tIMaterial *pFadeMaterial = blend ? m_ModulateSingleColor : m_TranslucentSingleColor;\n\t\trender->ViewDrawFade( color, pFadeMaterial );\n\t\tPerformScreenOverlay( worldView.x, worldView.y, worldView.width, worldView.height );'''
s = replace_once(s, needle, repl, 'native screen fade')

s = s.replace('GetClientMode()->DoPostScreenSpaceEffects( &worldView );', 'g_pClientMode->DoPostScreenSpaceEffects( &worldView );')
s = s.replace('GetClientMode()->PostRenderVGui();', 'g_pClientMode->PostRenderVGui();')
s = s.replace('\n\t\t\ttempView.m_nMotionBlurMode = MOTION_BLUR_DISABLE;\t\t// FIXME: Hack to get Mark up and running', '')
s = s.replace('\n\t\t\t\tvecHudPanels.AddToTail( VGui_GetFullscreenRootVPANEL() );\n', '\n')

# SDK2013 has no custom view-matrix override in CViewSetup. Normal skybox
# origin/angle transforms are already performed directly above this block.
start = s.find('\n\t\tif( m_bCustomViewMatrix )\n\t\t{')
if start != -1:
    depth = 0
    i = start
    brace = s.index('{', start)
    i = brace
    while i < len(s):
        if s[i] == '{': depth += 1
        elif s[i] == '}':
            depth -= 1
            if depth == 0:
                end = i + 1
                s = s[:start] + s[end:]
                break
        i += 1

# Console-only Z-pass hooks are not part of SDK2013 SP's PC renderer.
s = s.replace('\tBegin360ZPass();\n', '')
s = s.replace('\tEnd360ZPass();\t\t// DrawOpaqueRenderables currently already calls End360ZPass. No harm in calling it again to make sure we\'re always ending it\n', '')
s = s.replace('\tEnd360ZPass();\n', '')

# ASW stored draw-world-normal on CViewSetup in a couple setup calls. Keep the
# explicit deferred-view member instead.
s = s.replace('view.m_bDrawWorldNormal', 'm_bDrawWorldNormal')
s = s.replace('view.m_bCullFrontFaces', 'false')

# Make light-manager loops type-consistent with its unsigned sort count.
lm = Path('src/game/client/deferred/clight_manager.cpp')
t = lm.read_text(encoding='utf-8')
t = t.replace('for( int i = 0, baseLightIdx = 0; i < m_uiSortDataCount; i++, baseLightIdx += 4 )',
              'for( unsigned int i = 0, baseLightIdx = 0; i < m_uiSortDataCount; i++, baseLightIdx += 4 )')
t = t.replace('for( int i = 0; i < m_uiSortDataCount; i++ )',
              'for( unsigned int i = 0; i < m_uiSortDataCount; i++ )')
lm.write_text(t, encoding='utf-8')

# Sanity-check the compile errors this pass is meant to eliminate.
forbidden = [
    'FrustumCache_t', 'PRECACHE_REGISTER_BEGIN', 'DRAWWORLDLISTS_DRAW_WORLD_GEOMETRY',
    'DRAWWORLDLISTS_DRAW_DECALS_AND_OVERLAYS', 'm_bCullFrontFaces', 'FlipCulling',
    'GetWeaponList', 'EnsureCorrectRenderingModel', 'RenderPreScene(', 'PreViewDrawScene(',
    'PostViewDrawScene(', 'IsDepthOfFieldEnabled', 'DoDepthOfField(', 'm_nMotionBlurMode',
    'MOTION_BLUR_DISABLE', 'GetViewEffects()', 'SetViewFadeParams', 'VGui_GetFullscreenRootVPANEL',
    'm_bCustomViewMatrix', 'm_matCustomViewMatrix', 'Begin360ZPass', 'End360ZPass',
]
left = [x for x in forbidden if x in s]
if left:
    raise SystemExit('pass2 symbols remain: ' + ', '.join(left))

p.write_text(s, encoding='utf-8')
