#include "tr_local.h"
#include "qgl.h"
#include "gl_common.h"

#include <assert.h> // @pjb: remove me

typedef struct glimage_s
{
    GLuint          texnum;
	int			    TMU;				// only needed for voodoo2
	int			    internalFormat;
	int			    frameUsed;			// for texture usage in frame statistics
	int			    uploadWidth, uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
} glimage_t;

static glimage_t glimages[MAX_DRAWIMAGES];

byte	mipBlendColors[16][4] = {
	{0,0,0,0},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
};


/*
===============
Upload32

===============
*/
extern qboolean charSet;
static void Upload32( unsigned *data, 
						  int width, int height, 
						  qboolean mipmap, 
						  qboolean picmip, 
							qboolean lightMap,
						  int *format, 
						  int *pUploadWidth, int *pUploadHeight )
{
	int			samples;
	unsigned	*scaledBuffer = NULL;
	unsigned	*resampledBuffer = NULL;
	int			scaled_width, scaled_height;
	int			i, c;
	byte		*scan;
	GLenum		internalFormat = GL_RGB;
	float		rMax = 0, gMax = 0, bMax = 0;

	//
	// convert to exact power of 2 sizes
	//
	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;
	if ( r_roundImagesDown->integer && scaled_width > width )
		scaled_width >>= 1;
	if ( r_roundImagesDown->integer && scaled_height > height )
		scaled_height >>= 1;

	if ( scaled_width != width || scaled_height != height ) {
		resampledBuffer = Hunk_AllocateTempMemory( scaled_width * scaled_height * 4 );
		R_ResampleTexture(data, width, height, resampledBuffer, scaled_width, scaled_height);
		data = resampledBuffer;
		width = scaled_width;
		height = scaled_height;
	}

	//
	// perform optional picmip operation
	//
	if ( picmip ) {
		scaled_width >>= r_picmip->integer;
		scaled_height >>= r_picmip->integer;
	}

	//
	// clamp to minimum size
	//
	if (scaled_width < 1) {
		scaled_width = 1;
	}
	if (scaled_height < 1) {
		scaled_height = 1;
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	//
	while ( scaled_width > vdConfig.maxTextureSize
		|| scaled_height > vdConfig.maxTextureSize ) {
		scaled_width >>= 1;
		scaled_height >>= 1;
	}

	scaledBuffer = Hunk_AllocateTempMemory( sizeof( unsigned ) * scaled_width * scaled_height );

	//
	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	//
	c = width*height;
	scan = ((byte *)data);
	samples = 3;
	if (!lightMap) {
		for ( i = 0; i < c; i++ )
		{
			if ( scan[i*4+0] > rMax )
			{
				rMax = scan[i*4+0];
			}
			if ( scan[i*4+1] > gMax )
			{
				gMax = scan[i*4+1];
			}
			if ( scan[i*4+2] > bMax )
			{
				bMax = scan[i*4+2];
			}
			if ( scan[i*4 + 3] != 255 ) 
			{
				samples = 4;
				break;
			}
		}
		// select proper internal format
		if ( samples == 3 )
		{
			if ( vdConfig.textureCompression == TC_S3TC )
			{
				internalFormat = GL_RGB4_S3TC;
			}
			else if ( r_texturebits->integer == 16 )
			{
				internalFormat = GL_RGB5;
			}
			else if ( r_texturebits->integer == 32 )
			{
				internalFormat = GL_RGB8;
			}
			else
			{
				internalFormat = 3;
			}
		}
		else if ( samples == 4 )
		{
			if ( r_texturebits->integer == 16 )
			{
				internalFormat = GL_RGBA4;
			}
			else if ( r_texturebits->integer == 32 )
			{
				internalFormat = GL_RGBA8;
			}
			else
			{
				internalFormat = 4;
			}
		}
	} else {
		internalFormat = 3;
	}
	// copy or resample data as appropriate for first MIP level
	if ( ( scaled_width == width ) && 
		( scaled_height == height ) ) {
		if (!mipmap)
		{
			qglTexImage2D (GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			*pUploadWidth = scaled_width;
			*pUploadHeight = scaled_height;
			*format = internalFormat;

			goto done;
		}
		Com_Memcpy (scaledBuffer, data, width*height*4);
	}
	else
	{
		// use the normal mip-mapping function to go down from here
		while ( width > scaled_width || height > scaled_height ) {
			R_MipMap( (byte *)data, width, height );
			width >>= 1;
			height >>= 1;
			if ( width < 1 ) {
				width = 1;
			}
			if ( height < 1 ) {
				height = 1;
			}
		}
		Com_Memcpy( scaledBuffer, data, width * height * 4 );
	}

	R_LightScaleTexture (scaledBuffer, scaled_width, scaled_height, !mipmap );

	*pUploadWidth = scaled_width;
	*pUploadHeight = scaled_height;
	*format = internalFormat;

	qglTexImage2D (GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			R_MipMap( (byte *)scaledBuffer, scaled_width, scaled_height );
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;

			if ( r_colorMipLevels->integer ) {
				R_BlendOverTexture( (byte *)scaledBuffer, scaled_width * scaled_height, mipBlendColors[miplevel] );
			}

			qglTexImage2D (GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
		}
	}
done:

	if (mipmap)
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	GL_CheckErrors();

	if ( scaledBuffer != 0 )
		Hunk_FreeTempMemory( scaledBuffer );
	if ( resampledBuffer != 0 )
		Hunk_FreeTempMemory( resampledBuffer );
}


void GL_CreateImage( const image_t* image, const byte *pic, qboolean isLightmap )
{
    int glWrapClampMode;

    // Get the associated GL binding for this image
    glimage_t* glimage = glimages + image->index;

    Com_Memset( &glimages[image->index], 0, sizeof( glimage_t ) );

	glimage->texnum = 1024 + image->index;

    assert(image->wrapClampMode != GL_CLAMP && image->wrapClampMode != GL_REPEAT);

    switch (image->wrapClampMode)
    {
    case WRAPMODE_CLAMP:
        glWrapClampMode = GL_CLAMP;
        break;
    case WRAPMODE_REPEAT:
        glWrapClampMode = GL_REPEAT;
        break;
    }

	// lightmaps are always allocated on TMU 1
	if ( qglActiveTextureARB && isLightmap ) {
		glimage->TMU = 1;
	} else {
		glimage->TMU = 0;
	}

	if ( qglActiveTextureARB ) {
		GL_SelectTexture( glimage->TMU );
	}

	GL_Bind(image);

	Upload32( (unsigned *)pic, image->width, image->height, 
								image->mipmap,
								image->allowPicmip,
								isLightmap,
								&glimage->internalFormat,
								&glimage->uploadWidth,
								&glimage->uploadHeight );

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapClampMode );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapClampMode );

	qglBindTexture( GL_TEXTURE_2D, 0 );

	if ( glimage->TMU == 1 ) {
		GL_SelectTexture( 0 );
	}


}

void GL_SetImageBorderColor( const image_t* image, const float* borderColor )
{
    if (qglTexParameterfv)
    {
        GL_Bind( image );
	    qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
    }
}

void GL_DeleteImage( const image_t* image )
{
	qglDeleteTextures( 1, &glimages[image->index].texnum );
    Com_Memset( &glimages[image->index], 0, sizeof( glimage_t ) );
}

void GL_UpdateCinematic( const image_t* image, const byte* pic, int cols, int rows, qboolean dirty )
{
    assert( image->mipmap == qfalse );

	GL_Bind( image );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != image->width || rows != image->height ) {
		qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, pic );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );	
	} else {
		if (dirty) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, pic );
		}
	}
}

imageFormat_t GL_GetImageFormat( const image_t* image )
{
    switch ( glimages[image->index].internalFormat )
    {
    case 1:
        return IMAGEFORMAT_I;
    case 2:
        return IMAGEFORMAT_IA;
    case 3: 
        return IMAGEFORMAT_RGB;
    case 4:
        return IMAGEFORMAT_RGBA;
    case GL_RGBA8:
        return IMAGEFORMAT_RGBA8;
    case GL_RGB8:
        return IMAGEFORMAT_RGB8;
    case GL_RGB_S3TC:
        return IMAGEFORMAT_S3TC;
    case GL_RGBA4:
        return IMAGEFORMAT_RGBA4;
    case GL_RGB5:
        return IMAGEFORMAT_RGB5;
    default:
        return IMAGEFORMAT_UNKNOWN;
    }
}

int GL_ConvertImageFormat( imageFormat_t i )
{
    switch ( i )
    {
    case IMAGEFORMAT_I:
        return GL_LUMINANCE;
    case IMAGEFORMAT_IA:
        return GL_LUMINANCE_ALPHA;
    case IMAGEFORMAT_RGB:
        return GL_RGB;
    case IMAGEFORMAT_RGBA:
        return GL_RGBA;
    case IMAGEFORMAT_RGBA8:
        return GL_RGBA8;
    case IMAGEFORMAT_RGB8:
        return GL_RGB8;
    case IMAGEFORMAT_S3TC:
        return GL_RGB_S3TC;
    case IMAGEFORMAT_RGBA4:
        return GL_RGBA4;
    case IMAGEFORMAT_RGB5:
        return GL_RGB5;
    default:
        return 0;
    }
}

/*
** GL_Bind
*/
void GL_Bind( const image_t *image ) {

    glimage_t* glimage;

    if ( !image ) {
		RI_Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		glimage = &glimages[tr.defaultImage->index];
	} else {
		glimage = &glimages[image->index];
	}

	if ( r_nobind->integer && tr.dlightImage ) {		// performance evaluation option
		glimage = &glimages[tr.dlightImage->index];
	}

    assert(glimage->texnum != 0);
	
    if ( glState.currenttextures[glState.currenttmu] != glimage->texnum ) {
		glimage->frameUsed = tr.frameCount;
		glState.currenttextures[glState.currenttmu] = glimage->texnum;
		qglBindTexture (GL_TEXTURE_2D, glimage->texnum);
	}
}

/*
** GL_BindMultitexture
*/
void GL_BindMultitexture( const image_t *image0, GLuint env0, image_t *image1, GLuint env1 ) {
	glimage_t		*tex0, *tex1;

	tex0 = &glimages[image0->index];
	tex1 = &glimages[image1->index];

	if ( r_nobind->integer && tr.dlightImage ) {		// performance evaluation option
		tex0 = tex1 = &glimages[tr.dlightImage->index];
	}

	if ( glState.currenttextures[1] != tex1->texnum ) {
		GL_SelectTexture( 1 );
		tex1->frameUsed = tr.frameCount;
		glState.currenttextures[1] = tex1->texnum;
		qglBindTexture( GL_TEXTURE_2D, tex1->texnum );
	}
	if ( glState.currenttextures[0] != tex0->texnum ) {
		GL_SelectTexture( 0 );
		tex0->frameUsed = tr.frameCount;
		glState.currenttextures[0] = tex0->texnum;
		qglBindTexture( GL_TEXTURE_2D, tex0->texnum );
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
int GL_SumOfUsedImages( void ) 
{
	int	total;
	int i;

	total = 0;
	for ( i = 0; i < tr.numImages; i++ ) {
        const glimage_t* glimage = &glimages[tr.images[i]->index];
		if ( glimage->frameUsed == tr.frameCount ) {
			total += glimage->uploadWidth * glimage->uploadHeight;
		}
	}

	return total;
}

