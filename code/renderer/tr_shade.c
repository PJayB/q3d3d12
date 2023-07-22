/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_shade.c

#include "tr_local.h"
#include "tr_layer.h"

/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/


shaderCommands_t	tess;

/*
===================
@pjb: which dlights touch this surface?
===================
*/
static void RB_ComputeDlightIntersections( shaderCommands_t* input ) {
    int l;
    int     numDlights = min( backEnd.refdef.num_dlights, MAX_DLIGHTS );

	if ( !backEnd.refdef.num_dlights ) {
		return;
	}

    input->dlightCount = numDlights;

	for ( l = 0 ; l < numDlights ; l++ ) {
		dlight_t	*dl;
        dlightProjectionInfo_t* dlInfo = &input->dlightInfo[l];

        dlInfo->numIndexes = 0;

		if ( !( input->dlightBits & ( 1 << l ) ) ) {
			continue;	// this surface definately doesn't have any of this light
		}

		dl = &backEnd.refdef.dlights[l];

        // @pjb: todo: copy the light info in so we can light on the GPU
        VectorCopy( dl->color, dlInfo->color );
        dlInfo->radius = dl->radius;
        VectorCopy( dl->origin, dlInfo->origin );
        dlInfo->additive = dl->additive;
	}
}

static void RB_ComputeFog( shaderCommands_t* input )
{
    int i;
	fog_t *fog = tr.world->fogs + input->fogNum;

    // @pjb: todo: replace this because WHHHY
	for ( i = 0; i < input->numVertexes; i++ ) {
		* ( int * )&input->fogVars.colors[i] = fog->colorInt;
	}

	RB_CalcFogTexCoords( ( float * ) input->fogVars.texcoords[0] );
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum ) {

	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.dlightBits = 0;		// will be OR'd in by surface functions
    tess.dlightCount = 0;
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;
    tess.allowPreZ = backEnd.allowPreZ;

    // @pjb: force depth-rendering path if not rendering color
    if (RB_IsColorPass()) {
	    tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;
    } else if (RB_IsDepthPass()) {
        tess.currentStageIteratorFunc = state->depthStageIteratorFunc;
    } else {
        // Default to the normal generic iterator (probably debug)
        tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;
    }

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime) {
		tess.shaderTime = tess.shader->clampTime;
	}
}


/*
===============
ComputeColors
===============
*/
static void ComputeColors( shaderCommands_t* input, int stage, shaderStage_t *pStage )
{
	int		i;

	//
	// rgbGen
	//
	switch ( pStage->rgbGen )
	{
		case CGEN_IDENTITY:
			Com_Memset( input->svars[stage].colors, 0xff, input->numVertexes * 4 );
			break;
		default:
		case CGEN_IDENTITY_LIGHTING:
			Com_Memset( input->svars[stage].colors, tr.identityLightByte, input->numVertexes * 4 );
			break;
		case CGEN_LIGHTING_DIFFUSE:
			RB_CalcDiffuseColor( ( unsigned char * ) input->svars[stage].colors );
			break;
		case CGEN_EXACT_VERTEX:
			Com_Memcpy( input->svars[stage].colors, input->vertexColors, input->numVertexes * sizeof( input->vertexColors[0] ) );
			break;
		case CGEN_CONST:
			for ( i = 0; i < input->numVertexes; i++ ) {
				*(int *)input->svars[stage].colors[i] = *(int *)pStage->constantColor;
			}
			break;
		case CGEN_VERTEX:
			if ( tr.identityLight == 1 )
			{
				Com_Memcpy( input->svars[stage].colors, input->vertexColors, input->numVertexes * sizeof( input->vertexColors[0] ) );
			}
			else
			{
				for ( i = 0; i < input->numVertexes; i++ )
				{
					input->svars[stage].colors[i][0] = input->vertexColors[i][0] * tr.identityLight;
					input->svars[stage].colors[i][1] = input->vertexColors[i][1] * tr.identityLight;
					input->svars[stage].colors[i][2] = input->vertexColors[i][2] * tr.identityLight;
					input->svars[stage].colors[i][3] = input->vertexColors[i][3];
				}
			}
			break;
		case CGEN_ONE_MINUS_VERTEX:
			if ( tr.identityLight == 1 )
			{
				for ( i = 0; i < input->numVertexes; i++ )
				{
					input->svars[stage].colors[i][0] = 255 - input->vertexColors[i][0];
					input->svars[stage].colors[i][1] = 255 - input->vertexColors[i][1];
					input->svars[stage].colors[i][2] = 255 - input->vertexColors[i][2];
				}
			}
			else
			{
				for ( i = 0; i < input->numVertexes; i++ )
				{
					input->svars[stage].colors[i][0] = ( 255 - input->vertexColors[i][0] ) * tr.identityLight;
					input->svars[stage].colors[i][1] = ( 255 - input->vertexColors[i][1] ) * tr.identityLight;
					input->svars[stage].colors[i][2] = ( 255 - input->vertexColors[i][2] ) * tr.identityLight;
				}
			}
			break;
		case CGEN_FOG:
			{
				fog_t		*fog;

				fog = tr.world->fogs + input->fogNum;

				for ( i = 0; i < input->numVertexes; i++ ) {
					* ( int * )&input->svars[stage].colors[i] = fog->colorInt;
				}
			}
			break;
		case CGEN_WAVEFORM:
			RB_CalcWaveColor( &pStage->rgbWave, ( unsigned char * ) input->svars[stage].colors );
			break;
		case CGEN_ENTITY:
			RB_CalcColorFromEntity( ( unsigned char * ) input->svars[stage].colors );
			break;
		case CGEN_ONE_MINUS_ENTITY:
			RB_CalcColorFromOneMinusEntity( ( unsigned char * ) input->svars[stage].colors );
			break;
	}

	//
	// alphaGen
	//
	switch ( pStage->alphaGen )
	{
	case AGEN_SKIP:
		break;
	case AGEN_IDENTITY:
		if ( pStage->rgbGen != CGEN_IDENTITY ) {
			if ( ( pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1 ) ||
				 pStage->rgbGen != CGEN_VERTEX ) {
				for ( i = 0; i < input->numVertexes; i++ ) {
					input->svars[stage].colors[i][3] = 0xff;
				}
			}
		}
		break;
	case AGEN_CONST:
		if ( pStage->rgbGen != CGEN_CONST ) {
			for ( i = 0; i < input->numVertexes; i++ ) {
				input->svars[stage].colors[i][3] = pStage->constantColor[3];
			}
		}
		break;
	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha( &pStage->alphaWave, ( unsigned char * ) input->svars[stage].colors );
		break;
	case AGEN_LIGHTING_SPECULAR:
		RB_CalcSpecularAlpha( ( unsigned char * ) input->svars[stage].colors );
		break;
	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity( ( unsigned char * ) input->svars[stage].colors );
		break;
	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity( ( unsigned char * ) input->svars[stage].colors );
		break;
    case AGEN_VERTEX:
		if ( pStage->rgbGen != CGEN_VERTEX ) {
			for ( i = 0; i < input->numVertexes; i++ ) {
				input->svars[stage].colors[i][3] = input->vertexColors[i][3];
			}
		}
        break;
    case AGEN_ONE_MINUS_VERTEX:
        for ( i = 0; i < input->numVertexes; i++ )
        {
			input->svars[stage].colors[i][3] = 255 - input->vertexColors[i][3];
        }
        break;
	case AGEN_PORTAL:
		{
			unsigned char alpha;

			for ( i = 0; i < input->numVertexes; i++ )
			{
				float len;
				vec3_t v;

				VectorSubtract( input->xyz[i], backEnd.viewParms.orientation.origin, v );
				len = VectorLength( v );

				len /= input->shader->portalRange;

				if ( len < 0 )
				{
					alpha = 0;
				}
				else if ( len > 1 )
				{
					alpha = 0xff;
				}
				else
				{
					alpha = len * 0xff;
				}

				input->svars[stage].colors[i][3] = alpha;
			}
		}
		break;
	}

	//
	// fog adjustment for colors to fade out as fog increases
	//
	if ( input->fogNum )
	{
		switch ( pStage->adjustColorsForFog )
		{
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog( ( unsigned char * ) input->svars[stage].colors );
			break;
		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog( ( unsigned char * ) input->svars[stage].colors );
			break;
		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog( ( unsigned char * ) input->svars[stage].colors );
			break;
		case ACFF_NONE:
			break;
		}
	}
}

/*
===============
ComputeTexCoords
===============
*/
static void ComputeTexCoords( shaderCommands_t* input, int stage, shaderStage_t *pStage ) {
	int		i;
	int		b;

	for ( b = 0; b < NUM_TEXTURE_BUNDLES; b++ ) {
		int tm;

		//
		// generate the texture coordinates
		//
		switch ( pStage->bundle[b].tcGen )
		{
		case TCGEN_IDENTITY:
			Com_Memset( input->svars[stage].texcoords[b], 0, sizeof( float ) * 2 * input->numVertexes );
			break;
		case TCGEN_TEXTURE:
			for ( i = 0 ; i < input->numVertexes ; i++ ) {
				input->svars[stage].texcoords[b][i][0] = input->texCoords[i][0][0];
				input->svars[stage].texcoords[b][i][1] = input->texCoords[i][0][1];
			}
			break;
		case TCGEN_LIGHTMAP:
			for ( i = 0 ; i < input->numVertexes ; i++ ) {
				input->svars[stage].texcoords[b][i][0] = input->texCoords[i][1][0];
				input->svars[stage].texcoords[b][i][1] = input->texCoords[i][1][1];
			}
			break;
		case TCGEN_VECTOR:
			for ( i = 0 ; i < input->numVertexes ; i++ ) {
				input->svars[stage].texcoords[b][i][0] = DotProduct( input->xyz[i], pStage->bundle[b].tcGenVectors[0] );
				input->svars[stage].texcoords[b][i][1] = DotProduct( input->xyz[i], pStage->bundle[b].tcGenVectors[1] );
			}
			break;
		case TCGEN_FOG:
			RB_CalcFogTexCoords( ( float * ) input->svars[stage].texcoords[b] );
			break;
		case TCGEN_ENVIRONMENT_MAPPED:
			RB_CalcEnvironmentTexCoords( ( float * ) input->svars[stage].texcoords[b] );
			break;
		case TCGEN_BAD:
			return;
		}

		//
		// alter texture coordinates
		//
		for ( tm = 0; tm < pStage->bundle[b].numTexMods ; tm++ ) {
			switch ( pStage->bundle[b].texMods[tm].type )
			{
			case TMOD_NONE:
				tm = TR_MAX_TEXMODS;		// break out of for loop
				break;

			case TMOD_TURBULENT:
				RB_CalcTurbulentTexCoords( &pStage->bundle[b].texMods[tm].wave, 
						                 ( float * ) input->svars[stage].texcoords[b] );
				break;

			case TMOD_ENTITY_TRANSLATE:
				RB_CalcScrollTexCoords( backEnd.currentEntity->e.shaderTexCoord,
									 ( float * ) input->svars[stage].texcoords[b] );
				break;

			case TMOD_SCROLL:
				RB_CalcScrollTexCoords( pStage->bundle[b].texMods[tm].scroll,
										 ( float * ) input->svars[stage].texcoords[b] );
				break;

			case TMOD_SCALE:
				RB_CalcScaleTexCoords( pStage->bundle[b].texMods[tm].scale,
									 ( float * ) input->svars[stage].texcoords[b] );
				break;
			
			case TMOD_STRETCH:
				RB_CalcStretchTexCoords( &pStage->bundle[b].texMods[tm].wave, 
						               ( float * ) input->svars[stage].texcoords[b] );
				break;

			case TMOD_TRANSFORM:
				RB_CalcTransformTexCoords( &pStage->bundle[b].texMods[tm],
						                 ( float * ) input->svars[stage].texcoords[b] );
				break;

			case TMOD_ROTATE:
				RB_CalcRotateTexCoords( pStage->bundle[b].texMods[tm].rotateSpeed,
										( float * ) input->svars[stage].texcoords[b] );
				break;

			default:
				Com_Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'\n", pStage->bundle[b].texMods[tm].type, input->shader->name );
				break;
			}
		}
	}
}

// @pjb: dlights, fog
static void RB_CalculatePostEffects( shaderCommands_t *input )
{
	if ( input->dlightBits && input->shader->sort <= SS_OPAQUE
		&& !(input->shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) ) {
        RB_ComputeDlightIntersections( input );
    }

	if ( input->fogNum && input->shader->fogPass ) {
        RB_ComputeFog( input );
    }
}

/*
** @pjb: RB_StageIteratorDepth
*/
void RB_StageIteratorDepth( void )
{
    int stage;

	RB_DeformTessGeometry( &tess );

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = tess.xstages[stage];

		if ( !pStage )
		{
			break;
		}

		ComputeTexCoords( &tess, stage, pStage );
    }

    GFX_DrawStageDepth( &tess );
}

/*
** @pjb: RB_GenericStageIteratorPrep
*/
void RB_GenericStageIteratorPrep( shaderCommands_t* input )
{
    int stage;

	RB_DeformTessGeometry( input );

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = input->xstages[stage];

		if ( !pStage )
		{
			break;
		}

		ComputeColors( input, stage, pStage );
		ComputeTexCoords( input, stage, pStage );
    }

    RB_CalculatePostEffects( input );
}

/*
** RB_StageIteratorNull
*/
void RB_StageIteratorNull( void )
{
}

/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void )
{
    RB_GenericStageIteratorPrep( &tess );
    GFX_DrawStageGeneric( &tess );
}

/*
** RB_StageIteratorVertexLitTexture
*/
void RB_StageIteratorVertexLitTexture( void )
{
	RB_CalcDiffuseColor( ( unsigned char * ) tess.svars[0].colors );

    RB_CalculatePostEffects( &tess );

    GFX_DrawStageVertexLitTexture( &tess );
}

/*
** RB_StageIteratorLightmappedMultitexture
*/
void RB_StageIteratorLightmappedMultitexture( void ) {

    RB_ComputeDlightIntersections( &tess );

    RB_CalculatePostEffects( &tess );

    GFX_DrawStageLightmappedMultitexture( &tess );
}

/*
** RB_EndSurface
*/
void RB_EndSurface( void ) {
	shaderCommands_t *input;

	input = &tess;

	if (input->numIndexes == 0) {
		return;
	}

    if (input->indexes[SHADER_MAX_INDEXES-1] != 0) {
		Com_Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
	}	
	if (input->xyz[SHADER_MAX_VERTEXES-1][0] != 0) {
		Com_Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort ) {
		return;
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	//
	// call off to shader specific tess end function
	//
	tess.currentStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if ( r_showtris->integer ) {
		GFX_DebugDrawTris(input);
	}
	if ( r_shownormals->integer ) {
		GFX_DebugDrawNormals (input);
	}
	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;
}

