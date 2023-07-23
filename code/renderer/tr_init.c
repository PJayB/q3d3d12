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
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"
#include "tr_layer.h"
#include "../d3d12/D3D12QAPI.h"

vdconfig_t	vdConfig;

cvar_t	*r_flareSize;
cvar_t	*r_flareFade;

cvar_t	*r_railWidth;
cvar_t	*r_railCoreWidth;
cvar_t	*r_railSegmentLength;

cvar_t	*r_ignoreFastPath;

cvar_t	*r_verbose;
cvar_t	*r_ignore;

cvar_t	*r_displayRefresh;

cvar_t	*r_detailTextures;

cvar_t	*r_znear;

cvar_t	*r_smp;
cvar_t	*r_showSmp;
cvar_t	*r_skipBackEnd;

cvar_t	*r_ignorehwgamma;
cvar_t	*r_measureOverdraw;

cvar_t	*r_inGameVideo;
cvar_t	*r_fastsky;
cvar_t	*r_drawSun;
cvar_t	*r_dynamiclight;
cvar_t	*r_dlightBacks;

cvar_t	*r_lodbias;
cvar_t	*r_lodscale;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_facePlaneCull;
cvar_t	*r_showcluster;
cvar_t	*r_nocurves;

cvar_t	*r_allowExtensions;

cvar_t	*r_ext_compressed_textures;
cvar_t	*r_ext_gamma_control;
cvar_t	*r_ext_multitexture;
cvar_t	*r_ext_compiled_vertex_array;
cvar_t	*r_ext_texture_env_add;

cvar_t	*r_ignoreGLErrors;
cvar_t	*r_logFile;

cvar_t	*r_stencilbits;
cvar_t	*r_depthbits;
cvar_t	*r_colorbits;
cvar_t	*r_stereo;
cvar_t	*r_primitives;
cvar_t	*r_texturebits;

cvar_t	*r_drawBuffer;
cvar_t  *r_glDriver;
cvar_t	*r_lightmap;
cvar_t	*r_showlighting;
cvar_t	*r_vertexLight;
cvar_t	*r_uiFullScreen;
cvar_t	*r_shadows;
cvar_t	*r_flares;
cvar_t	*r_mode;
cvar_t	*r_nobind;
cvar_t	*r_singleShader;
cvar_t	*r_roundImagesDown;
cvar_t	*r_colorMipLevels;
cvar_t	*r_picmip;
cvar_t	*r_showtris;
cvar_t	*r_showsky;
cvar_t	*r_shownormals;
cvar_t	*r_finish;
cvar_t	*r_clear;
cvar_t	*r_swapInterval;
cvar_t	*r_textureMode;
cvar_t	*r_offsetFactor;
cvar_t	*r_offsetUnits;
cvar_t	*r_gamma;
cvar_t	*r_intensity;
cvar_t	*r_lockpvs;
cvar_t	*r_noportals;
cvar_t	*r_portalOnly;

cvar_t	*r_subdivisions;
cvar_t	*r_lodCurveError;

cvar_t	*r_fullscreen;

cvar_t	*r_customwidth;
cvar_t	*r_customheight;
cvar_t	*r_customaspect;

cvar_t	*r_overBrightBits;
cvar_t	*r_mapOverBrightBits;

cvar_t	*r_debugSurface;
cvar_t	*r_simpleMipMaps;

cvar_t	*r_showImages;

cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_debugLight;
cvar_t	*r_debugSort;
cvar_t	*r_printShaders;
cvar_t	*r_saveFontData;

cvar_t	*r_maxpolys;
int		max_polys;
cvar_t	*r_maxpolyverts;
int		max_polyverts;

// @pjb: fixed hardware?
cvar_t  *r_fixedhw;

static void AssertCvarRange( cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral )
{
	if ( shouldBeIntegral )
	{
		if ( ( int ) cv->value != cv->integer )
		{
			RI_Printf( PRINT_WARNING, "WARNING: cvar '%s' must be integral (%f)\n", cv->name, cv->value );
			Cvar_Set( cv->name, va( "%d", cv->integer ) );
		}
	}

	if ( cv->value < minVal )
	{
		RI_Printf( PRINT_WARNING, "WARNING: cvar '%s' out of range (%f < %f)\n", cv->name, cv->value, minVal );
		Cvar_Set( cv->name, va( "%f", minVal ) );
	}
	else if ( cv->value > maxVal )
	{
		RI_Printf( PRINT_WARNING, "WARNING: cvar '%s' out of range (%f > %f)\n", cv->name, cv->value, maxVal );
		Cvar_Set( cv->name, va( "%f", maxVal ) );
	}
}

/* 
    @pjb: InitDriver
*/
void InitDriver( void )
{
    R_ResetGraphicsLayer();

#if defined(Q_HAS_D3D12)
	D3D12_DriverInit(&tr_graphicsApi);
#else
#   error Must define at least one renderer.
#endif

    R_ValidateGraphicsLayer();

	// init command buffers and SMP
	R_InitCommandBuffers();
}

void ShutdownDriver( void )
{
    GFX_Shutdown();
}


/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
    const char *description;
    int         width, height;
	float		pixelAspect;		// pixel width / height
} vidmode_t;

vidmode_t r_vidModes[] =
{
    { "Mode  0: 1280x720",		1280,	720,	1 },
    { "Mode  1: 1024x768",		1024,	768,	1 },
    { "Mode  2: 1280x768",		1280,	768,	1 },
    { "Mode  3: 1360x768",		1360,	768,	1 },
    { "Mode  4: 1366x768",		1366,	768,	1 },
    { "Mode  5: 1280x800",		1280,	800,	1 },
    { "Mode  6: 1152x864",		1152,	864,	1 },
    { "Mode  7: 1440x900",		1440,	900,	1 },
    { "Mode  8: 1600x900",		1600,	900,	1 },
    { "Mode  9: 1280x960",		1280,	960,	1 },
    { "Mode 10: 1776x1000",		1776,	1000,	1 },
    { "Mode 11: 1280x1024",		1280,	1024,	1 },
    { "Mode 12: 1360x1024",		1360,	1024,	1 },
    { "Mode 13: 1400x1050",		1400,	1050,	1 },
    { "Mode 14: 1680x1050",		1680,	1050,	1 },
    { "Mode 15: 1920x1080",		1920,	1080,	1 },
    { "Mode 16: 2560x1440",		2560,	1440,	1 },
    { "Mode 17: 3840x2160",             3840,	2160,	1 }
};
static int	s_numVidModes = ( sizeof( r_vidModes ) / sizeof( r_vidModes[0] ) );

qboolean R_GetModeInfo( int *width, int *height, float *windowAspect, int mode ) {
	vidmode_t	*vm;

    if ( mode < -1 ) {
        return qfalse;
	}
	if ( mode >= s_numVidModes ) {
		return qfalse;
	}

	if ( mode == -1 ) {
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		*windowAspect = r_customaspect->value;
		return qtrue;
	}

	vm = &r_vidModes[mode];

    *width  = vm->width;
    *height = vm->height;
    *windowAspect = (float)vm->width / ( vm->height * vm->pixelAspect );

    return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void )
{
	int i;

	RI_Printf( PRINT_ALL, "\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		RI_Printf( PRINT_ALL, "%s\n", r_vidModes[i].description );
	}
	RI_Printf( PRINT_ALL, "\n" );
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot( int x, int y, int width, int height, char *name, qboolean jpeg ) {
	static char	fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
	screenshotCommand_t	*cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	Q_strncpyz( fileName, name, sizeof(fileName) );
	cmd->fileName = fileName;
	cmd->jpeg = jpeg;
}

/* 
================== 
R_ScreenshotFilename
================== 
*/  
void R_ScreenshotFilename( int lastNumber, char *fileName ) {
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.tga" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga"
		, a, b, c, d );
}

/* 
================== 
R_ScreenshotFilename
================== 
*/  
void R_ScreenshotFilenameJPEG( int lastNumber, char *fileName ) {
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.jpg" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg"
		, a, b, c, d );
}

/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
void R_LevelShot( void ) {
	char		checkname[MAX_OSPATH];
	byte		*buffer;
	byte		*source;
	byte		*src, *dst;
	int			x, y;
	int			r, g, b;
	float		xScale, yScale;
	int			xx, yy;

	sprintf( checkname, "levelshots/%s.tga", tr.world->baseName );

	source = Hunk_AllocateTempMemory( vdConfig.vidWidth * vdConfig.vidHeight * 3 );

	buffer = Hunk_AllocateTempMemory( 128 * 128*3 + 18);
	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = 128;
	buffer[14] = 128;
	buffer[16] = 24;	// pixel size

    //GFX_ReadPixels( 0, 0, vdConfig.vidWidth, vdConfig.vidHeight, IMAGEFORMAT_RGB, source ); 
	RI_Printf(PRINT_WARNING, "TODO: Screenshots not implemented.\n");

	// resample from source
	xScale = vdConfig.vidWidth / 512.0f;
	yScale = vdConfig.vidHeight / 384.0f;
	for ( y = 0 ; y < 128 ; y++ ) {
		for ( x = 0 ; x < 128 ; x++ ) {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ ) {
				for ( xx = 0 ; xx < 4 ; xx++ ) {
					src = source + 3 * ( vdConfig.vidWidth * (int)( (y*3+yy)*yScale ) + (int)( (x*4+xx)*xScale ) );
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst = buffer + 18 + 3 * ( y * 128 + x );
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	FS_WriteFile( checkname, buffer, 128 * 128*3 + 18 );

	Hunk_FreeTempMemory( buffer );
	Hunk_FreeTempMemory( source );

	RI_Printf( PRINT_ALL, "Wrote %s\n", checkname );
}

/* 
================== 
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
================== 
*/  
void R_ScreenShot_f (void) {
	char	checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !strcmp( Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilename( lastNumber, checkname );

      if (!FS_FileExists( checkname ))
      {
        break; // file doesn't exist
      }
		}

		if ( lastNumber >= 9999 ) {
			RI_Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n"); 
			return;
 		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, vdConfig.vidWidth, vdConfig.vidHeight, checkname, qfalse );

	if ( !silent ) {
		RI_Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
} 

void R_ScreenShotJPEG_f (void) {
	char		checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !strcmp( Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.jpg", Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilenameJPEG( lastNumber, checkname );

      if (!FS_FileExists( checkname ))
      {
        break; // file doesn't exist
      }
		}

		if ( lastNumber == 10000 ) {
			RI_Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n"); 
			return;
 		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, vdConfig.vidWidth, vdConfig.vidHeight, checkname, qtrue );

	if ( !silent ) {
		RI_Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
} 

// @pjb: print info about the driver
void R_GfxInfo( void )
{
    GFX_GraphicsInfo();
}


//============================================================================

/*
===============
R_Register
===============
*/
void R_Register( void ) 
{
	//
	// latched and archived variables
	//
#if defined(Q_WINRT_FIXED_DISPLAY)
    r_fixedhw = Cvar_Get( "r_fixedhw", "1", CVAR_LATCH);
#else
    r_fixedhw = Cvar_Get( "r_fixedhw", "0", CVAR_LATCH);
#endif
#if defined(Q_HAS_D3D12)
    const char* defaultD3DVersion = "12";
#else
    const char* defaultD3DVersion = "11";
#endif
	r_glDriver = Cvar_Get( "r_glDriver", OPENGL_DRIVER_NAME, CVAR_ARCHIVE | CVAR_LATCH );
	r_allowExtensions = Cvar_Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compressed_textures = Cvar_Get( "r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_gamma_control = Cvar_Get( "r_ext_gamma_control", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_multitexture = Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compiled_vertex_array = Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH);
#ifdef __linux__ // broken on linux
	r_ext_texture_env_add = Cvar_Get( "r_ext_texture_env_add", "0", CVAR_ARCHIVE | CVAR_LATCH);
#else
	r_ext_texture_env_add = Cvar_Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);
#endif

	r_picmip = Cvar_Get ("r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_roundImagesDown = Cvar_Get ("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorMipLevels = Cvar_Get ("r_colorMipLevels", "0", CVAR_LATCH );
	AssertCvarRange( r_picmip, 0, 16, qtrue );
	r_detailTextures = Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_texturebits = Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorbits = Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_stereo = Cvar_Get( "r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH );
#ifdef __linux__
	r_stencilbits = Cvar_Get( "r_stencilbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
#else
	r_stencilbits = Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH );
#endif
	r_depthbits = Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_overBrightBits = Cvar_Get ("r_overBrightBits", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ignorehwgamma = Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = Cvar_Get( "r_mode", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_fullscreen = Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_customwidth = Cvar_Get( "r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = Cvar_Get( "r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH );
	r_customaspect = Cvar_Get( "r_customaspect", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_simpleMipMaps = Cvar_Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_vertexLight = Cvar_Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_uiFullScreen = Cvar_Get( "r_uifullscreen", "0", 0);
	r_subdivisions = Cvar_Get ("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);
#if (defined(MACOS_X) || defined(__linux__)) && defined(SMP)
  // Default to using SMP on Mac OS X or Linux if we have multiple processors
	r_smp = Cvar_Get( "r_smp", Sys_ProcessorCount() > 1 ? "1" : "0", CVAR_ARCHIVE | CVAR_LATCH);
#else        
	r_smp = Cvar_Get( "r_smp", "1", CVAR_ARCHIVE | CVAR_LATCH);
#endif
	r_ignoreFastPath = Cvar_Get( "r_ignoreFastPath", "1", CVAR_ARCHIVE | CVAR_LATCH );

	//
	// temporary latched variables that can only change over a restart
	//
	r_displayRefresh = Cvar_Get( "r_displayRefresh", "0", CVAR_LATCH );
	AssertCvarRange( r_displayRefresh, 0, 200, qtrue );
	r_fullbright = Cvar_Get ("r_fullbright", "0", CVAR_LATCH|CVAR_CHEAT );
	r_mapOverBrightBits = Cvar_Get ("r_mapOverBrightBits", "2", CVAR_LATCH );
	r_intensity = Cvar_Get ("r_intensity", "1", CVAR_LATCH );
	r_singleShader = Cvar_Get ("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH );

	//
	// archived variables that can change at any time
	//
	r_lodCurveError = Cvar_Get( "r_lodCurveError", "250", CVAR_ARCHIVE|CVAR_CHEAT );
	r_lodbias = Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE | CVAR_SYSTEM_SET );
	r_flares = Cvar_Get ("r_flares", "0", CVAR_ARCHIVE | CVAR_SYSTEM_SET );
	r_znear = Cvar_Get( "r_znear", "4", CVAR_CHEAT );
	AssertCvarRange( r_znear, 0.001f, 200, qtrue );
	r_ignoreGLErrors = Cvar_Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE | CVAR_SYSTEM_SET );
	r_fastsky = Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE | CVAR_SYSTEM_SET );
	r_inGameVideo = Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE | CVAR_SYSTEM_SET );
	r_drawSun = Cvar_Get( "r_drawSun", "0", CVAR_ARCHIVE | CVAR_SYSTEM_SET );
	r_dynamiclight = Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE | CVAR_SYSTEM_SET );
	r_dlightBacks = Cvar_Get( "r_dlightBacks", "1", CVAR_ARCHIVE | CVAR_SYSTEM_SET );
	r_finish = Cvar_Get ("r_finish", "0", CVAR_ARCHIVE | CVAR_SYSTEM_SET);
	r_textureMode = Cvar_Get( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE | CVAR_SYSTEM_SET );
	r_swapInterval = Cvar_Get( "r_swapInterval", "0", CVAR_ARCHIVE );
#ifdef __MACOS__
	r_gamma = Cvar_Get( "r_gamma", "1.2", CVAR_ARCHIVE );
#else
	r_gamma = Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE );
#endif
	r_facePlaneCull = Cvar_Get ("r_facePlaneCull", "1", CVAR_ARCHIVE | CVAR_SYSTEM_SET );

	r_railWidth = Cvar_Get( "r_railWidth", "16", CVAR_ARCHIVE | CVAR_SYSTEM_SET );
	r_railCoreWidth = Cvar_Get( "r_railCoreWidth", "6", CVAR_ARCHIVE | CVAR_SYSTEM_SET );
	r_railSegmentLength = Cvar_Get( "r_railSegmentLength", "32", CVAR_ARCHIVE | CVAR_SYSTEM_SET );

	r_primitives = Cvar_Get( "r_primitives", "0", CVAR_ARCHIVE | CVAR_SYSTEM_SET );

	r_ambientScale = Cvar_Get( "r_ambientScale", "0.6", CVAR_CHEAT );
	r_directedScale = Cvar_Get( "r_directedScale", "1", CVAR_CHEAT );

	//
	// temporary variables that can change at any time
	//
	r_showImages = Cvar_Get( "r_showImages", "0", CVAR_TEMP );

	r_debugLight = Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
	r_debugSort = Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );
	r_printShaders = Cvar_Get( "r_printShaders", "0", 0 );
	r_saveFontData = Cvar_Get( "r_saveFontData", "0", 0 );

	r_nocurves = Cvar_Get ("r_nocurves", "0", CVAR_CHEAT );
	r_drawworld = Cvar_Get ("r_drawworld", "1", CVAR_CHEAT );
	r_lightmap = Cvar_Get ("r_lightmap", "0", 0 );
	r_showlighting = Cvar_Get ("r_showlighting", "0", 0 );
	r_portalOnly = Cvar_Get ("r_portalOnly", "0", CVAR_CHEAT );

	r_flareSize = Cvar_Get ("r_flareSize", "40", CVAR_CHEAT);
	r_flareFade = Cvar_Get ("r_flareFade", "7", CVAR_CHEAT);

	r_showSmp = Cvar_Get ("r_showSmp", "0", CVAR_CHEAT);
	r_skipBackEnd = Cvar_Get ("r_skipBackEnd", "0", CVAR_CHEAT);

	r_measureOverdraw = Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
	r_lodscale = Cvar_Get( "r_lodscale", "5", CVAR_CHEAT );
	r_norefresh = Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities = Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );
	r_ignore = Cvar_Get( "r_ignore", "1", CVAR_CHEAT );
	r_nocull = Cvar_Get ("r_nocull", "0", CVAR_CHEAT);
	r_novis = Cvar_Get ("r_novis", "0", CVAR_CHEAT);
	r_showcluster = Cvar_Get ("r_showcluster", "0", CVAR_CHEAT);
	r_speeds = Cvar_Get ("r_speeds", "0", CVAR_CHEAT);
	r_verbose = Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
	r_logFile = Cvar_Get( "r_logFile", "0", CVAR_CHEAT );
	r_debugSurface = Cvar_Get ("r_debugSurface", "0", CVAR_CHEAT);
	r_nobind = Cvar_Get ("r_nobind", "0", CVAR_CHEAT);
	r_showtris = Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
	r_showsky = Cvar_Get ("r_showsky", "0", CVAR_CHEAT);
	r_shownormals = Cvar_Get ("r_shownormals", "0", CVAR_CHEAT);
	r_clear = Cvar_Get ("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor = Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
	r_offsetUnits = Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );
	r_drawBuffer = Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
	r_lockpvs = Cvar_Get ("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals = Cvar_Get ("r_noportals", "0", CVAR_CHEAT);
	r_shadows = Cvar_Get( "cg_shadows", "1", 0 );

	r_maxpolys = Cvar_Get( "r_maxpolys", va("%d", MAX_POLYS), 0);
	r_maxpolyverts = Cvar_Get( "r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);

	// make sure all the commands added here are also
	// removed in R_Shutdown
	Cmd_AddCommand( "imagelist", R_ImageList_f );
	Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	Cmd_AddCommand( "skinlist", R_SkinList_f );
	Cmd_AddCommand( "modellist", R_Modellist_f );
	Cmd_AddCommand( "modelist", R_ModeList_f );
	Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
	Cmd_AddCommand( "gfxinfo", R_GfxInfo );
}

// @pjb: this used to be done way down inside GLimp, but we have to do it much earlier now
void R_InitVMode( void )
{
    RI_Printf( PRINT_ALL, "...setting mode %d:", r_mode->integer );

	if ( !R_GetModeInfo( &vdConfig.vidWidth, &vdConfig.vidHeight, &vdConfig.windowAspect, r_mode->integer ) )
	{
		RI_Printf( PRINT_ALL, " invalid mode. Setting to default.\n" );

        // @pjb: apparently this is a safe fallback
		r_mode->integer = 3;
        R_GetModeInfo( &vdConfig.vidWidth, &vdConfig.vidHeight, &vdConfig.windowAspect, r_mode->integer );
	}

	RI_Printf( PRINT_ALL, " %dx%d\n", vdConfig.vidWidth, vdConfig.vidHeight );
}

/*
===============
R_Init
===============
*/
void R_Init( void ) {	
	size_t	err;
	int i;
	byte *ptr;

	RI_Printf( PRINT_ALL, "----- R_Init -----\n" );

	// clear all our internal state
	Com_Memset( &tr, 0, sizeof( tr ) );
	Com_Memset( &backEnd, 0, sizeof( backEnd ) );
	Com_Memset( &tess, 0, sizeof( tess ) );

    // @pjb: set up default rendering
    backEnd.viewParms.passFeatures = PASSF_ALL;

//	Swap_Init();

	if ( (int)tess.xyz & 15 ) {
		Com_Printf( "WARNING: tess.xyz not 16 byte aligned\n" );
	}
	Com_Memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );

	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 )
		{
			if ( i < FUNCTABLE_SIZE / 4 )
			{
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
		}
	}

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	max_polys = r_maxpolys->integer;
	if (max_polys < MAX_POLYS)
		max_polys = MAX_POLYS;

	max_polyverts = r_maxpolyverts->integer;
	if (max_polyverts < MAX_POLYVERTS)
		max_polyverts = MAX_POLYVERTS;

	ptr = Hunk_Alloc( sizeof( *backEndData[0] ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
	backEndData[0] = (backEndData_t *) ptr;
	backEndData[0]->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData[0] ));
	backEndData[0]->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData[0] ) + sizeof(srfPoly_t) * max_polys);
	if ( r_smp->integer ) {
		ptr = Hunk_Alloc( sizeof( *backEndData[1] ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
		backEndData[1] = (backEndData_t *) ptr;
		backEndData[1]->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData[1] ));
		backEndData[1]->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData[1] ) + sizeof(srfPoly_t) * max_polys);
	} else {
		backEndData[1] = NULL;
	}
	R_ToggleSmpFrame();

    //
	// print out informational messages
	//
#ifndef Q_WINRT_PLATFORM
    R_InitVMode();
#endif

    // @pjb: Select our driver
    InitDriver();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();


	err = GFX_LastError();
	if ( err != 0 )
		RI_Printf (PRINT_WARNING, "Graphics driver error set: 0x%x\n", err);

	RI_Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow ) {	

	RI_Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );

	Cmd_RemoveCommand ("modellist");
	Cmd_RemoveCommand ("screenshotJPEG");
	Cmd_RemoveCommand ("screenshot");
	Cmd_RemoveCommand ("imagelist");
	Cmd_RemoveCommand ("shaderlist");
	Cmd_RemoveCommand ("skinlist");
	Cmd_RemoveCommand ("gfxinfo");
	Cmd_RemoveCommand( "modelist" );
	Cmd_RemoveCommand( "shaderstate" );


	if ( tr.registered ) {
		R_SyncRenderThread();
		R_ShutdownCommandBuffers();

        // @pjb: makes more sense to unbind BEFORE deleting all the textures :P
        GFX_UnbindResources();
		R_DeleteTextures();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {
        ShutdownDriver();
	}

	tr.registered = qfalse;
}

/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void ) {
	R_SyncRenderThread();
	if (!Sys_LowPhysicalMemory()) {
		RB_ShowImages();
	}
}

