from pathlib import Path


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected 1 match, got {count}")
    return text.replace(old, new, 1)


vh = Path("src/game/client/viewrender.h")
s = vh.read_text(encoding="utf-8")
s = replace_once(
    s,
    "\nprivate:\n\tint\t\t\t\tm_BuildWorldListsNumber;",
    "\nprotected:\n\tint\t\t\t\tm_BuildWorldListsNumber;",
    "viewrender access section",
)
vh.write_text(s, encoding="utf-8")

p = Path("src/game/client/deferred/viewrender_deferred.cpp")
s = p.read_text(encoding="utf-8")

for old in [
    "\n\tg_ShaderEditorSystem->UpdateSkymask( bDrew3dSkybox );\n",
    "\n\t\tg_ShaderEditorSystem->UpdateSkymask();\n",
    "\n\t\tg_ShaderEditorSystem->CustomPostRender();\n",
]:
    if old not in s:
        raise SystemExit(f"missing ShaderEditor hook: {old!r}")
    s = s.replace(old, "\n", 1)

s = replace_once(
    s,
    "\n\tASSERT_LOCAL_PLAYER_RESOLVABLE();\n\tint slot = GET_ACTIVE_SPLITSCREEN_SLOT();\n",
    "\n",
    "RenderView split-screen prologue",
)
s = replace_once(
    s,
    "\t\tSetupMain3DView( slot, worldView, hudViewSetup, nClearFlags, saveRenderTarget );\n\n\t\tg_pClientShadowMgr->UpdateSplitscreenLocalPlayerShadowSkip();",
    "\t\tSetupMain3DView( worldView, nClearFlags );",
    "main 3D view setup",
)
s = replace_once(
    s,
    "\t\tif ( m_FreezeParams[ slot ].m_bTakeFreezeFrame )",
    "\t\tif ( m_rbTakeFreezeFrame[ STEREO_EYE_MONO ] )",
    "freeze frame check",
)
s = replace_once(
    s,
    "\t\t\tm_FreezeParams[ slot ].m_bTakeFreezeFrame = false;",
    "\t\t\tm_rbTakeFreezeFrame[ STEREO_EYE_MONO ] = false;",
    "freeze frame reset",
)
s = replace_once(s, "\tif ( VGui_IsSplitScreen() )", "\tif ( false )", "split-screen viewport border")
s = replace_once(s, "\t\t\tif ( GET_ACTIVE_SPLITSCREEN_SLOT() == 0 )", "\t\t\tif ( true )", "split-screen HUD panel branch")

split_assert = "\tAssert( !IsSplitScreenSupported() || pEnt->ShouldDrawForSplitScreenUser( GET_ACTIVE_SPLITSCREEN_SLOT() ) );\n"
if s.count(split_assert) != 2:
    raise SystemExit(f"split renderable asserts: expected 2, got {s.count(split_assert)}")
s = s.replace(split_assert, "")
blur_assert = "\tAssert( (pEnt->GetIClientUnknown() == NULL) || (pEnt->GetIClientUnknown()->GetIClientEntity() == NULL) || (pEnt->GetIClientUnknown()->GetIClientEntity()->IsBlurred() == false) );\n"
s = replace_once(s, blur_assert, "", "ASW blur assert")
s = s.replace("\tASSERT_LOCAL_PLAYER_RESOLVABLE();\n", "")

s = replace_once(s, "\tRenderableInstance_t pInstances[ MAX_STATICS_PER_BATCH ];\n\t\n", "", "static prop instance array")
s = replace_once(s, "\t\tpInstances[ numScheduled ] = itEntity->m_InstanceData;\n", "", "static prop instance assignment")
old_call = "staticpropmgr->DrawStaticProps( pStatics, pInstances, numScheduled, false, vcollide_wireframe.GetBool() );"
if s.count(old_call) != 2:
    raise SystemExit(f"static prop calls: expected 2, got {s.count(old_call)}")
s = s.replace(old_call, "staticpropmgr->DrawStaticProps( pStatics, numScheduled, DEPTH_MODE_NORMAL, vcollide_wireframe.GetBool() );")

start = s.index("extern ConVar cl_modelfastpath;")
end_marker = "static void\tDrawOpaqueRenderables_NPCs( int nCount, CClientRenderablesList::CEntry **ppEntities, bool bNoDecals )\n{\n\tDrawOpaqueRenderables_Range( nCount, ppEntities, bNoDecals );\n}\n"
end = s.index(end_marker, start) + len(end_marker)
s = s[:start] + s[end:]

fn_start = s.index("void CBaseWorldViewDeferred::DrawOpaqueRenderablesDeferred( bool bNoDecals )")
fn_end = s.index("\n\n\nstatic ConVar r_unlimitedrefract", fn_start)
replacement = '''void CBaseWorldViewDeferred::DrawOpaqueRenderablesDeferred( bool bNoDecals )
{
\tVPROF( "CViewRender::DrawOpaqueRenderables" );

\tif ( !r_drawopaquerenderables.GetBool() || !m_pMainView->ShouldDrawEntities() )
\t\treturn;

\trender->SetBlend( 1 );
\tRopeManager()->ResetRenderCache();
\tg_pParticleSystemMgr->ResetRenderCache();

\tconst int nOpaqueRenderableCount = m_pRenderablesList->m_RenderGroupCounts[RENDER_GROUP_OPAQUE];
\tCUtlVector< CClientRenderablesList::CEntry* > brushModels;
\tCUtlVector< CClientRenderablesList::CEntry* > staticProps;
\tCUtlVector< CClientRenderablesList::CEntry* > otherRenderables;
\tbrushModels.EnsureCapacity( nOpaqueRenderableCount );
\tstaticProps.EnsureCapacity( nOpaqueRenderableCount );
\totherRenderables.EnsureCapacity( nOpaqueRenderableCount );

\tCClientRenderablesList::CEntry *pOpaqueList = m_pRenderablesList->m_RenderGroups[RENDER_GROUP_OPAQUE];
\tfor ( int i = 0; i < nOpaqueRenderableCount; ++i )
\t{
\t\tswitch ( pOpaqueList[i].m_nModelType )
\t\t{
\t\tcase RENDERABLE_MODEL_BRUSH:
\t\t\tbrushModels.AddToTail( &pOpaqueList[i] );
\t\t\tbreak;
\t\tcase RENDERABLE_MODEL_STATIC_PROP:
\t\t\tstaticProps.AddToTail( &pOpaqueList[i] );
\t\t\tbreak;
\t\tdefault:
\t\t\totherRenderables.AddToTail( &pOpaqueList[i] );
\t\t\tbreak;
\t\t}
\t}

\tDrawOpaqueRenderables_DrawBrushModels( brushModels.Count(), brushModels.Base(), bNoDecals );
\tDrawOpaqueRenderables_Range( otherRenderables.Count(), otherRenderables.Base(), bNoDecals );
\tDrawOpaqueRenderables_DrawStaticProps( staticProps.Count(), staticProps.Base() );

\tRopeManager()->DrawRenderCache( false );
\tg_pParticleSystemMgr->DrawRenderCache( false );
}'''
s = s[:fn_start] + replacement + s[fn_end:]

forbidden = [
    "GET_ACTIVE_SPLITSCREEN_SLOT",
    "ASSERT_LOCAL_PLAYER_RESOLVABLE",
    "UpdateSplitscreenLocalPlayerShadowSkip",
    "m_FreezeParams",
    "g_ShaderEditorSystem",
    "ModelRenderSystemData_t",
    "GetClientModelRenderable",
    "g_pModelRenderSystem",
    "cl_modelfastpath",
]
leftovers = [x for x in forbidden if x in s]
if leftovers:
    raise SystemExit("ASW-only symbols remain: " + ", ".join(leftovers))

p.write_text(s, encoding="utf-8")
