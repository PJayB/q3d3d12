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
// @pjb: win_snd.h: Win32-specific Quake3 sound header file

#ifndef __WIN_SHARED_H__
#define __WIN_SHARED_H__

#define C_EXPORT  extern "C" 

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

extern sysEvent_t	eventQue[MAX_QUED_EVENTS];
extern int			eventHead, eventTail;
extern byte		sys_packetReceived[MAX_MSGLEN];
extern char		sys_cmdline[MAX_STRING_CHARS];

/*
========================================================================

SHARED SYSTEM FUNCTIONS

========================================================================
*/

void Sys_In_Restart_f( void );
void Sys_Net_Restart_f( void );
void Sys_InitStreamThread( void );
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );
void Sys_QueUserEvent( int time, sysEventType_t type, int value, int value2, int userIndex, int ptrLength, void *ptr ); // @pjb: +userID
void Sys_DetectCPU( void );
void Sys_InitTimer( void );
int Sys_FrameTime( void );

// @pjb: denotes the time at the start of the frame, not the current time
#define SYS_EVENT_FRAME_TIME (-1)

qboolean	Sys_GetPacket ( netadr_t *net_from, msg_t *net_message );

void	IN_Init (void);
void	IN_Shutdown (void);
void    IN_Frame( void );

int     Sys_MapKey( int key );

#endif
