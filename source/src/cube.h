/// one big bad include file for the whole engine... nasty!

#include "tools.h"

enum                            // block types, order matters!
{
    SOLID = 0,                  // entirely solid cube [only specifies wtex]
    CORNER,                     // half full corner of a wall
    FHF,                        // floor heightfield using neighbour vdelta values
    CHF,                        // idem ceiling
    SPACE,                      // entirely empty cube
    SEMISOLID,                  // generated by mipmapping
    MAXTYPE
};
 
struct sqr
{
    uchar type;                 // one of the above
    char floor, ceil;           // height, in cubes
    uchar wtex, ftex, ctex;     // wall/floor/ceil texture ids
    uchar r, g, b;              // light value at upper left vertex
    uchar vdelta;               // vertex delta, used for heightfield cubes
    char defer;                 // used in mipmapping, when true this cube is not a perfect mip
    char occluded;              // true when occluded
    uchar utex;                 // upper wall tex id
    uchar tag;                  // used by triggers
};

enum                            // hardcoded texture numbers
{
    DEFAULT_SKY = 0,
    DEFAULT_LIQUID,
    DEFAULT_WALL,
    DEFAULT_FLOOR,
    DEFAULT_CEIL
};

enum                            // static entity types
{
    NOTUSED = 0,                // entity slot not in use in map
    LIGHT,                      // lightsource, attr1 = radius, attr2 = intensity
    PLAYERSTART,                // attr1 = angle
    I_CLIPS, I_AMMO,I_GRENADE, 
    I_HEALTH, I_ARMOUR, I_AKIMBO,
    MAPMODEL,                   // attr1 = angle, attr2 = idx
    CARROT,                     // attr1 = tag, attr2 = type
    LADDER,
    CTF_FLAG,                   // attr1 = angle, attr2 = red/blue
    MAXENTTYPES
};

struct persistent_entity        // map entity
{
    short x, y, z;              // cube aligned position
    short attr1;
    uchar type;                 // type is one of the above
    uchar attr2, attr3, attr4; 
};

struct entity : public persistent_entity    
{
    bool spawned;               // the only dynamic state of a map entity
};

#define MAPVERSION 6            // bump if map format changes, see worldio.cpp

struct header                   // map file format header
{
    char head[4];               // "CUBE"
    int version;                // any >8bit quantity is a little indian
    int headersize;             // sizeof(header)
    int sfactor;                // in bits
    int numents;
    char maptitle[128];
    uchar texlists[3][256];
    int waterlevel;
    int reserved[15];
};

#define SWS(w,x,y,s) (&(w)[(y)*(s)+(x)])
#define SW(w,x,y) SWS(w,x,y,ssize)
#define S(x,y) SW(world,x,y)            // convenient lookup of a lowest mip cube
#define SMALLEST_FACTOR 6               // determines number of mips there can be
#define DEFAULT_FACTOR 8
#define LARGEST_FACTOR 11               // 10 is already insane
#define SOLID(x) ((x)->type==SOLID)
#define MINBORD 2                       // 2 cubes from the edge of the world are always solid
#define OUTBORD(x,y) ((x)<MINBORD || (y)<MINBORD || (x)>=ssize-MINBORD || (y)>=ssize-MINBORD)

struct vec { float x, y, z; };
struct block { int x, y, xs, ys; };
struct mapmodelinfo { int rad, h, zoff, snap; char *name; };

enum { GUN_KNIFE = 0, GUN_PISTOL, GUN_SHOTGUN, GUN_SUBGUN, GUN_SNIPER, GUN_ASSAULT, GUN_GRENADE, NUMGUNS };
enum { G_PRIMARY=0, G_SECONDARY, G_MELEE, G_GRENADE, G_NUM };



// Added by Rick
class CBot;

#define MAX_STORED_LOCATIONS  7

extern int lastmillis;                  // last time (Moved up by Rick)

class CPrevLocation
{
     int nextupdate;
     
public:
     vec prevloc[MAX_STORED_LOCATIONS];
     
     CPrevLocation(void) : nextupdate(0) { Reset(); };
     void Reset(void) { loopi(MAX_STORED_LOCATIONS) prevloc[i].x=prevloc[i].y=prevloc[i].z=0.0f; };
     void Update(const vec &o)
     {
          extern float GetDistance(vec v1, vec v2);
          
          if (nextupdate > lastmillis) return;
          if (GetDistance(o, prevloc[0]) >= 4.0f)
          {
               for(int i=(MAX_STORED_LOCATIONS-1);i>=1;i--)
                    prevloc[i] = prevloc[i-1];
               prevloc[0] = o;
          }
          nextupdate = lastmillis + 100;
     };
};

struct itemstat { int add, start, max, sound; };
// End add


struct md2state
{
    int anim, frame, range, basetime;
    float speed;
    //md2state() : anim(0), frame(0), range(0), basetime(0), speed(100.0f) { };
	md2state()
	{
		anim = frame = range = basetime = 0;
		speed = 100.0f;
	}

    bool operator==(const md2state &o) const { return frame==o.frame && range==o.range && basetime==o.basetime && speed==o.speed; };
    bool operator!=(const md2state &o) const { return frame!=o.frame || range!=o.range || basetime!=o.basetime || speed!=o.speed; };
};


struct md3state
{
    int anim;
    int frm;
    int lastTime;
};

enum // md3 animations
{
    BOTH_DEATH1 = 0,
    BOTH_DEAD1,
    BOTH_DEATH2,
    BOTH_DEAD2,
    BOTH_DEATH3,
    BOTH_DEAD3,
    TORSO_GESTURE,
    TORSO_ATTACK,
    TORSO_ATTACK2,
    TORSO_DROP,
    TORSO_RAISE,
    TORSO_STAND,
    TORSO_STAND2,
    LEGS_WALKCR,
    LEGS_WALK,
    LEGS_RUN,
    LEGS_BACK,
    LEGS_SWIM,
    LEGS_JUMP,
    LEGS_LAND,
    LEGS_JUMPB,
    LEGS_LANDB,
    LEGS_IDLE,
    LEGS_IDLECR,
    LEGS_TURN
};

struct dynent                           // players & monsters
{
    vec o, vel;                         // origin, velocity
    float yaw, pitch, roll;             // used as vec in one place
    float oldpitch;
    float maxspeed;                     // cubes per second, 24 for player
    bool outsidemap;                    // from his eyes
    bool inwater;
    bool onfloor, jumpnext;
    int move, strafe;
    bool k_left, k_right, k_up, k_down; // see input code  
    int timeinair;                      // used for fake gravity
    float radius, eyeheight, aboveeye;  // bounding box size
    int lastupdate, plag, ping;
    int lifesequence;                   // sequence id for each respawn, used in damage test
    int state;                          // one of CS_* below
    int frags;
    int health, armour; //armourtype, quadmillis;
    int akimbomillis;
    int gunselect, gunwait;
    int lastaction, lastattackgun, lastmove;
    bool attacking;
    int ammo[NUMGUNS];
    int mag[NUMGUNS];
    int monsterstate;                   // one of M_* below, M_NONE means human
    int mtype;                          // see monster.cpp
    dynent *enemy;                      // monster wants to kill this entity
    // Added by Rick: targetpitch
    float targetpitch;                    // monster wants to look in this direction
    // End add   
    float targetyaw;                    // monster wants to look in this direction
    bool blocked, moving;               // used by physics to signal ai
    int trigger;                        // millis at which transition to another monsterstate takes place
    vec attacktarget;                   // delayed attacks
    int anger;                          // how many times already hit by fellow monster
    int flagscore;                      // EDIT: AH
    string name, team;
    //int startheight;
    int shots;                          //keeps track of shots from auto weapons
    bool reloading, hasarmour, weaponchanging;
    int nextweapon; // weapon we switch to
    int primary;                        //primary gun
    int nextprimary; // primary after respawning
    int skin, nextskin; // skin after respawning
    md3state animstate[3];
	int lastanimswitchtime;
	md2state current, prev;
	int attackmillis;
    
    bool onladder;
    int gravity;
    bool isphysent; // hack
    
    int thrownademillis;
    struct physent *inhandnade;
    int akimbo;
    int akimbolastaction[2];
	float rotspeed;
    
    // Added by Rick
    CBot *pBot; // Only used if this is a bot, points to the bot class if we are the host,
                // for other clients its NULL
    bool bIsBot; // Is this dynent a bot?
    CPrevLocation PrevLocations; // Previous stored locations of this player
    // End add by Rick      
};
// Moved from server.cpp by Rick
struct server_entity            // server side version of "entity" type
{
    bool spawned;
    int spawnsecs;
};
// End move

// EDIT: AH
enum { CTFF_INBASE = 0, CTFF_STOLEN, CTFF_DROPPED };

struct flaginfo
{
    entity *flag;
    dynent *thief;
    vec originalpos;
    int state; // one of the types above
    bool pick_ack;
    flaginfo() : flag(0), thief(0), state(CTFF_INBASE), pick_ack(false) {};
};

enum { PHYSENT_NONE = 0, NADE_ACTIVATED, NADE_THROWED, GIB};

struct physent : dynent // nades, gibs
{
    int millis, timetolife, state; // see enum above
    dynent *owner;
};

#define NADE_IN_HAND (player1->gunselect==GUN_GRENADE && player1->inhandnade)
#define NADE_THROWING (player1->gunselect==GUN_GRENADE && player1->thrownademillis && !player1->inhandnade)

#define has_akimbo(d) ((d)->gunselect==GUN_PISTOL && (d)->akimbo)

#define SAVEGAMEVERSION 6               // bump if dynent/netprotocol changes or any other savegame/demo data bumped from 5

//enum { A_BLUE, A_GREEN, A_YELLOW };     // armour types... take 20/40/60 % off
enum { M_NONE = 0, M_SEARCH, M_HOME, M_ATTACKING, M_PAIN, M_SLEEP, M_AIMING, M_NADE /* bloody hack*/ };  // monster states

#define MAXCLIENTS 256                  // in a multiplayer game, can be arbitrarily changed
#define MAXTRANS 5000                   // max amount of data to swallow in 1 go
#define CUBE_SERVER_PORT 28765
#define CUBE_SERVINFO_PORT 28766
#define PROTOCOL_VERSION 123            // bump when protocol changes

#define WEAPONCHANGE_TIME 400

// network messages codes, c2s, c2c, s2c
enum
{
    SV_INITS2C = 0, SV_INITC2S, SV_POS, SV_TEXT, SV_SOUND, SV_CDIS,
    SV_GIBDIED, SV_DIED, SV_GIBDAMAGE, SV_DAMAGE, SV_SHOT, SV_FRAGS,
    SV_TIMEUP, SV_EDITENT, SV_MAPRELOAD, SV_ITEMACC,
    SV_MAPCHANGE, SV_ITEMSPAWN, SV_ITEMPICKUP, SV_DENIED,
    SV_PING, SV_PONG, SV_CLIENTPING, SV_GAMEMODE,
    SV_EDITH, SV_EDITT, SV_EDITS, SV_EDITD, SV_EDITE,
    SV_SENDMAP, SV_RECVMAP, SV_SERVMSG, SV_ITEMLIST, SV_WEAPCHANGE,
    SV_MODELSKIN,
    SV_FLAGPICKUP, SV_FLAGDROP, SV_FLAGRETURN, SV_FLAGSCORE, SV_FLAGINFO, SV_FLAGS, //EDIT: AH
    // Added by Rick: Bot specific messages
    SV_BOTSOUND, SV_BOTDIS, SV_BOTDIED, SV_CLIENT2BOTDMG, SV_BOT2BOTDMG,
    SV_BOTFRAGS, SV_ADDBOT, SV_BOTUPDATE, SV_BOTCOMMAND,
    // End add    
    SV_BOTITEMPICKUP, SV_BOT2CLIENTDMG, SV_DIEDBYBOT,
    SV_EXT,
};     

enum { CS_ALIVE = 0, CS_DEAD, CS_LAGGED, CS_EDITING };

// hardcoded sounds, defined in sounds.cfg
enum
{
    S_JUMP = 0, S_LAND,
    S_KNIFE,
    S_PISTOL, S_RPISTOL,
    S_SHOTGUN, S_RSHOTGUN,
    S_SUBGUN, S_RSUBGUN,
    S_SNIPER, S_RSNIPER, 
    S_ASSULT, S_RASSULT,
    S_ITEMAMMO, S_ITEMHEALTH,
    S_ITEMARMOUR, S_ITEMPUP, 
    S_NOAMMO, S_PUPOUT, 
    S_PAIN1, S_PAIN2, S_PAIN3, S_PAIN4, S_PAIN5, S_PAIN6, //24
    S_DIE1, S_DIE2, 
    S_FEXPLODE, 
    S_SPLASH1, S_SPLASH2,
    S_GRUNT1, S_GRUNT2, S_RUMBLE,    
    S_FLAGDROP, S_FLAGPICKUP, S_FLAGRETURN, S_FLAGSCORE,
    S_GRENADEPULL, S_GRENADETHROW,
    S_RAKIMBO, S_GUNCHANGE, S_GIB,
    S_NULL
};

// vertex array format

struct vertex { float u, v, x, y, z; uchar r, g, b, a; }; 

typedef vector<dynent *> dvector;
typedef vector<char *> cvector;
typedef vector<int> ivector;

// globals ooh naughty

extern sqr *world, *wmip[];             // map data, the mips are sequential 2D arrays in memory
extern header hdr;                      // current map header
extern int sfactor, ssize;              // ssize = 2^sfactor
extern int cubicsize, mipsize;          // cubicsize = ssize^2
extern dynent *player1;                 // special client ent that receives input and acts as camera
extern dvector players;                 // all the other clients (in multiplayer)
extern vector<physent *>physents;
extern bool editmode;
extern vector<entity> ents;             // map entities
extern vec worldpos;                    // current target of the crosshair in the world
extern int lastmillis;                  // last time
extern int curtime;                     // current frame time
extern int gamemode, nextmode;
extern int xtraverts;
extern bool demoplayback;


#define DMF 16.0f 
#define DAF 1.0f 
#define DVF 100.0f

#define VIRTW 2400                      // virtual screen size for text & HUD
#define VIRTH 1800
#define FONTH 64
#define PIXELTAB (VIRTW/12)

#define PI  (3.1415927f)
#define PI2 (2*PI)

// simplistic vector ops
#define dotprod(u,v) ((u).x * (v).x + (u).y * (v).y + (u).z * (v).z)
#define vmul(u,f)    { (u).x *= (f); (u).y *= (f); (u).z *= (f); }
#define vdiv(u,f)    { (u).x /= (f); (u).y /= (f); (u).z /= (f); }
#define vadd(u,v)    { (u).x += (v).x; (u).y += (v).y; (u).z += (v).z; };
#define vsub(u,v)    { (u).x -= (v).x; (u).y -= (v).y; (u).z -= (v).z; };
#define vdist(d,v,e,s) vec v = s; vsub(v,e); float d = (float)sqrt(dotprod(v,v));
#define vdistsquared(d,v,e,s) vec v = s; vsub(v,e); float d = (dotprod(v,v));
#define vreject(v,u,max) ((v).x>(u).x+(max) || (v).x<(u).x-(max) || (v).y>(u).y+(max) || (v).y<(u).y-(max))
#define vlinterp(v,f,u,g) { (v).x = (v).x*f+(u).x*g; (v).y = (v).y*f+(u).y*g; (v).z = (v).z*f+(u).z*g; }
// Added by Rick (compares 2 vectors)
#define vis(v1,v2)   ((v1.x==v2.x) && (v1.y==v2.y) && (v1.z==v2.z))
// End add by Rick
#define sgetstr() { char *t = text; int tlen=0; do { *t = getint(p); } while(*t++ && ++tlen<MAXTRANS); text[MAXTRANS-1]=0; } // used by networking

/* Gamemodes
0 - tdm
1 - coop edit
2 - dm
3 - survivor
4 - team survior
5 - ctf
6 - pistols
7 - bot tdm
8 - bot dm
9 - last swiss standing
*/

#define m_noitems     (gamemode==3 || gamemode==4)
#define m_noitemsnade (gamemode==9)
#define m_nogun		  (gamemode==9)
#define m_noitemsrail (false)
#define m_arena       (gamemode==3 || gamemode==4 || gamemode==9)
#define m_tarena      (gamemode==4)
#define m_teammode    (gamemode==0 || gamemode==4 || gamemode==5 || gamemode==7)
#define m_botmode	  (gamemode==7 || gamemode == 8)
#define m_ctf	      (gamemode==5)
#define m_pistol      (gamemode==6)
#define m_lss		  (gamemode==9)
//#define m_sp          (gamemode<0)
//#define m_dmsp        (gamemode==-1)
//#define m_classicsp   (gamemode==-2)
#define isteam(a,b)   (m_teammode && strcmp(a, b)==0)

#define TEAM_CLA 0 //
#define TEAM_RVSF 1 //
// rb means red/blue
#define rb_team_string(t) ((t) ? "RVSF" : "CLA")
#define rb_team_int(t) (strcmp((t), "CLA") == 0 ? TEAM_CLA : TEAM_RVSF)
#define rb_opposite(o) ((o) == TEAM_CLA ? TEAM_RVSF : TEAM_CLA)

enum    // function signatures for script functions, see command.cpp
{
    ARG_1INT, ARG_2INT, ARG_3INT, ARG_4INT,
    ARG_NONE,
    ARG_1STR, ARG_2STR, ARG_3STR, ARG_5STR, ARG_6STR,
    ARG_DOWN, ARG_DWN1,
    ARG_1EXP, ARG_2EXP,
    ARG_1EST, ARG_2EST,
    ARG_VARI
}; 

// nasty macros for registering script functions, abuses globals to avoid excessive infrastructure
#define COMMANDN(name, fun, nargs) static bool __dummy_##fun = addcommand(#name, (void (*)())fun, nargs)
#define COMMAND(name, nargs) COMMANDN(name, name, nargs)
#define VARP(name, min, cur, max) int name = variable(#name, min, cur, max, &name, NULL, true)
#define VAR(name, min, cur, max)  int name = variable(#name, min, cur, max, &name, NULL, false)
#define VARF(name, min, cur, max, body)  void var_##name(); static int name = variable(#name, min, cur, max, &name, var_##name, false); void var_##name() { body; }
#define VARFP(name, min, cur, max, body) void var_##name(); static int name = variable(#name, min, cur, max, &name, var_##name, true); void var_##name() { body; }

#define ATOI(s) strtol(s, NULL, 0)		// supports hexadecimal numbers

#ifdef WIN32
    #define strcasecmp strcmp // fixme
	#define WIN32_LEAN_AND_MEAN
	#include "windows.h"
	#define _WINDOWS
	#define ZLIB_DLL
	#define cos(x) cos((float)(x))
	#define sin(x) sin((float)(x))
#else
	#include <dlfcn.h>
#endif

#include <time.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <SDL.h>
#include <SDL_image.h>

#include <enet/enet.h>

#include <zlib.h>

// Added by Rick
extern ENetHost *clienthost;
inline bool ishost(void) { return !clienthost; };

void splaysound(int n, vec *loc=0);
void addteamscore(dynent *d);
void renderscore(dynent *d);
extern void conoutf(const char *s, ...); // Moved from protos.h
extern void particle_trail(int type, int fade, vec &from, vec &to); // Moved from protos.h
extern bool listenserv;
extern bool intermission;
#include "bot/bot.h"
// End add by Rick

#include "protos.h"				// external function decls

