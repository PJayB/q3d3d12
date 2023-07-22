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

/*****************************************************************************
 * name:		be_interface.c // bk010221 - FIXME - DEAD code elimination
 *
 * desc:		bot library interface
 *
 * $Archive: /MissionPack/code/botlib/be_interface.c $
 *
 *****************************************************************************/

#include "../qshared/q_shared.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"
#include "be_interface.h"

#include "../game/be_ea.h"
#include "be_ai_weight.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_weap.h"
#include "../game/be_ai_chat.h"
#include "../game/be_ai_char.h"
#include "../game/be_ai_gen.h"

//library globals in a structure
botlib_globals_t botlibglobals;

//
int bot_developer;
//qtrue if the library is setup
int botlibsetup = qfalse;

//===========================================================================
//
// several functions used by the exported functions
//
//===========================================================================

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Sys_MilliSeconds(void)
{
	return clock() * 1000 / CLOCKS_PER_SEC;
} //end of the function Sys_MilliSeconds
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean ValidClientNumber(int num, char *str)
{
	if (num < 0 || num > botlibglobals.maxclients)
	{
		//weird: the disabled stuff results in a crash
		BotImport_Print(PRT_ERROR, "%s: invalid client number %d, [0, %d]\n",
										str, num, botlibglobals.maxclients);
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotValidateClientNumber
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean ValidEntityNumber(int num, char *str)
{
	if (num < 0 || num > botlibglobals.maxentities)
	{
		BotImport_Print(PRT_ERROR, "%s: invalid entity number %d, [0, %d]\n",
										str, num, botlibglobals.maxentities);
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotValidateClientNumber
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean BotLibSetup(char *str)
{
	if (!botlibglobals.botlibsetup)
	{
		BotImport_Print(PRT_ERROR, "%s: bot library used before being setup\n", str);
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotLibSetup

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibSetup(void)
{
	int		errnum;
	
	bot_developer = LibVarGetValue("bot_developer");
  memset( &botlibglobals, 0, sizeof(botlibglobals) ); // bk001207 - init
	//initialize byte swapping (litte endian etc.)
//	Swap_Init();
	Log_Open("botlib.log");
	//
	BotImport_Print(PRT_MESSAGE, "------- BotLib Initialization -------\n");
	//
	botlibglobals.maxclients = (int) LibVarValue("maxclients", "128");
	botlibglobals.maxentities = (int) LibVarValue("maxentities", "1024");

	errnum = AAS_Setup();			//be_aas_main.c
	if (errnum != BLERR_NOERROR) return errnum;
	errnum = EA_Setup();			//be_ea.c
	if (errnum != BLERR_NOERROR) return errnum;
	errnum = BotSetupWeaponAI();	//be_ai_weap.c
	if (errnum != BLERR_NOERROR)return errnum;
	errnum = BotSetupGoalAI();		//be_ai_goal.c
	if (errnum != BLERR_NOERROR) return errnum;
	errnum = BotSetupChatAI();		//be_ai_chat.c
	if (errnum != BLERR_NOERROR) return errnum;
	errnum = BotSetupMoveAI();		//be_ai_move.c
	if (errnum != BLERR_NOERROR) return errnum;

	botlibsetup = qtrue;
	botlibglobals.botlibsetup = qtrue;

	return BLERR_NOERROR;
} //end of the function Export_BotLibSetup
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibShutdown(void)
{
	if (!BotLibSetup("BotLibShutdown")) return BLERR_LIBRARYNOTSETUP;
#ifndef DEMO
	//DumpFileCRCs();
#endif //DEMO
	//
	BotShutdownChatAI();		//be_ai_chat.c
	BotShutdownMoveAI();		//be_ai_move.c
	BotShutdownGoalAI();		//be_ai_goal.c
	BotShutdownWeaponAI();		//be_ai_weap.c
	BotShutdownWeights();		//be_ai_weight.c
	BotShutdownCharacters();	//be_ai_char.c
	//shud down aas
	AAS_Shutdown();
	//shut down bot elemantary actions
	EA_Shutdown();
	//free all libvars
	LibVarDeAllocAll();
	//remove all global defines from the pre compiler
	PC_RemoveAllGlobalDefines();

	//dump all allocated memory
//	DumpMemory();
#ifdef DEBUG
	PrintMemoryLabels();
#endif
	//shut down library log file
	Log_Shutdown();
	//
	botlibsetup = qfalse;
	botlibglobals.botlibsetup = qfalse;
	// print any files still open
	PC_CheckOpenSourceHandles();
	//
	return BLERR_NOERROR;
} //end of the function Export_BotLibShutdown
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibVarSet(char *var_name, char *value)
{
	LibVarSet(var_name, value);
	return BLERR_NOERROR;
} //end of the function Export_BotLibVarSet
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibVarGet(char *var_name, char *value, int size)
{
	char *varvalue;

	varvalue = LibVarGetString(var_name);
	strncpy(value, varvalue, size-1);
	value[size-1] = '\0';
	return BLERR_NOERROR;
} //end of the function Export_BotLibVarGet
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibStartFrame(float time)
{
	if (!BotLibSetup("BotStartFrame")) return BLERR_LIBRARYNOTSETUP;
	return AAS_StartFrame(time);
} //end of the function Export_BotLibStartFrame
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibLoadMap(const char *mapname)
{
#ifdef DEBUG
	int starttime = Sys_MilliSeconds();
#endif
	int errnum;

	if (!BotLibSetup("BotLoadMap")) return BLERR_LIBRARYNOTSETUP;
	//
	BotImport_Print(PRT_MESSAGE, "------------ Map Loading ------------\n");
	//startup AAS for the current map, model and sound index
	errnum = AAS_LoadMap(mapname);
	if (errnum != BLERR_NOERROR) return errnum;
	//initialize the items in the level
	BotInitLevelItems();		//be_ai_goal.h
	BotSetBrushModelTypes();	//be_ai_move.h
	//
	BotImport_Print(PRT_MESSAGE, "-------------------------------------\n");
#ifdef DEBUG
	BotImport_Print(PRT_MESSAGE, "map loaded in %d msec\n", Sys_MilliSeconds() - starttime);
#endif
	//
	return BLERR_NOERROR;
} //end of the function Export_BotLibLoadMap
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibUpdateEntity(int ent, bot_entitystate_t *state)
{
	if (!BotLibSetup("BotUpdateEntity")) return BLERR_LIBRARYNOTSETUP;
	if (!ValidEntityNumber(ent, "BotUpdateEntity")) return BLERR_INVALIDENTITYNUMBER;

	return AAS_UpdateEntity(ent, state);
} //end of the function Export_BotLibUpdateEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_TestMovementPrediction(int entnum, vec3_t origin, vec3_t dir);
void ElevatorBottomCenter(aas_reachability_t *reach, vec3_t bottomcenter);
int BotGetReachabilityToGoal(vec3_t origin, int areanum,
									  int lastgoalareanum, int lastareanum,
									  int *avoidreach, float *avoidreachtimes, int *avoidreachtries,
									  bot_goal_t *goal, int travelflags, int movetravelflags,
									  struct bot_avoidspot_s *avoidspots, int numavoidspots, int *flags);

int AAS_PointLight(vec3_t origin, int *red, int *green, int *blue);

int AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);

int AAS_Reachability_WeaponJump(int area1num, int area2num);

int BotFuzzyPointReachabilityArea(vec3_t origin);

float BotGapDistance(vec3_t origin, vec3_t hordir, int entnum);

void AAS_FloodAreas(vec3_t origin);

int BotExportTest(int parm0, char *parm1, vec3_t parm2, vec3_t parm3)
{

//	return AAS_PointLight(parm2, NULL, NULL, NULL);

#ifdef DEBUG
	static int area = -1;
	static int line[2];
	int newarea, i, highlightarea, flood;
//	int reachnum;
	vec3_t eye, forward, right, end, origin;
//	vec3_t bottomcenter;
//	aas_trace_t trace;
//	aas_face_t *face;
//	aas_entity_t *ent;
//	bsp_trace_t bsptrace;
//	aas_reachability_t reach;
//	bot_goal_t goal;

	// clock_t start_time, end_time;
//	vec3_t mins = {-16, -16, -24};
//	vec3_t maxs = {16, 16, 32};

//	int areas[10], numareas;


	//return 0;

	if (!aasworld.loaded) return 0;

	/*
	if (parm0 & 1)
	{
		AAS_ClearShownPolygons();
		AAS_FloodAreas(parm2);
	} //end if
	return 0;
	*/
	for (i = 0; i < 2; i++) if (!line[i]) line[i] = BotImport_DebugLineCreate();

//	AAS_ClearShownDebugLines();

	//if (AAS_AgainstLadder(parm2)) BotImport_Print(PRT_MESSAGE, "against ladder\n");
	//BotOnGround(parm2, PRESENCE_NORMAL, 1, &newarea, &newarea);
	//BotImport_Print(PRT_MESSAGE, "%f %f %f\n", parm2[0], parm2[1], parm2[2]);
	//*
	highlightarea = LibVarGetValue("bot_highlightarea");
	if (highlightarea > 0)
	{
		newarea = highlightarea;
	} //end if
	else
	{
		VectorCopy(parm2, origin);
		origin[2] += 0.5;
		//newarea = AAS_PointAreaNum(origin);
		newarea = BotFuzzyPointReachabilityArea(origin);
	} //end else

	BotImport_Print(PRT_MESSAGE, "\rtravel time to goal (%d) = %d  ", botlibglobals.goalareanum,
		AAS_AreaTravelTimeToGoalArea(newarea, origin, botlibglobals.goalareanum, TFL_DEFAULT));
	//newarea = BotReachabilityArea(origin, qtrue);
	if (newarea != area)
	{
		BotImport_Print(PRT_MESSAGE, "origin = %f, %f, %f\n", origin[0], origin[1], origin[2]);
		area = newarea;
		BotImport_Print(PRT_MESSAGE, "new area %d, cluster %d, presence type %d\n",
					area, AAS_AreaCluster(area), AAS_PointPresenceType(origin));
		BotImport_Print(PRT_MESSAGE, "area contents: ");
		if (aasworld.areasettings[area].contents & AREACONTENTS_WATER)
		{
			BotImport_Print(PRT_MESSAGE, "water &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_LAVA)
		{
			BotImport_Print(PRT_MESSAGE, "lava &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_SLIME)
		{
			BotImport_Print(PRT_MESSAGE, "slime &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_JUMPPAD)
		{
			BotImport_Print(PRT_MESSAGE, "jump pad &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_CLUSTERPORTAL)
		{
			BotImport_Print(PRT_MESSAGE, "cluster portal &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_VIEWPORTAL)
		{
			BotImport_Print(PRT_MESSAGE, "view portal &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_DONOTENTER)
		{
			BotImport_Print(PRT_MESSAGE, "do not enter &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_MOVER)
		{
			BotImport_Print(PRT_MESSAGE, "mover &");
		} //end if
		if (!aasworld.areasettings[area].contents)
		{
			BotImport_Print(PRT_MESSAGE, "empty");
		} //end if
		BotImport_Print(PRT_MESSAGE, "\n");
		BotImport_Print(PRT_MESSAGE, "travel time to goal (%d) = %d\n", botlibglobals.goalareanum,
					AAS_AreaTravelTimeToGoalArea(newarea, origin, botlibglobals.goalareanum, TFL_DEFAULT|TFL_ROCKETJUMP));
		/*
		VectorCopy(origin, end);
		end[2] += 5;
		numareas = AAS_TraceAreas(origin, end, areas, NULL, 10);
		AAS_TraceClientBBox(origin, end, PRESENCE_CROUCH, -1);
		BotImport_Print(PRT_MESSAGE, "num areas = %d, area = %d\n", numareas, areas[0]);
		*/
		/*
		botlibglobals.goalareanum = newarea;
		VectorCopy(parm2, botlibglobals.goalorigin);
		BotImport_Print(PRT_MESSAGE, "new goal %2.1f %2.1f %2.1f area %d\n",
								origin[0], origin[1], origin[2], newarea);
		*/
	} //end if
	//*
	flood = LibVarGetValue("bot_flood");
	if (parm0 & 1)
	{
		if (flood)
		{
			AAS_ClearShownPolygons();
			AAS_ClearShownDebugLines();
			AAS_FloodAreas(parm2);
		}
		else
		{
			botlibglobals.goalareanum = newarea;
			VectorCopy(parm2, botlibglobals.goalorigin);
			BotImport_Print(PRT_MESSAGE, "new goal %2.1f %2.1f %2.1f area %d\n",
									origin[0], origin[1], origin[2], newarea);
		}
	} //end if*/
	if (flood)
		return 0;
//	if (parm0 & BUTTON_USE)
//	{
//		botlibglobals.runai = !botlibglobals.runai;
//		if (botlibglobals.runai) BotImport_Print(PRT_MESSAGE, "started AI\n");
//		else BotImport_Print(PRT_MESSAGE, "stopped AI\n");
		//* /
		/*
		goal.areanum = botlibglobals.goalareanum;
		reachnum = BotGetReachabilityToGoal(parm2, newarea, 1,
										ms.avoidreach, ms.avoidreachtimes,
										&goal, TFL_DEFAULT);
		if (!reachnum)
		{
			BotImport_Print(PRT_MESSAGE, "goal not reachable\n");
		} //end if
		else
		{
			AAS_ReachabilityFromNum(reachnum, &reach);
			AAS_ClearShownDebugLines();
			AAS_ShowArea(area, qtrue);
			AAS_ShowArea(reach.areanum, qtrue);
			AAS_DrawCross(reach.start, 6, LINECOLOR_BLUE);
			AAS_DrawCross(reach.end, 6, LINECOLOR_RED);
			//
			if ((reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_ELEVATOR)
			{
				ElevatorBottomCenter(&reach, bottomcenter);
				AAS_DrawCross(bottomcenter, 10, LINECOLOR_GREEN);
			} //end if
		} //end else*/
//		BotImport_Print(PRT_MESSAGE, "travel time to goal = %d\n",
//					AAS_AreaTravelTimeToGoalArea(area, origin, botlibglobals.goalareanum, TFL_DEFAULT));
//		BotImport_Print(PRT_MESSAGE, "test rj from 703 to 716\n");
//		AAS_Reachability_WeaponJump(703, 716);
//	} //end if*/

/*	face = AAS_AreaGroundFace(newarea, parm2);
	if (face)
	{
		AAS_ShowFace(face - aasworld.faces);
	} //end if*/
	/*
	AAS_ClearShownDebugLines();
	AAS_ShowArea(newarea, parm0 & BUTTON_USE);
	AAS_ShowReachableAreas(area);
	*/
	AAS_ClearShownPolygons();
	AAS_ClearShownDebugLines();
	AAS_ShowAreaPolygons(newarea, 1, parm0 & 4);
	if (parm0 & 2) AAS_ShowReachableAreas(area);
	else
	{
		static int lastgoalareanum, lastareanum;
		static int avoidreach[MAX_AVOIDREACH];
		static float avoidreachtimes[MAX_AVOIDREACH];
		static int avoidreachtries[MAX_AVOIDREACH];
		int reachnum, resultFlags;
		bot_goal_t goal;
		aas_reachability_t reach;

		/*
		goal.areanum = botlibglobals.goalareanum;
		VectorCopy(botlibglobals.goalorigin, goal.origin);
		reachnum = BotGetReachabilityToGoal(origin, newarea,
									  lastgoalareanum, lastareanum,
									  avoidreach, avoidreachtimes, avoidreachtries,
									  &goal, TFL_DEFAULT|TFL_FUNCBOB|TFL_ROCKETJUMP, TFL_DEFAULT|TFL_FUNCBOB|TFL_ROCKETJUMP,
									  NULL, 0, &resultFlags);
		AAS_ReachabilityFromNum(reachnum, &reach);
		AAS_ShowReachability(&reach);
		*/
		int curarea;
		vec3_t curorigin;

		goal.areanum = botlibglobals.goalareanum;
		VectorCopy(botlibglobals.goalorigin, goal.origin);
		VectorCopy(origin, curorigin);
		curarea = newarea;
		for ( i = 0; i < 100; i++ ) {
			if ( curarea == goal.areanum ) {
				break;
			}
			reachnum = BotGetReachabilityToGoal(curorigin, curarea,
										  lastgoalareanum, lastareanum,
										  avoidreach, avoidreachtimes, avoidreachtries,
										  &goal, TFL_DEFAULT|TFL_FUNCBOB|TFL_ROCKETJUMP, TFL_DEFAULT|TFL_FUNCBOB|TFL_ROCKETJUMP,
										  NULL, 0, &resultFlags);
			AAS_ReachabilityFromNum(reachnum, &reach);
			AAS_ShowReachability(&reach);
			VectorCopy(reach.end, origin);
			lastareanum = curarea;
			curarea = reach.areanum;
		}
	} //end else
	VectorClear(forward);
	//BotGapDistance(origin, forward, 0);
	/*
	if (parm0 & BUTTON_USE)
	{
		BotImport_Print(PRT_MESSAGE, "test rj from 703 to 716\n");
		AAS_Reachability_WeaponJump(703, 716);
	} //end if*/

	AngleVectors(parm3, forward, right, NULL);
	//get the eye 16 units to the right of the origin
	VectorMA(parm2, 8, right, eye);
	//get the eye 24 units up
	eye[2] += 24;
	//get the end point for the line to be traced
	VectorMA(eye, 800, forward, end);

//	AAS_TestMovementPrediction(1, parm2, forward);
/*
    //trace the line to find the hit point
	trace = AAS_TraceClientBBox(eye, end, PRESENCE_NORMAL, 1);
	if (!line[0]) line[0] = BotImport_DebugLineCreate();
	BotImport_DebugLineShow(line[0], eye, trace.endpos, LINECOLOR_BLUE);
	//
	AAS_ClearShownDebugLines();
	if (trace.ent)
	{
		ent = &aasworld.entities[trace.ent];
		AAS_ShowBoundingBox(ent->origin, ent->mins, ent->maxs);
	} //end if
*/

/*
	start_time = clock();
	for (i = 0; i < 2000; i++)
	{
		AAS_Trace2(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
//		AAS_TraceClientBBox(eye, end, PRESENCE_NORMAL, 1);
	} //end for
	end_time = clock();
	BotImport_Print(PRT_MESSAGE, "me %lu clocks, %lu CLOCKS_PER_SEC\n", end_time - start_time, CLOCKS_PER_SEC);
	start_time = clock();
	for (i = 0; i < 2000; i++)
	{
		AAS_Trace(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
	} //end for
	end_time = clock();
	BotImport_Print(PRT_MESSAGE, "id %lu clocks, %lu CLOCKS_PER_SEC\n", end_time - start_time, CLOCKS_PER_SEC);
*/

    // TTimo: nested comments are BAD for gcc -Werror, use #if 0 instead..
#if 0
	AAS_ClearShownDebugLines();
	//bsptrace = AAS_Trace(eye, NULL, NULL, end, 1, MASK_PLAYERSOLID);
	bsptrace = AAS_Trace(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
	if (!line[0]) line[0] = BotImport_DebugLineCreate();
	BotImport_DebugLineShow(line[0], eye, bsptrace.endpos, LINECOLOR_YELLOW);
	if (bsptrace.fraction < 1.0)
	{
		face = AAS_TraceEndFace(&trace);
		if (face)
		{
			AAS_ShowFace(face - aasworld.faces);
		} //end if
		
		AAS_DrawPlaneCross(bsptrace.endpos,
									bsptrace.plane.normal,
									bsptrace.plane.dist + bsptrace.exp_dist,
									bsptrace.plane.type, LINECOLOR_GREEN);
		if (trace.ent)
		{
			ent = &aasworld.entities[trace.ent];
			AAS_ShowBoundingBox(ent->origin, ent->mins, ent->maxs);
		} //end if
	} //end if
	//bsptrace = AAS_Trace2(eye, NULL, NULL, end, 1, MASK_PLAYERSOLID);
	bsptrace = AAS_Trace2(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
	BotImport_DebugLineShow(line[1], eye, bsptrace.endpos, LINECOLOR_BLUE);
	if (bsptrace.fraction < 1.0)
	{
		AAS_DrawPlaneCross(bsptrace.endpos,
									bsptrace.plane.normal,
									bsptrace.plane.dist,// + bsptrace.exp_dist,
									bsptrace.plane.type, LINECOLOR_RED);
		if (bsptrace.ent)
		{
			ent = &aasworld.entities[bsptrace.ent];
			AAS_ShowBoundingBox(ent->origin, ent->mins, ent->maxs);
		} //end if
	} //end if
#endif
#endif
	return 0;
} //end of the function BotExportTest

