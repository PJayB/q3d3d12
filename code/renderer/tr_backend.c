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
#include "tr_local.h"
#include "tr_layer.h"

backEndData_t	*backEndData[SMP_FRAMES];
backEndState_t	backEnd;

static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

const float s_identityMatrix[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0, 
    0, 0, 1, 0,
    0, 0, 0, 1 
};

void RB_UploadCinematic(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

// @pjb: convenience functions for checking the pass features
qboolean RB_IsDepthPass(void) { 
    return (backEnd.viewParms.passFeatures & PASSF_DEPTH) != 0;
}
qboolean RB_IsColorPass(void) { 
    return (backEnd.viewParms.passFeatures & (PASSF_DEBUG|PASSF_COLOR)) != 0;
}

/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void ) {
	float		c;

	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	c = ( backEnd.refdef.time & 255 ) / 255.0f;
    {
        float clearCol[] = { c, c, c, 1.0f };
        GFX_Clear( CLEAR_COLOR, clearCol, CLEAR_DEFAULT_STENCIL, CLEAR_DEFAULT_DEPTH );
    }

	backEnd.isHyperspace = qtrue;
}


static void Set3DProjection( void ) {
	GFX_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );

	// set the window clipping
	GFX_SetViewport( 
        backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

    GFX_ResetState3D();

    backEnd.allowPreZ = qtrue;
}

/*
=================
GLRB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView (void) {
	int clearBits = 0;
    float clearCol[4] = { 0, 0, 0, 0 };

    // we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	//
	// set the modelview matrix for the viewer
	// ensures that depth writes are enabled for the depth clear
	//
	Set3DProjection();

    // clear relevant buffers
    if ( RB_IsDepthPass() ) {
	    clearBits = CLEAR_DEPTH;
    }

	if ( r_measureOverdraw->integer )
	{
		clearBits |= CLEAR_STENCIL;
	}

	if ( !RB_IsDepthPass() && 
         r_fastsky->integer && 
         !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		clearBits |= CLEAR_COLOR;	// FIXME: only if sky shaders have been used
#ifdef _DEBUG
        clearCol[0] = 0.8f; // FIXME: get color of sky
        clearCol[1] = 0.7f;
        clearCol[2] = 0.4f;
        clearCol[3] = 1.0f;
#endif
	}

    if ( clearBits != 0 ) {
	    GFX_Clear( clearBits, clearCol, CLEAR_DEFAULT_STENCIL, CLEAR_DEFAULT_DEPTH );
    }

	if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}

	//glState.faceCulling = -1;		// force face culling to set next time
    // @pjb: this should be guaranteed to be valid now that the shadowing code 
    // doesn't call CullFace directly.

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	// clip to the plane of the portal
	if ( backEnd.viewParms.isPortal ) {
		float	plane[4];
		float	plane2[4];

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct (backEnd.viewParms.orientation.axis[0], plane);
		plane2[1] = DotProduct (backEnd.viewParms.orientation.axis[1], plane);
		plane2[2] = DotProduct (backEnd.viewParms.orientation.axis[2], plane);
		plane2[3] = DotProduct (plane, backEnd.viewParms.orientation.origin) - plane[3];

		GFX_SetPortalRendering( qtrue, s_flipMatrix, plane2 );
	} else {
		GFX_SetPortalRendering( qfalse, NULL, NULL );
	}
}


/*
==================
RB_RenderDrawSurfList
==================
*/
void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	shader_t		*shader, *oldShader;
	int				fogNum, oldFogNum;
	int				entityNum, oldEntityNum;
	int				dlighted, oldDlighted;
	qboolean		depthRange, oldDepthRange;
	int				i;
	drawSurf_t		*drawSurf;
	size_t			oldSort;
	float			originalTime;

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView ();

	// draw everything
	oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
	oldFogNum = -1;
	oldDepthRange = qfalse;
	oldDlighted = qfalse;
	oldSort = ~0U; // @pjb: unsigned
	depthRange = qfalse;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for (i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++) {
		if ( drawSurf->sort == oldSort ) {
			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}
		oldSort = drawSurf->sort;
		R_DecomposeSort( (unsigned) drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted );

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if (shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted 
			|| ( entityNum != oldEntityNum && !shader->entityMergable ) ) {
			if (oldShader != NULL) {
				RB_EndSurface();
			}
			RB_BeginSurface( shader, fogNum );
			oldShader = shader;

            // @pjb: don't need to track these in depth only mode
            if (RB_IsColorPass()) {
			    oldFogNum = fogNum;
			    oldDlighted = dlighted;
            }

            // @pjb: make sure dlightBits is propagated safely
            // @pjb: only do that if we're in color mode
            if (entityNum == oldEntityNum && RB_IsColorPass()) {
                tess.dlightBits = backEnd.currentEntity->dlightBits;
            }
		}

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) {
			depthRange = qfalse;

			if ( entityNum != ENTITYNUM_WORLD ) {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation );

				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->needDlights && RB_IsColorPass()) {
                    tess.dlightBits |= backEnd.currentEntity->dlightBits;
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation );
				}

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK ) {
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.orientation = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

                // @pjb: no need to transform lights in depth mode
                if (RB_IsColorPass()) {
				    R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation );
                }
			}

            GFX_SetModelViewMatrix( backEnd.orientation.modelMatrix );

			//
			// change depthrange if needed
			//
			if ( oldDepthRange != depthRange ) {
				if ( depthRange ) {
                    GFX_SetDepthRange( 0, 0.3f );
				} else {
                    GFX_SetDepthRange( 0, 1 );
				}
				oldDepthRange = depthRange;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	backEnd.refdef.floatTime = originalTime;

	// draw the contents of the last shader batch
	if (oldShader != NULL) {
		RB_EndSurface();
	}

	// go back to the world modelview matrix
    GFX_SetModelViewMatrix( backEnd.viewParms.world.modelMatrix );
	if ( depthRange ) {
        GFX_SetDepthRange( 0, 1 );
	}

	// add light flares on lights that aren't obscured
    if (RB_IsColorPass()) {
    	RB_RenderFlares();
    }
}


/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

// @pjb: constructs an orthographic matrix
void ConstructOrtho(float* m, float left, float right, float bottom, float top, float Near, float Far)
{
	float x = -((right + left) / (right - left));
	float y = -((top + bottom) / (top - bottom));
	float z = -((Far + Near) / (Far - Near));
		
	m[ 0] = 2 / (right - left);
	m[ 1] = 0;
	m[ 2] = 0;
	m[ 3] = 0;
	
	m[ 4] = 0;
	m[ 5] = 2 / (top - bottom);
	m[ 6] = 0;
	m[ 7] = 0;
	
	m[ 8] = 0;
	m[ 9] = 0;
	m[10] = -2 / (Far - Near);
	m[11] = 0;
	
	m[12] = x;
	m[13] = y;
	m[14] = z;
	m[15] = 1;
}


/*
================
Set2DProjection

================
*/
static void	Set2DProjection(void) {
    float orthoMatrix[16];

    backEnd.projection2D = qtrue;

    GFX_SetViewport( 0, 0, vdConfig.vidWidth, vdConfig.vidHeight );

    // @pjb: generate orthographic projection matrix manually and pass to
    // GFX_SetProjection
    ConstructOrtho( orthoMatrix, 0, vdConfig.vidWidth, vdConfig.vidHeight, 0, 0, 1 ); 
    GFX_SetProjectionMatrix( orthoMatrix );

    // Reset the state for 2D rendering
    GFX_ResetState2D();

	// set time for 2D shaders
	backEnd.refdef.time = RI_ScaledMilliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
    backEnd.allowPreZ = qfalse; // @pjb: disallow prepass
}


/*
=============
@pjb: RB_StretchRaw

Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RB_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	int			i, j;
	int			start, end;

	if ( !tr.registered ) {
		return;
	}

    // we definately want to sync every frame for the cinematics
	GFX_Flush();

	start = end = 0;
	if ( r_speeds->integer ) {
		start = RI_ScaledMilliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0 ; ( 1 << i ) < cols ; i++ ) {
	}
	for ( j = 0 ; ( 1 << j ) < rows ; j++ ) {
	}
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows) {
		Com_Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

    RB_UploadCinematic( w, h, cols, rows, data, client, dirty );

	if ( r_speeds->integer ) {
		end = RI_ScaledMilliseconds();
		RI_Printf( PRINT_ALL, "UploadCinematic %i, %i: %i msec\n", cols, rows, end - start );
	}

	Set2DProjection();

    {
        float coords[4] = {
            x, y,
            x+w, y+h 
        };
        float texcoords[4] = {
            0.5f / cols, 0.5f / rows,
            ( cols - 0.5f ) / cols, ( rows - 0.5f ) / rows
        };
        float color[3] = {
            tr.identityLight,
            tr.identityLight,
            tr.identityLight 
        };

        GFX_DrawImage( tr.scratchImage[client], coords, texcoords, color );
    }
}

const void* RB_StretchRawCmd( const void* data )
{
	const stretchRawCommand_t* cmd = (const stretchRawCommand_t*) data;

    RB_StretchRaw(
        cmd->x, cmd->y,
        cmd->w, cmd->h,
        cmd->cols, cmd->rows,
        cmd->data, cmd->client,
        cmd->dirty );

	return cmd + 1;
}

/*
=============
@pjb: RB_UploadCinematic
=============
*/
void RB_UploadCinematic(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
    GFX_UpdateCinematic( tr.scratchImage[client], data, cols, rows, dirty );
    tr.scratchImage[client]->width = cols;
    tr.scratchImage[client]->height = rows;
}

const void* RB_UploadCinematicCmd( const void* data )
{
	const uploadCineCommand_t* cmd = (const uploadCineCommand_t*) data;

    RB_UploadCinematic(
        cmd->w, cmd->h,
        cmd->cols, cmd->rows,
        cmd->data, cmd->client,
        cmd->dirty );

	return cmd + 1;
}

/*
=============
RB_SetColor

=============
*/
const void	*RB_SetColor( const void *data ) {
	const setColorCommand_t	*cmd;

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;

	return (const void *)(cmd + 1);
}

/*
=============
RB_StretchPic
=============
*/
const void *RB_StretchPic ( const void *data ) {
	const stretchPicCommand_t	*cmd;
	shader_t *shader;
	int		numVerts, numIndexes;

	cmd = (const stretchPicCommand_t *)data;

    if ( !RB_IsColorPass() ) {
        goto end;
    }

	if ( !backEnd.projection2D ) {
		Set2DProjection();
	}

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;

        // @pjb: this can be called without any kind of setup when rendering UI, so
        // set it up now.
        if ( !RB_IsColorPass() ) {
            backEnd.viewParms.passFeatures |= PASSF_COLOR;
        }

		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	*(int *)tess.vertexColors[ numVerts ] =
		*(int *)tess.vertexColors[ numVerts + 1 ] =
		*(int *)tess.vertexColors[ numVerts + 2 ] =
		*(int *)tess.vertexColors[ numVerts + 3 ] = *(int *)backEnd.color2D;

	tess.xyz[ numVerts ][0] = cmd->x;
	tess.xyz[ numVerts ][1] = cmd->y;
	tess.xyz[ numVerts ][2] = 0;

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 1 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][1] = cmd->y;
	tess.xyz[ numVerts + 1 ][2] = 0;

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 2 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][2] = 0;

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	tess.xyz[ numVerts + 3 ][0] = cmd->x;
	tess.xyz[ numVerts + 3 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][2] = 0;

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

end:
	return (const void *)(cmd + 1);
}


/*
@pjb: RB_CheckForFlush
*/
const void	*RB_Flush( const void *data ) {
	const flushCommand_t	*cmd = (const flushCommand_t*)data;

	// sync with gl if needed
	if ( r_finish->integer == 1 && !backEnd.finishCalled ) {
		GFX_Flush();
		backEnd.finishCalled = qtrue;
	}
	if ( r_finish->integer == 0 ) {
		backEnd.finishCalled = qtrue;
	}

	return (const void *)(cmd + 1);
}

/*
=============
RB_DrawSurfs

=============
*/
const void	*RB_DrawSurfs( const void *data ) {
	const drawSurfsCommand_t	*cmd;

    // finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );
    
	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawBuffer

=============
*/
const void	*RB_DrawBuffer( const void *data ) {
	const drawBufferCommand_t	*cmd;

	cmd = (const drawBufferCommand_t *)data;

    // TODO: GFX_SetDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->integer ) {
        const float clearCol[] = { 1, 0, 0.5f, 1 };
        GFX_Clear( CLEAR_COLOR | CLEAR_DEPTH, clearCol, CLEAR_DEFAULT_STENCIL, CLEAR_DEFAULT_DEPTH );
	}

	return (const void *)(cmd + 1);
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void ) {
	int		i;
	image_t	*image;
	float	x, y, w, h;
	int		start, end;

	if ( !backEnd.projection2D ) {
		Set2DProjection();
	}

    GFX_Clear( CLEAR_COLOR, CLEAR_DEFAULT_COLOR, CLEAR_DEFAULT_STENCIL, CLEAR_DEFAULT_DEPTH );
	GFX_Flush();

	start = RI_ScaledMilliseconds();

	for ( i=0 ; i<tr.numImages ; i++ ) {
		image = tr.images[i];

		w = vdConfig.vidWidth / 20;
		h = vdConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
            // @pjb: todo: check me
			w *= image->width / 512.0f;
			h *= image->height / 512.0f;
		}

        {
            float coords[] = { x, y, x + w, y + h };
            float texcoords[] = { 0, 0, 1, 1 };   
            GFX_DrawImage( image, coords, texcoords, NULL );
        }
	}

	GFX_Flush();

	end = RI_ScaledMilliseconds();
	RI_Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );
}


/*
=============
RB_SwapBuffers

=============
*/
const void	*RB_SwapBuffers( const void *data ) {
	const swapBuffersCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// texture swapping test
	if ( r_showImages->integer ) {
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t *)data;

	if ( !backEnd.finishCalled ) {
		GFX_Flush();
	}

    GFX_EndFrame();

	backEnd.projection2D = qfalse;
	return (const void *)(cmd + 1);
}

/* 
============================================================================== 
 
						SCREEN SHOTS 

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

============================================================================== 
*/ 

/* 
================== 
RB_TakeScreenshot
================== 
*/  
void RB_TakeScreenshot( int x, int y, int width, int height, char *fileName ) {
	byte		*buffer;
	int			i, c, temp;
		
	buffer = Hunk_AllocateTempMemory(vdConfig.vidWidth*vdConfig.vidHeight*3+18);

	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	// TODO: GFX_ReadPixels( x, y, width, height, IMAGEFORMAT_RGB, buffer+18 ); 
	RI_Printf( PRINT_WARNING, "TODO: Screenshots not implemented.\n" );

	// swap rgb to bgr
	c = 18 + width * height * 3;
	for (i=18 ; i<c ; i+=3) {
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	FS_WriteFile( fileName, buffer, c );

	Hunk_FreeTempMemory( buffer );
}

/* 
================== 
RB_TakeScreenshotJPEG
================== 
*/  
void RB_TakeScreenshotJPEG( int x, int y, int width, int height, char *fileName ) {
	byte		*buffer;

	buffer = Hunk_AllocateTempMemory(vdConfig.vidWidth*vdConfig.vidHeight*4);

	//TODO: GFX_ReadPixels( x, y, width, height, IMAGEFORMAT_RGBA, buffer ); 
	RI_Printf(PRINT_WARNING, "TODO: Screenshots not implemented.\n");

	FS_WriteFile( fileName, buffer, 1 );		// create path
	SaveJPG( fileName, 95, vdConfig.vidWidth, vdConfig.vidHeight, buffer);

	Hunk_FreeTempMemory( buffer );
}

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void *RB_TakeScreenshotCmd( const void *data ) {
	const screenshotCommand_t	*cmd;
	
	cmd = (const screenshotCommand_t *)data;
	
	if (cmd->jpeg)
		RB_TakeScreenshotJPEG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	else
		RB_TakeScreenshot( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	
	return (const void *)(cmd + 1);	
}

/*
==================
@pjb: RB_DrawDebugCmd
==================
*/
const void *RB_DrawDebugCmd( const void *data ) {
	const drawDebugCommand_t	*cmd;
	
	cmd = (const drawDebugCommand_t *)data;

    Set2DProjection();
    GFX_DebugDraw();
	
	return (const void *)(cmd + 1);	
}

/*
==================
@pjb: RB_OpenList()
==================
*/
const void *RB_OpenList( const void *data ) {
	const startOfListCommand_t	*cmd;
	
	cmd = (const startOfListCommand_t *)data;

    GFX_BeginFrame();
	
	return (const void *)(cmd + 1);	
}

/*
====================
RB_ExecuteRenderCommands

This function will be called synchronously if running without
smp extensions, or asynchronously by another thread.
====================
*/
void RB_ExecuteRenderCommands( const void *data ) {
	int		t1, t2;
#ifdef _DEBUG
	int cmdID;
    static qboolean frameStarted = qfalse;
#endif

	t1 = RI_ScaledMilliseconds ();

	if ( !r_smp->integer || data == backEndData[0]->commands.cmds ) {
		backEnd.smpFrame = 0;
	} else {
		backEnd.smpFrame = 1;
	}
    
#ifdef _DEBUG
    cmdID = *(const int *)data;
    if (frameStarted == qfalse && 
        cmdID != RC_START_OF_LIST &&
        cmdID != RC_END_OF_LIST) {
        Com_Error(ERR_FATAL, "Need to open the render command list before calling draw commands (%d).", cmdID);
    }
    frameStarted = qtrue;
#endif

	while ( 1 ) {
		switch ( *(const int *)data ) {
		case RC_SET_COLOR:
			data = RB_SetColor( data );
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic( data );
			break;
		case RC_STRETCH_RAW: // @pjb
			data = RB_StretchRawCmd( data );
			break;
		case RC_UPLOAD_CINE: // @pjb
			data = RB_UploadCinematicCmd( data );
			break;
        case RC_FLUSH:
            data = RB_Flush( data );
            break;
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs( data );
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer( data );
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers( data );
			break;
		case RC_SCREENSHOT:
			data = RB_TakeScreenshotCmd( data );
			break;
        case RC_DEBUG: // @pjb: draw debug rendering
			data = RB_DrawDebugCmd( data );
			break;

        case RC_START_OF_LIST: // @pjb
            data = RB_OpenList( data );
            break;
        case RC_END_OF_LIST:
		default:
#ifdef _DEBUG
            // @pjb:
            frameStarted = qfalse;
#endif
			// stop rendering on this thread
			t2 = RI_ScaledMilliseconds ();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}
}


/*
================
RB_RenderThread
================
*/
void RB_RenderThread( void ) {
	const void	*data;

	// wait for either a rendering command or a quit command
	while ( 1 ) {
		// sleep until we have work to do
		data = RSMP_RendererSleep();

		if ( !data ) {
			return;	// all done, renderer is shutting down
		}

		renderThreadActive = qtrue;

		RB_ExecuteRenderCommands( data );

		renderThreadActive = qfalse;
	}
}

