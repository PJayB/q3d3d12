#include "tr_local.h"
#include "qgl.h"
#include "gl_common.h"
#include "tr_state.h"

void GLRB_ShadowFinish( void )
{
	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_NOTEQUAL, 0, 255 );

	qglDisable (GL_CLIP_PLANE0);
	qglDisable (GL_CULL_FACE);

	GL_Bind( tr.whiteImage );

    qglLoadIdentity ();

	qglColor3f( 0.6f, 0.6f, 0.6f );
	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

//	qglColor3f( 1, 0, 0 );
//	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	qglBegin( GL_QUADS );
	qglVertex3f( -100, 100, -10 );
	qglVertex3f( 100, 100, -10 );
	qglVertex3f( 100, -100, -10 );
	qglVertex3f( -100, -100, -10 );
	qglEnd ();

	qglColor4f(1,1,1,1);
	qglDisable( GL_STENCIL_TEST );
}

void RenderShadowEdges( const float* edges, int edgeCount )
{
    int i;
    for ( i = 0; i < edgeCount; ++i, edges += 16 )
    {
        qglBegin( GL_TRIANGLE_STRIP );
		qglVertex3fv( &edges[ 0] );
		qglVertex3fv( &edges[ 4] );
		qglVertex3fv( &edges[ 8] );
		qglVertex3fv( &edges[12] );
		qglEnd();
    }
}

void GLRB_ShadowSilhouette( const float* edges, int edgeCount )
{
	GL_Bind( tr.whiteImage );
	qglEnable( GL_CULL_FACE );
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	qglColor3f( 0.2f, 0.2f, 0.2f );

	// don't write to the color buffer
	qglColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	// mirrors have the culling order reversed
	GL_Cull( CT_BACK_SIDED ); // @pjb: resolves to GL_FRONT if to mirror flag is set
	qglStencilOp( GL_KEEP, GL_KEEP, GL_INCR );

	RenderShadowEdges( edges, edgeCount );

	GL_Cull( CT_FRONT_SIDED ); // @pjb: resolves to GL_BACK due to mirror flag is set
	qglStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

	RenderShadowEdges( edges, edgeCount );

	// reenable writing to the color buffer
	qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
}
