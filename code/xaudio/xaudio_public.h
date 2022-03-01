
// @pjb: xaudio public apis
#ifndef __AUDIO_PUBLIC_H__
#define __AUDIO_PUBLIC_H__

#ifdef __cplusplus
extern "C" {
#endif

// initializes cycling through a DMA buffer and returns information on it
qboolean XAudio_Init(void);

// gets the current DMA position
int		XAudio_GetDMAPos(void);

// shutdown the DMA xfer.
void	XAudio_Shutdown(void);

void	XAudio_BeginPainting ( int reserve );

void	XAudio_Submit( int offset, int length );

void    XAudio_Activate(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
