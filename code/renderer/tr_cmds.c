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

volatile renderCommandList_t	*renderCommandList;

volatile qboolean	renderThreadActive;

void R_OpenCommandBuffer( void );

/*
=====================
R_PerformanceCounters
=====================
*/
void R_PerformanceCounters( void ) {
	if ( !r_speeds->integer ) {
		// clear the counters even if we aren't printing
		Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
		Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
		return;
	}

	if (r_speeds->integer == 1) {
		RI_Printf (PRINT_ALL, "%i/%i shaders/surfs %i leafs %i verts %i/%i tris %.2f mtex %.2f dc\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes, 
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3, 
			GFX_GetFrameImageMemoryUsage()/(1000000.0f), backEnd.pc.c_overDraw / (float)(vdConfig.vidWidth * vdConfig.vidHeight) ); 
	} else if (r_speeds->integer == 2) {
		RI_Printf (PRINT_ALL, "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out, 
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );
		RI_Printf (PRINT_ALL, "(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out, 
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out );
	} else if (r_speeds->integer == 3) {
		RI_Printf (PRINT_ALL, "viewcluster: %i\n", tr.viewCluster );
	} else if (r_speeds->integer == 4) {
		if ( backEnd.pc.c_dlightVertexes ) {
			RI_Printf (PRINT_ALL, "dlight srf:%i  culled:%i  verts:%i  tris:%i\n", 
				tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
				backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3 );
		}
	} 
	else if (r_speeds->integer == 5 )
	{
		RI_Printf( PRINT_ALL, "zFar: %.0f\n", tr.viewParms.zFar );
	}
	else if (r_speeds->integer == 6 )
	{
		RI_Printf( PRINT_ALL, "flare adds:%i tests:%i renders:%i\n", 
			backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders );
	}

	Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
	Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}


/*
====================
R_InitCommandBuffers
====================
*/
void R_InitCommandBuffers( void ) {
	vdConfig.smpActive = qfalse;
	if ( r_smp->integer ) {
		RI_Printf( PRINT_ALL, "Trying SMP acceleration...\n" );
		if ( RSMP_SpawnRenderThread( RB_RenderThread ) ) {
			RI_Printf( PRINT_ALL, "...succeeded.\n" );
			vdConfig.smpActive = qtrue;
		} else {
			RI_Printf( PRINT_ALL, "...failed.\n" );
		}
	}

    // @pjb
    R_OpenCommandBuffer();
}

/*
====================
R_ShutdownCommandBuffers
====================
*/
void R_ShutdownCommandBuffers( void ) {
	// kill the rendering thread
	if ( vdConfig.smpActive ) {
        RSMP_WakeRenderer( NULL );
		vdConfig.smpActive = qfalse;
	}
}

/*
============
@pjb: R_OpenCommandBuffer

make sure there is enough command space, waiting on the
render thread if needed.
============
*/
void R_OpenCommandBuffer() {
    renderCommandList_t	*cmdList;
    startOfListCommand_t* cmd;

    if (backEndData[tr.smpFrame]->open == qfalse) {
        backEndData[tr.smpFrame]->open = qtrue;
        cmdList = &backEndData[tr.smpFrame]->commands;
        assert(cmdList->used == 0);

        cmd = (startOfListCommand_t*)(cmdList->cmds + cmdList->used);
        cmdList->used += sizeof(startOfListCommand_t);

        cmd->commandId = RC_START_OF_LIST;
    }
}

/*
============
@pjb: R_CloseCommandBuffer
============
*/
renderCommandList_t* R_CloseCommandBuffer() {
    renderCommandList_t	*cmdList;

    cmdList = &backEndData[tr.smpFrame]->commands;
    assert(cmdList); // bk001205
    assert(backEndData[tr.smpFrame]->open == qtrue ||
           cmdList->used == 0);

    // add an end-of-list command
    *(int *)(cmdList->cmds + cmdList->used) = RC_END_OF_LIST;

    backEndData[tr.smpFrame]->open = qfalse;

    // clear it out, in case this is a sync and not a buffer flip
    cmdList->used = 0;

    return cmdList;
}

/*
====================
R_IssueRenderCommands
====================
*/
int	c_blockedOnRender;
int	c_blockedOnMain;

void R_IssueRenderCommands( qboolean runPerformanceCounters ) {
	renderCommandList_t	*cmdList;

    // @pjb: close the list
    cmdList = R_CloseCommandBuffer();

	if ( vdConfig.smpActive ) {
		// if the render thread is not idle, wait for it
		if ( renderThreadActive ) {
			c_blockedOnRender++;
			if ( r_showSmp->integer ) {
				RI_Printf( PRINT_ALL, "R" );
			}
		} else {
			c_blockedOnMain++;
			if ( r_showSmp->integer ) {
				RI_Printf( PRINT_ALL, "." );
			}
		}

		// sleep until the renderer has completed
		RSMP_FrontEndSleep();
	}

	// at this point, the back end thread is idle, so it is ok
	// to look at it's performance counters
	if ( runPerformanceCounters ) {
		R_PerformanceCounters();
	}

	// actually start the commands going
	if ( !r_skipBackEnd->integer ) {
		// let it start on the new batch
		if ( !vdConfig.smpActive ) {
			RB_ExecuteRenderCommands( cmdList->cmds );
		} else {
            RSMP_WakeRenderer( cmdList );
		}
	}
}


/*
====================
R_SyncRenderThread

Issue any pending commands and wait for them to complete.
After exiting, the render thread will have completed its work
and will remain idle and the main thread is free to issue
OpenGL calls until R_IssueRenderCommands is called.
====================
*/
void R_SyncRenderThread( void ) {
	if ( !tr.registered ) {
		return;
	}
	R_IssueRenderCommands( qfalse );

	if ( !vdConfig.smpActive ) {
		return;
	}
	RSMP_FrontEndSleep();
}

/*
============
R_GetCommandBuffer

make sure there is enough command space, waiting on the
render thread if needed.
============
*/
void *R_GetCommandBuffer( int bytes ) {
	renderCommandList_t	*cmdList;

	cmdList = &backEndData[tr.smpFrame]->commands;

	if (!backEndData[tr.smpFrame]->open) {
		R_OpenCommandBuffer();
	}

	assert(cmdList->used >= 4);

	// always leave room for the end of list command
	if ( cmdList->used + bytes + 4 > MAX_RENDER_COMMANDS ) {
		if ( bytes > MAX_RENDER_COMMANDS - 4 ) {
			Com_Error( ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes );
		}
		// if we run out of room, just start dropping commands
		return NULL;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
}


/*
=============
R_AddDrawSurfCmd

=============
*/
void	R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	drawSurfsCommand_t	*cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_SURFS;

	cmd->drawSurfs = drawSurfs;
	cmd->numDrawSurfs = numDrawSurfs;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

/*
=============
@pjb: R_AddFlushCmd

=============
*/
void	R_AddFlushCmd( void ) {
	flushCommand_t	*cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_FLUSH;
}


/*
=============
RE_SetColor

Passing NULL will set the color to white
=============
*/
void	RE_SetColor( const float *rgba ) {
	setColorCommand_t	*cmd;

  if ( !tr.registered ) {
    return;
  }
	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SET_COLOR;
	if ( !rgba ) {
		static float colorWhite[4] = { 1, 1, 1, 1 };

		rgba = colorWhite;
	}

	cmd->color[0] = rgba[0];
	cmd->color[1] = rgba[1];
	cmd->color[2] = rgba[2];
	cmd->color[3] = rgba[3];
}


/*
=============
RE_StretchPic
=============
*/
void RE_StretchPic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	stretchPicCommand_t	*cmd;

  if (!tr.registered) {
    return;
  }
	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_STRETCH_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

/*
=============
@pjb: R_DrawDebugCmd

=============
*/
void	R_DrawDebugCmd( void ) {
	drawDebugCommand_t	*cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DEBUG;
}


/*
====================
RE_BeginFrame

If running in stereo, RE_BeginFrame will be called twice
for each RE_EndFrame
====================
*/
void RE_BeginFrame( stereoFrame_t stereoFrame ) {
	drawBufferCommand_t	*drawBufferCmd;

	if ( !tr.registered ) {
		return;
	}
	backEnd.finishCalled = qfalse;

	tr.frameCount++;
	tr.frameSceneNum = 0;

	//
	// gamma stuff
	//
	if ( r_gamma->modified ) {
		r_gamma->modified = qfalse;

		R_SyncRenderThread();
		R_SetColorMappings();
	}

    // check for errors
    if ( !r_ignoreGLErrors->integer ) {
        size_t	err;

		R_SyncRenderThread();
        if ( ( err = GFX_LastError() ) != 0 ) {
            Com_Error( ERR_FATAL, "RE_BeginFrame() - glGetError() failed (0x%x)!\n", err );
        }
    }

    //
    // @pjb: check the command queue is open
    //
    R_OpenCommandBuffer();

	//
	// draw buffer stuff
	//
    drawBufferCmd = R_GetCommandBuffer(sizeof(*drawBufferCmd));
    if (!drawBufferCmd) {
        return;
    }
    drawBufferCmd->commandId = RC_DRAW_BUFFER;

	if ( vdConfig.stereoEnabled ) {
		if ( stereoFrame == STEREO_LEFT ) {
            drawBufferCmd->buffer = (int)DRAW_BUFFER_BACK_LEFT;
		} else if ( stereoFrame == STEREO_RIGHT ) {
            drawBufferCmd->buffer = (int)DRAW_BUFFER_BACK_RIGHT;
		} else {
			Com_Error( ERR_FATAL, "RE_BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame );
		}
	} else {
		if ( stereoFrame != STEREO_CENTER ) {
			Com_Error( ERR_FATAL, "RE_BeginFrame: Stereo is disabled, but stereoFrame was %i", stereoFrame );
		}
		if ( !Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) ) {
            drawBufferCmd->buffer = (int)DRAW_BUFFER_FRONT;
		} else {
            drawBufferCmd->buffer = (int)DRAW_BUFFER_BACK;
		}
	}
}


/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void RE_EndFrame( int *frontEndMsec, int *backEndMsec ) {
	swapBuffersCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SWAP_BUFFERS;

	R_IssueRenderCommands( qtrue );

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current ones
	R_ToggleSmpFrame();

    // @pjb: open a new command list
    R_OpenCommandBuffer();

	if ( frontEndMsec ) {
		*frontEndMsec = tr.frontEndMsec;
	}
	tr.frontEndMsec = 0;
	if ( backEndMsec ) {
		*backEndMsec = backEnd.pc.msec;
	}
	backEnd.pc.msec = 0;
}


/*
=============
RE_StretchRaw

@pjb: moved here so we can pipe it to the back-end instead of rendering from the game thread.
DO NOT FREE THIS MEMORY - IT NEEDS TO STAY AROUND FOR THE RENDER THREAD TO CONSUME

Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
    stretchRawCommand_t* cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_STRETCH_RAW;
    cmd->x = x;
    cmd->y = y;
    cmd->w = w;
    cmd->h = h;
    cmd->cols = cols;
    cmd->rows = rows;
    cmd->data = data;
    cmd->client = client;
    cmd->dirty = dirty;    
}

/*
=============
RE_UploadCinematic
@pjb: moved here so we can pipe it to the back-end instead of rendering from the game thread.
DO NOT FREE THIS MEMORY - IT NEEDS TO STAY AROUND FOR THE RENDER THREAD TO CONSUME
=============
*/
void RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
    uploadCineCommand_t* cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_UPLOAD_CINE;
    cmd->w = w;
    cmd->h = h;
    cmd->cols = cols;
    cmd->rows = rows;
    cmd->data = data;
    cmd->client = client;
    cmd->dirty = dirty;    
}
