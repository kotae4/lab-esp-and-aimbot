#pragma once

#include <stdint.h>
#include <math.h>

#define PI    (3.141592654f)
#define Rad2Deg (57.295779513082320876798154814105f)

struct Vector3f
{
	float x, y, z;
	Vector3f() { x = y = z = 0; }
	Vector3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

	static float squareddist(const Vector3f& v1, const Vector3f& v2)
	{
		Vector3f diff{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
		return ((diff.x * diff.x) + (diff.y * diff.y) + (diff.z * diff.z));
	}

	static float dist(const Vector3f& v1, const Vector3f& v2)
	{
		Vector3f diff{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
		return sqrtf((diff.x * diff.x) + (diff.y * diff.y) + (diff.z * diff.z));
	}
};

struct Vector4f
{
	float x, y, z, w;
};

struct Matrixf
{
	float data[16];

	void transform(const Vector3f& in, Vector4f& out) const
	{
		out.x = ((in.x * data[0]) + (in.y * data[4]) + (in.z * data[8]) + data[12]);
		out.y = ((in.x * data[1]) + (in.y * data[5]) + (in.z * data[9]) + data[13]);
		out.z = ((in.x * data[2]) + (in.y * data[6]) + (in.z * data[10]) + data[14]);
		out.w = ((in.x * data[3]) + (in.y * data[7]) + (in.z * data[11]) + data[15]);
	}
};

template<typename T>
struct Vector
{
	T* data;
	int capacity;
	int count;
};

class font_wrapper
{
public:
	char pad_0000[24]; //0x0000
	int32_t defaulth; //0x0018
}; //Size: 0x001C
static_assert(sizeof(font_wrapper) == 0x1C);

class physent_wrapper
{
public:
	Vector3f o; //0x0000
	Vector3f vel; //0x000C
	Vector3f deltapos; //0x0018
	Vector3f newpos; //0x0024
	Vector3f rotation; //0x0030
	float pitchvel; //0x003C
	float maxspeed; //0x0040
	int32_t timeinair; //0x0044
	float radius; //0x0048
	float eyeheight; //0x004C
	float maxeyeheight; //0x0050
	float aboveeye; //0x0054
	bool inwater; //0x0058
	bool onfloor; //0x0059
	bool onladder; //0x005A
	bool jumpnext; //0x005B
	bool jumpd; //0x005C
	bool crouching; //0x005D
	bool crouchedinair; //0x005E
	bool trycrouch; //0x005F
	bool cancollide; //0x0060
	bool stuck; //0x0061
	bool scoping; //0x0062
	char pad_0063[1]; //0x0063
	int32_t lastjump; //0x0064
	float lastjumpheight; //0x0068
	int32_t lastsplash; //0x006C
	int8_t move; //0x0070
	int8_t strafe; //0x0071
	uint8_t state; //0x0072
	uint8_t type; //0x0073
	float eyeheightvel; //0x0074
	int32_t last_pos; //0x0078

	virtual ~physent_wrapper();
	virtual void oncollision();
	virtual void onmoved(const Vector3f& dist);
}; //Size: 0x007C
static_assert(sizeof(physent_wrapper) == 0x0080);

class animstate_wrapper
{
public:
	int32_t anim; //0x0000
	int32_t frame; //0x0004
	int32_t range; //0x0008
	int32_t basetime; //0x000C
	float speed; //0x0010
}; //Size: 0x0014
static_assert(sizeof(animstate_wrapper) == 0x14);

class dynent_wrapper : public physent_wrapper
{
public:
	bool k_left; //0x0000
	bool k_right; //0x0001
	bool k_up; //0x0002
	bool k_down; //0x0003
	class animstate_wrapper prev[2]; //0x0004
	class animstate_wrapper current[2]; //0x002C
	int32_t lastanimswitchtime[2]; //0x0054
	void* lastmodel[2]; //0x005C
	int32_t lastrendered; //0x0064

	virtual ~dynent_wrapper();
}; //Size: 0x0068
static_assert(sizeof(dynent_wrapper) == sizeof(physent_wrapper) + 0x0068);

class playerstate_wrapper
{
public:
	int32_t health; //0x0000
	int32_t armour; //0x0004
	int32_t primary; //0x0008
	int32_t nextprimary; //0x000C
	int32_t gunselect; //0x0010
	bool akimbo; //0x0014
	char pad_0015[3]; //0x0015
	int32_t ammo[9]; //0x0018
	int32_t mag[9]; //0x003C
	int32_t gunwait[9]; //0x0060
	int32_t pstatshots[9]; //0x0084
	int32_t pstatdamage[9]; //0x00A8

	virtual ~playerstate_wrapper();
	virtual void spawnstate(int gamemode);
}; //Size: 0x00CC
static_assert(sizeof(playerstate_wrapper) == 0xD0);

class poshist_wrapper
{
public:
	int32_t nextupdate; //0x0000
	int32_t curpos; //0x0004
	int32_t numpos; //0x0008
	Vector3f pos[7]; //0x000C
}; //Size: 0x0060
static_assert(sizeof(poshist_wrapper) == 0x60);

class playerent_wrapper : public dynent_wrapper, public playerstate_wrapper
{
public:
	int32_t curskin; //0x01B8
	int32_t nextskin[2]; //0x01BC
	int32_t clientnum; //0x01C4
	int32_t lastupdate; //0x01C8
	int32_t plag; //0x01CC
	int32_t ping; //0x01D0
	uint32_t address; //0x01D4
	int32_t lifesequence; //0x01D8
	int32_t frags; //0x01DC
	int32_t flagscore; //0x01E0
	int32_t deaths; //0x01E4
	int32_t tks; //0x01E8
	int32_t lastaction; //0x01EC
	int32_t lastmove; //0x01F0
	int32_t lastpain; //0x01F4
	int32_t lastvoicecom; //0x01F8
	int32_t lastdeath; //0x01FC
	int32_t clientrole; //0x0200
	bool attacking; //0x0204
	char name[260]; //0x0205
	char pad_0309[3]; //0x0309
	int32_t team; //0x030C
	int32_t weaponchanging; //0x0310
	int32_t nextweapon; //0x0314
	int32_t spectatemode; //0x0318
	int32_t followplayercn; //0x031C
	int32_t eardamagemillis; //0x0320
	float maxroll; //0x0324
	float maxrolleffect; //0x0328
	float movroll; //0x032C
	float effroll; //0x0330
	int32_t ffov; //0x0334
	int32_t scopefov; //0x0338
	class weapon_wrapper* weapons[9]; //0x033C
	class weapon_wrapper* prevweaponsel; //0x0360
	class weapon_wrapper* weaponsel; //0x0364
	class weapon_wrapper* nextweaponsel; //0x0368
	class weapon_wrapper* primweap; //0x036C
	class weapon_wrapper* nextprimweap; //0x0370
	class weapon_wrapper* lastattackweapon; //0x0374
	class poshist_wrapper history; //0x0378
	char* skin_noteam; //0x03D8
	char* skin_cla; //0x03DC
	char* skin_rvsf; //0x03E0
	float deltayaw; //0x03E4
	float deltapitch; //0x03E8
	float newyaw; //0x03EC
	float newpitch; //0x03F0
	int32_t smoothmilis; //0x03F4
	Vector3f head; //0x03F8
	bool ignored; //0x0404
	bool muted; //0x0405
	bool nocorpse; //0x0406

	virtual ~playerent_wrapper();
}; //Size: 0x0408
static_assert(sizeof(playerent_wrapper) == sizeof(dynent_wrapper) + sizeof(playerstate_wrapper) + 0x250);

class weapon_wrapper
{
public:
	int32_t type; //0x0004
	class playerent_wrapper* owner; //0x0008
	class guninfo_wrapper* info; //0x000C
	int32_t* ammo; //0x0010
	int32_t* mag; //0x0014
	int32_t* gunwait; //0x0018
	int32_t shots; //0x001C
	int32_t reloading; //0x0020
	int32_t lastaction; //0x0024

	virtual ~weapon_wrapper();
	virtual int dynspread();
	virtual float dynrecoil();
	virtual bool attack(Vector3f& targ);
	virtual void attackfx(const Vector3f& from, const Vector3f& to, int millis);
	virtual void attackphysics(Vector3f& from, Vector3f& to);
	virtual void attacksound();
	virtual bool reload(bool autoreloaded);
	virtual void reset();
	virtual bool busy();
	virtual int modelanim();
	virtual void updatetimers(int millis);
	virtual bool selectable();
	virtual bool deselectable();
	virtual void renderstats();
	virtual void renderhudmodel();
	virtual void renderaimhelp(bool teamwarning);
	virtual void onselecting(bool sound);
	virtual void ondeselecting();
	virtual void onammopicked();
	virtual void onownerdies();
	virtual void removebounceent(uintptr_t* b);
	virtual int flashtime() const;

}; //Size: 0x0028
static_assert(sizeof(weapon_wrapper) == 0x28);

class guninfo_wrapper
{
public:
	char modelname[23]; //0x0000
	char title[42]; //0x0017
	char pad_0041[1]; //0x0041
	int16_t sound; //0x0042
	int16_t reload; //0x0044
	int16_t reloadtime; //0x0046
	int16_t attackdelay; //0x0048
	int16_t damage; //0x004A
	int16_t piercing; //0x004C
	int16_t projspeed; //0x004E
	int16_t part; //0x0050
	int16_t spread; //0x0052
	int16_t recoil; //0x0054
	int16_t magsize; //0x0056
	int16_t mdl_kick_rot; //0x0058
	int16_t mdl_kick_back; //0x005A
	int16_t recoilincrease; //0x005C
	int16_t recoilbase; //0x005E
	int16_t maxrecoil; //0x0060
	int16_t recoilbackfade; //0x0062
	int16_t pushfactor; //0x0064
	bool isauto; //0x0066
	char pad_0067[1]; //0x0067
}; //Size: 0x0068
static_assert(sizeof(guninfo_wrapper) == 0x68);

class traceresult_wrapper
{
public:
	Vector3f end; //0x0000
	bool collided; //0x000C
}; //Size: 0x000D
static_assert(sizeof(traceresult_wrapper) == 0x10);

// function definitions

typedef void(__cdecl* tgl_drawhud)(int w, int h, int curfps, int nquads, int curvert, bool underwater, int elapsed);
typedef int(__cdecl* ttext_width)(const char* str);
typedef void(__cdecl* tdraw_text)(const char* str, int left, int top, int r, int g, int b, int a, int cursor, int maxwidth);
typedef void(__cdecl* tTraceLine)(Vector3f from, Vector3f to, dynent_wrapper* pTracer, bool CheckPlayers, traceresult_wrapper* tr, bool SkipTags);