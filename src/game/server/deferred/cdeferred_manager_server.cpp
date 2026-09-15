
#include "cbase.h"
#include "deferred/deferred_shared_common.h"
#include "mapentities.h"
#include "filesystem.h"
#include "bspfile.h"

#include "tier0/memdbgon.h"

static CDeferredManagerServer __g_defmanager;
CDeferredManagerServer *GetDeferredManager()
{
	return &__g_defmanager;
}

CDeferredManagerServer::CDeferredManagerServer() : BaseClass( "DeferredManagerServer" )
{
}

CDeferredManagerServer::~CDeferredManagerServer()
{
}

bool CDeferredManagerServer::Init()
{
	return true;
}

void CDeferredManagerServer::Shutdown()
{
}

#define LIGHT_MIN_LIGHT_VALUE 0.03f

static float ComputeMapLightRadius( float radius, float intensity,
	float constantAttn, float linearAttn, float quadraticAttn )
{
	if ( radius > 0.0f )
		return radius;

	if ( quadraticAttn == 0.0f )
	{
		if ( linearAttn == 0.0f )
			return 2000.0f;

		return MAX( 0.0f, ( intensity / LIGHT_MIN_LIGHT_VALUE - constantAttn ) / linearAttn );
	}

	const float a = quadraticAttn;
	const float b = linearAttn;
	const float c = constantAttn - intensity / LIGHT_MIN_LIGHT_VALUE;
	const float discrim = b * b - 4.0f * a * c;

	if ( discrim < 0.0f )
		return 2000.0f;

	return MAX( 0.0f, ( -b + sqrtf( discrim ) ) / ( 2.0f * a ) );
}

static float ComputeMapLightRadius( const dworldlight_t &light )
{
	if ( light.radius > 0.0f )
		return light.radius;

	const float intensity = sqrtf( DotProduct( light.intensity, light.intensity ) );
	return ComputeMapLightRadius( 0.0f, intensity, light.constant_attn,
		light.linear_attn, light.quadratic_attn );
}

static float GetMapEntityFloat( CEntityMapData &data, const char *key, float defaultValue )
{
	char value[MAPKEY_MAXLENGTH];
	if ( !data.ExtractValue( key, value ) || !value[0] )
		return defaultValue;

	return atof( value );
}

static bool GetMapEntityVector( CEntityMapData &data, const char *key, Vector &out )
{
	char value[MAPKEY_MAXLENGTH];
	if ( !data.ExtractValue( key, value ) || !value[0] )
		return false;

	UTIL_StringToVector( out.Base(), value );
	return true;
}

void CDeferredManagerServer::LevelInitPreEntity()
{
	const char *entStr = engine->GetMapEntitiesString();
	if ( entStr == NULL || !*entStr )
		return;

	// Maps authored specifically for the deferred renderer should keep their
	// explicit deferred-light setup instead of receiving a second set of lights.
	if ( V_stristr( entStr, "light_deferred" ) != NULL )
		return;

	char bspPath[MAX_PATH];
	Q_snprintf( bspPath, sizeof( bspPath ), "maps/%s.bsp", STRING( gpGlobals->mapname ) );

	FileHandle_t hFile = g_pFullFileSystem->Open( bspPath, "rb", "GAME" );
	if ( hFile == FILESYSTEM_INVALID_HANDLE )
	{
		Warning( "Deferred lighting: unable to open %s for world-light data.\n", bspPath );
		return;
	}

	dheader_t header;
	if ( g_pFullFileSystem->Read( &header, sizeof( header ), hFile ) != sizeof( header ) )
	{
		g_pFullFileSystem->Close( hFile );
		return;
	}

	if ( header.ident != IDBSPHEADER )
	{
		g_pFullFileSystem->Close( hFile );
		return;
	}

	const lump_t *pLightLump = &header.lumps[ LUMP_WORLDLIGHTS ];
	if ( pLightLump->filelen == 0 && header.lumps[ LUMP_WORLDLIGHTS_HDR ].filelen > 0 )
		pLightLump = &header.lumps[ LUMP_WORLDLIGHTS_HDR ];

	CUtlVector< dworldlight_t > worldLights;
	if ( pLightLump->filelen > 0 && ( pLightLump->filelen % sizeof( dworldlight_t ) ) == 0 )
	{
		const int lightCount = pLightLump->filelen / sizeof( dworldlight_t );
		worldLights.SetCount( lightCount );
		g_pFullFileSystem->Seek( hFile, pLightLump->fileofs, FILESYSTEM_SEEK_HEAD );
		g_pFullFileSystem->Read( worldLights.Base(), pLightLump->filelen, hFile );
	}

	g_pFullFileSystem->Close( hFile );

	const char *szParamDiffuse = GetLightParamName( LPARAM_DIFFUSE );
	const char *szParamLightType = GetLightParamName( LPARAM_LIGHTTYPE );
	const char *szParamSpotConeInner = GetLightParamName( LPARAM_SPOTCONE_INNER );
	const char *szParamSpotConeOuter = GetLightParamName( LPARAM_SPOTCONE_OUTER );
	const char *szParamPower = GetLightParamName( LPARAM_POWER );
	const char *szParamRadius = GetLightParamName( LPARAM_RADIUS );
	const char *szParamVisDist = GetLightParamName( LPARAM_VIS_DIST );
	const char *szParamVisRange = GetLightParamName( LPARAM_VIS_RANGE );
	const char *szParamShadowDist = GetLightParamName( LPARAM_SHADOW_DIST );
	const char *szParamShadowRange = GetLightParamName( LPARAM_SHADOW_RANGE );
#if DEFCFG_CONFIGURABLE_VOLUMETRIC_LOD
	const char *szParamVolumeSamples = GetLightParamName( LPARAM_VOLUME_SAMPLES );
#endif

	int convertedLights = 0;
	char skipToken[MAPKEY_MAXLENGTH];

	for ( ; true; entStr = MapEntity_SkipToNextEntity( entStr, skipToken ) )
	{
		char token[MAPKEY_MAXLENGTH];
		entStr = MapEntity_ParseToken( entStr, token );
		if ( entStr == NULL )
			break;

		if ( token[0] != '{' )
			continue;

		CEntityMapData entData( (char *)entStr );

		char className[MAPKEY_MAXLENGTH] = { 0 };
		entData.ExtractValue( "classname", className );

		const bool isPoint = FStrEq( className, "light" );
		const bool isSpot = FStrEq( className, "light_spot" );
		if ( !isPoint && !isSpot )
			continue;

		Vector pos = vec3_origin;
		GetMapEntityVector( entData, "origin", pos );

		QAngle angles = vec3_angle;
		Vector angleVector;
		if ( GetMapEntityVector( entData, "angles", angleVector ) )
			angles.Init( angleVector.x, angleVector.y, angleVector.z );

		if ( isSpot )
		{
			char pitchText[MAPKEY_MAXLENGTH] = { 0 };
			if ( entData.ExtractValue( "pitch", pitchText ) && pitchText[0] )
				angles.x = -atof( pitchText );
		}

		float radius = GetMapEntityFloat( entData, "_distance", 0.0f );
		float innerCone = GetMapEntityFloat( entData, "_inner_cone", 30.0f );
		float outerCone = GetMapEntityFloat( entData, "_cone", 45.0f );
		float exponent = GetMapEntityFloat( entData, "_exponent", 1.0f );

		// The BSP world-light lump is VRAD's resolved representation of these
		// entities. Prefer it for the final radius and, for spotlights, the real
		// beam direction and cone. This also handles target-based light_spot
		// entities whose useful trajectory is not captured by raw angles alone.
		const dworldlight_t *pBestWorldLight = NULL;
		float bestDistSqr = FLT_MAX;
		FOR_EACH_VEC( worldLights, i )
		{
			const dworldlight_t &worldLight = worldLights[i];
			if ( isPoint && worldLight.type != emit_point )
				continue;
			if ( isSpot && worldLight.type != emit_spotlight )
				continue;

			const float distSqr = ( worldLight.origin - pos ).LengthSqr();
			if ( distSqr < bestDistSqr )
			{
				bestDistSqr = distSqr;
				pBestWorldLight = &worldLight;
			}
		}

		// World-light origins normally match their source entity exactly. Keep a
		// small tolerance so unrelated nearby lights never steal each other's data.
		if ( pBestWorldLight != NULL && bestDistSqr <= 16.0f )
		{
			radius = ComputeMapLightRadius( *pBestWorldLight );
			if ( isSpot )
			{
				VectorAngles( pBestWorldLight->normal, angles );
				innerCone = RAD2DEG( acosf( clamp( pBestWorldLight->stopdot, -1.0f, 1.0f ) ) );
				outerCone = RAD2DEG( acosf( clamp( pBestWorldLight->stopdot2, -1.0f, 1.0f ) ) );
				exponent = pBestWorldLight->exponent;
			}
		}

		char lightColor[MAPKEY_MAXLENGTH] = "255 255 255 200";
		entData.ExtractValue( "_light", lightColor );

		int color[4] = { 255, 255, 255, 200 };
		UTIL_StringToIntArray( color, 4, lightColor );

		if ( radius <= 0.0f )
		{
			radius = ComputeMapLightRadius( 0.0f, MAX( 1, color[3] ),
				GetMapEntityFloat( entData, "_constant_attn", 0.0f ),
				GetMapEntityFloat( entData, "_linear_attn", 0.0f ),
				GetMapEntityFloat( entData, "_quadratic_attn", 1.0f ) );
		}

		if ( radius <= 0.0f )
			continue;

		CDeferredLight *lightEntity = static_cast< CDeferredLight* >(
			CBaseEntity::CreateNoSpawn( "light_deferred", pos, angles ) );
		if ( lightEntity == NULL )
			continue;

		lightEntity->KeyValue( szParamDiffuse, lightColor );
		lightEntity->KeyValue( "spawnflags", "11" ); // enabled + shadows + volumetrics
		lightEntity->KeyValue( szParamLightType, isSpot ? "1" : "0" );
		lightEntity->KeyValue( szParamPower, UTIL_VarArgs( "%g", exponent ) );
		lightEntity->KeyValue( szParamRadius, UTIL_VarArgs( "%g", radius ) );

		if ( isSpot )
		{
			lightEntity->KeyValue( szParamSpotConeInner, UTIL_VarArgs( "%g", innerCone ) );
			lightEntity->KeyValue( szParamSpotConeOuter, UTIL_VarArgs( "%g", outerCone ) );
		}

#if DEFCFG_CONFIGURABLE_VOLUMETRIC_LOD
		lightEntity->KeyValue( szParamVolumeSamples, "50" );
#endif

		const int visDist = MIN( 32767, MAX( 1, (int)( radius * 2.0f ) ) );
		const int visRange = MIN( 32767, MAX( 1, (int)( radius * 1.25f ) ) );
		const int shadowDist = MIN( 32767, MAX( 1, (int)( radius * ( 5.0f / 6.0f ) ) ) );
		const int shadowRange = MIN( 32767, MAX( 1, (int)( radius * ( 2.0f / 3.0f ) ) ) );

		lightEntity->KeyValue( szParamVisDist, UTIL_VarArgs( "%d", visDist ) );
		lightEntity->KeyValue( szParamVisRange, UTIL_VarArgs( "%d", visRange ) );
		lightEntity->KeyValue( szParamShadowDist, UTIL_VarArgs( "%d", shadowDist ) );
		lightEntity->KeyValue( szParamShadowRange, UTIL_VarArgs( "%d", shadowRange ) );

		char targetName[MAPKEY_MAXLENGTH] = { 0 };
		if ( entData.ExtractValue( "targetname", targetName ) && targetName[0] )
			lightEntity->KeyValue( "targetname", targetName );

		char parentName[MAPKEY_MAXLENGTH] = { 0 };
		if ( entData.ExtractValue( "parentname", parentName ) && parentName[0] )
			lightEntity->KeyValue( "parentname", parentName );

		DispatchSpawn( lightEntity );
		++convertedLights;
	}

	DevMsg( 1, "Deferred lighting: converted %d map light/light_spot entities with volumetrics enabled.\n",
		convertedLights );
}

int CDeferredManagerServer::AddCookieTexture( const char *pszCookie )
{
	Assert( g_pStringTable_LightCookies != NULL );

	return  g_pStringTable_LightCookies->AddString( true, pszCookie );
}

void CDeferredManagerServer::AddWorldLight( CDeferredLight *l )
{
	CDeferredLightContainer *pC = FindAvailableContainer();

	if ( !pC )
		pC = assert_cast< CDeferredLightContainer* >( CreateEntityByName( "deferred_light_container" ) );

	pC->AddWorldLight( l );
}