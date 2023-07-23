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
//
// cg_syscalls.c -- this file is only included when building a dll
// cg_syscalls.asm is included instead when building a qvm
#ifdef Q3_VM
#error "Do not use in VM build"
#endif

#define ENGINE_CGAME

#include "cg_local.h"

static int (QDECL *syscallA)( vmArg_t* args, ... ) = (int (QDECL *)( vmArg_t*, ...)) (size_t) ~0ULL;

void dllEntry( int (QDECL  *syscallptr)( vmArg_t* arg,... ) ) {
	syscallA = syscallptr;
}

#include "engine.cgame.shared.h"
#include "engine.cgame.syscalls.h"
