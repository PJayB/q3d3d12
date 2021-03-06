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
/*****************************************************************************
 * name:		botlib.h
 *
 * desc:		bot AI library
 *
 * $Archive: /source/code/game/botai.h $
 *
 *****************************************************************************/

#define	BOTLIB_API_VERSION		2

struct aas_clientmove_s;
struct aas_entityinfo_s;
struct aas_areainfo_s;
struct aas_altroutegoal_s;
struct aas_predictroute_s;
struct bot_consolemessage_s;
struct bot_match_s;
struct bot_goal_s;
struct bot_moveresult_s;
struct bot_initmove_s;
struct weaponinfo_s;

#define BOTFILESBASEFOLDER		"botfiles"
//debug line colors
#define LINECOLOR_NONE			-1
#define LINECOLOR_RED			1//0xf2f2f0f0L
#define LINECOLOR_GREEN			2//0xd0d1d2d3L
#define LINECOLOR_BLUE			3//0xf3f3f1f1L
#define LINECOLOR_YELLOW		4//0xdcdddedfL
#define LINECOLOR_ORANGE		5//0xe0e1e2e3L

//Print types
#define PRT_MESSAGE				1
#define PRT_WARNING				2
#define PRT_ERROR				3
#define PRT_FATAL				4
#define PRT_EXIT				5

//console message types
#define CMS_NORMAL				0
#define CMS_CHAT				1

//botlib error codes
#define BLERR_NOERROR					0	//no error
#define BLERR_LIBRARYNOTSETUP			1	//library not setup
#define BLERR_INVALIDENTITYNUMBER		2	//invalid entity number
#define BLERR_NOAASFILE					3	//no AAS file available
#define BLERR_CANNOTOPENAASFILE			4	//cannot open AAS file
#define BLERR_WRONGAASFILEID			5	//incorrect AAS file id
#define BLERR_WRONGAASFILEVERSION		6	//incorrect AAS file version
#define BLERR_CANNOTREADAASLUMP			7	//cannot read AAS file lump
#define BLERR_CANNOTLOADICHAT			8	//cannot load initial chats
#define BLERR_CANNOTLOADITEMWEIGHTS		9	//cannot load item weights
#define BLERR_CANNOTLOADITEMCONFIG		10	//cannot load item config
#define BLERR_CANNOTLOADWEAPONWEIGHTS	11	//cannot load weapon weights
#define BLERR_CANNOTLOADWEAPONCONFIG	12	//cannot load weapon config

//action flags
#define ACTION_ATTACK			0x0000001
#define ACTION_USE				0x0000002
#define ACTION_RESPAWN			0x0000008
#define ACTION_JUMP				0x0000010
#define ACTION_MOVEUP			0x0000020
#define ACTION_CROUCH			0x0000080
#define ACTION_MOVEDOWN			0x0000100
#define ACTION_MOVEFORWARD		0x0000200
#define ACTION_MOVEBACK			0x0000800
#define ACTION_MOVELEFT			0x0001000
#define ACTION_MOVERIGHT		0x0002000
#define ACTION_DELAYEDJUMP		0x0008000
#define ACTION_TALK				0x0010000
#define ACTION_GESTURE			0x0020000
#define ACTION_WALK				0x0080000
#define ACTION_AFFIRMATIVE		0x0100000
#define ACTION_NEGATIVE			0x0200000
#define ACTION_GETFLAG			0x0800000
#define ACTION_GUARDBASE		0x1000000
#define ACTION_PATROL			0x2000000
#define ACTION_FOLLOWME			0x8000000

//the bot input, will be converted to an usercmd_t
typedef struct bot_input_s
{
	float thinktime;		//time since last output (in seconds)
	vec3_t dir;				//movement direction
	float speed;			//speed in the range [0, 400]
	vec3_t viewangles;		//the view angles
	int actionflags;		//one of the ACTION_? flags
	int weapon;				//weapon to use
} bot_input_t;

#ifndef BSPTRACE

#define BSPTRACE

//bsp_trace_t hit surface
typedef struct bsp_surface_s
{
	char name[16];
	int flags;
	int value;
} bsp_surface_t;

//remove the bsp_trace_s structure definition l8r on
//a trace is returned when a box is swept through the world
typedef struct bsp_trace_s
{
	qboolean		allsolid;	// if true, plane is not valid
	qboolean		startsolid;	// if true, the initial point was in a solid area
	float			fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t			endpos;		// final position
	cplane_t		plane;		// surface normal at impact
	float			exp_dist;	// expanded plane distance
	int				sidenum;	// number of the brush side hit
	bsp_surface_t	surface;	// the hit point surface
	int				contents;	// contents on other side of surface hit
	int				ent;		// number of entity hit
} bsp_trace_t;

#endif	// BSPTRACE

//entity state
typedef struct bot_entitystate_s
{
	int		type;			// entity type
	int		flags;			// entity flags
	vec3_t	origin;			// origin of the entity
	vec3_t	angles;			// angles of the model
	vec3_t	old_origin;		// for lerping
	vec3_t	mins;			// bounding box minimums
	vec3_t	maxs;			// bounding box maximums
	int		groundent;		// ground entity
	int		solid;			// solid type
	int		modelindex;		// model used
	int		modelindex2;	// weapons, CTF flags, etc
	int		frame;			// model frame number
	int		event;			// impulse events -- muzzle flashes, footsteps, etc
	int		eventParm;		// even parameter
	int		powerups;		// bit flags
	int		weapon;			// determines weapon and flash model, etc
	int		legsAnim;		// mask off ANIM_TOGGLEBIT
	int		torsoAnim;		// mask off ANIM_TOGGLEBIT
} bot_entitystate_t;

// @pjb: hack: breaks abstraction
int Export_BotLibSetup(void);
int Export_BotLibShutdown(void);
int Export_BotLibVarSet(char *var_name, char *value);
int Export_BotLibVarGet(char *var_name, char *value, int size);
int Export_BotLibStartFrame(float time);
int Export_BotLibLoadMap(const char *mapname);
int Export_BotLibUpdateEntity(int ent, bot_entitystate_t *state);
int BotExportTest(int parm0, char *parm1, vec3_t parm2, vec3_t parm3);
int GeneticParentsAndChildSelection(int numranks, float *ranks, int *parent1, int *parent2, int *child);
void BotResetWeaponState(int weaponstate);

void QDECL BotImport_Print(int type, char *fmt, ...);
void BotImport_Trace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask);
void BotImport_EntityTrace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask);
int BotImport_PointContents(vec3_t point);
int BotImport_inPVS(vec3_t p1, vec3_t p2);
char *BotImport_BSPEntityData(void);
void BotImport_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t outmins, vec3_t outmaxs, vec3_t origin);
void *BotImport_GetMemory(size_t size);
void BotImport_FreeMemory(void *ptr);
void *BotImport_HunkAlloc( size_t size );
int BotImport_DebugPolygonCreate(int color, int numPoints, vec3_t *points);
void BotImport_DebugPolygonShow(int id, int color, int numPoints, vec3_t *points);
void BotImport_DebugPolygonDelete(int id);
int BotImport_DebugLineCreate(void);
void BotImport_DebugLineDelete(int line);
void BotImport_DebugLineShow(int line, vec3_t start, vec3_t end, int color);
int BotImport_AvailableMemory( void );
void BotImport_BotClientCommand( int client, char *command );

int FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode );
int FS_Read2( void *buffer, int len, fileHandle_t f );
int FS_Write( const void *buffer, int len, fileHandle_t f );
void FS_FCloseFile( fileHandle_t f );
int FS_Seek( fileHandle_t f, long offset, int origin );

/* Library variables:

name:						default:			module(s):			description:

"basedir"					""					l_utils.c			base directory
"gamedir"					""					l_utils.c			game directory
"cddir"						""					l_utils.c			CD directory

"log"						"0"					l_log.c				enable/disable creating a log file
"maxclients"				"4"					be_interface.c		maximum number of clients
"maxentities"				"1024"				be_interface.c		maximum number of entities
"bot_developer"				"0"					be_interface.c		bot developer mode

"phys_friction"				"6"					be_aas_move.c		ground friction
"phys_stopspeed"			"100"				be_aas_move.c		stop speed
"phys_gravity"				"800"				be_aas_move.c		gravity value
"phys_waterfriction"		"1"					be_aas_move.c		water friction
"phys_watergravity"			"400"				be_aas_move.c		gravity in water
"phys_maxvelocity"			"320"				be_aas_move.c		maximum velocity
"phys_maxwalkvelocity"		"320"				be_aas_move.c		maximum walk velocity
"phys_maxcrouchvelocity"	"100"				be_aas_move.c		maximum crouch velocity
"phys_maxswimvelocity"		"150"				be_aas_move.c		maximum swim velocity
"phys_walkaccelerate"		"10"				be_aas_move.c		walk acceleration
"phys_airaccelerate"		"1"					be_aas_move.c		air acceleration
"phys_swimaccelerate"		"4"					be_aas_move.c		swim acceleration
"phys_maxstep"				"18"				be_aas_move.c		maximum step height
"phys_maxsteepness"			"0.7"				be_aas_move.c		maximum floor steepness
"phys_maxbarrier"			"32"				be_aas_move.c		maximum barrier height
"phys_maxwaterjump"			"19"				be_aas_move.c		maximum waterjump height
"phys_jumpvel"				"270"				be_aas_move.c		jump z velocity
"phys_falldelta5"			"40"				be_aas_move.c
"phys_falldelta10"			"60"				be_aas_move.c
"rs_waterjump"				"400"				be_aas_move.c
"rs_teleport"				"50"				be_aas_move.c
"rs_barrierjump"			"100"				be_aas_move.c
"rs_startcrouch"			"300"				be_aas_move.c
"rs_startgrapple"			"500"				be_aas_move.c
"rs_startwalkoffledge"		"70"				be_aas_move.c
"rs_startjump"				"300"				be_aas_move.c
"rs_rocketjump"				"500"				be_aas_move.c
"rs_bfgjump"				"500"				be_aas_move.c
"rs_jumppad"				"250"				be_aas_move.c
"rs_aircontrolledjumppad"	"300"				be_aas_move.c
"rs_funcbob"				"300"				be_aas_move.c
"rs_startelevator"			"50"				be_aas_move.c
"rs_falldamage5"			"300"				be_aas_move.c
"rs_falldamage10"			"500"				be_aas_move.c
"rs_maxjumpfallheight"		"450"				be_aas_move.c

"max_aaslinks"				"4096"				be_aas_sample.c		maximum links in the AAS
"max_routingcache"			"4096"				be_aas_route.c		maximum routing cache size in KB
"forceclustering"			"0"					be_aas_main.c		force recalculation of clusters
"forcereachability"			"0"					be_aas_main.c		force recalculation of reachabilities
"forcewrite"				"0"					be_aas_main.c		force writing of aas file
"aasoptimize"				"0"					be_aas_main.c		enable aas optimization
"sv_mapChecksum"			"0"					be_aas_main.c		BSP file checksum
"bot_visualizejumppads"		"0"					be_aas_reach.c		visualize jump pads

"bot_reloadcharacters"		"0"					-					reload bot character files
"ai_gametype"				"0"					be_ai_goal.c		game type
"droppedweight"				"1000"				be_ai_goal.c		additional dropped item weight
"weapindex_rocketlauncher"	"5"					be_ai_move.c		rl weapon index for rocket jumping
"weapindex_bfg10k"			"9"					be_ai_move.c		bfg weapon index for bfg jumping
"weapindex_grapple"			"10"				be_ai_move.c		grapple weapon index for grappling
"entitytypemissile"			"3"					be_ai_move.c		ET_MISSILE
"offhandgrapple"			"0"					be_ai_move.c		enable off hand grapple hook
"cmd_grappleon"				"grappleon"			be_ai_move.c		command to activate off hand grapple
"cmd_grappleoff"			"grappleoff"		be_ai_move.c		command to deactivate off hand grapple
"itemconfig"				"items.c"			be_ai_goal.c		item configuration file
"weaponconfig"				"weapons.c"			be_ai_weap.c		weapon configuration file
"synfile"					"syn.c"				be_ai_chat.c		file with synonyms
"rndfile"					"rnd.c"				be_ai_chat.c		file with random strings
"matchfile"					"match.c"			be_ai_chat.c		file with match strings
"nochat"					"0"					be_ai_chat.c		disable chats
"max_messages"				"1024"				be_ai_chat.c		console message heap size
"max_weaponinfo"			"32"				be_ai_weap.c		maximum number of weapon info
"max_projectileinfo"		"32"				be_ai_weap.c		maximum number of projectile info
"max_iteminfo"				"256"				be_ai_goal.c		maximum number of item info
"max_levelitems"			"256"				be_ai_goal.c		maximum number of level items

*/

// @pjb: bypassing export mechanism
int PC_AddGlobalDefine(char *string);
int PC_RemoveGlobalDefine(char *name);
int PC_LoadSourceHandle(const char *filename);
int PC_FreeSourceHandle(int handle);
int PC_ReadTokenHandle(int handle, pc_token_t *pc_token);
int PC_SourceFileAndLine(int handle, char *filename, int *line);
