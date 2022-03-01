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
#ifndef __TR_PUBLIC_H
#define __TR_PUBLIC_H

#include "../cgame/tr_types.h"

void		RE_BeginFrame( stereoFrame_t stereoFrame );
void		RE_BeginRegistration( vdconfig_t *glconfig );
void        RE_EndRegistration( void );
void		RE_LoadWorldMap( const char *mapname );
void		RE_SetWorldVisData( const byte *vis );
qhandle_t	RE_RegisterShader( const char *name );
qhandle_t	RE_RegisterShaderNoMip( const char *name );
qhandle_t	RE_RegisterModel( const char *name );
qhandle_t	RE_RegisterSkin( const char *name );
void        RE_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font );
void		RE_Shutdown( qboolean destroyWindow );
void        RE_StretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void        RE_StretchRaw( int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty );
void        RE_SetColor( const float *rgba );
void        RE_EndFrame( int *frontEndMsec, int *backEndMsec );
void        RE_UploadCinematic( int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty );
void        RE_ClearScene( void );
void        RE_AddRefEntityToScene( const refEntity_t *ent );
void        RE_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
void        RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void        RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void        RE_RenderScene( const refdef_t *fd );
qboolean	R_GetEntityToken( char *buffer, int size );
qboolean    R_inPVS( const vec3_t p1, const vec3_t p2 );
int			R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					float frac, const char *tagName );
void		R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );
int         R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				    int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );
int         R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
void        R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );


#endif	// __TR_PUBLIC_H
