#pragma once

#include "game_definitions.h"

#define DEBUGPRINT

#define VIRTH 1800
#define CS_ALIVE 0x0
#define CS_DEAD 0x1
#define CS_SPECTATE 0x5

#define SM_NONE 0x0

void hk_gl_drawhud(int w, int h, int curfps, int nquads, int curvert, bool underwater, int elapsed);

class CheatMain
{
public:
	// we want this class to be purely static
	CheatMain() = delete;
	CheatMain(const CheatMain&) = delete;
	CheatMain& operator=(const CheatMain& o) = delete;
	~CheatMain() = delete;

	static HMODULE hMod;
	static uintptr_t ProcessBaseAddress;
	static void ThreadedInitialize(HMODULE hMod);
	static tgl_drawhud ogl_drawhud_trampoline;

	static bool AttachDebugConsole(void);
	static void DbgPrint(const char* fmt, ...);

	static void SetupHUDDrawing();
	static void draw_textf(int x, int y, int r, int g, int b, const char* fmt, ...);
	static void draw_text(const char* str, int x, int y, int r, int g, int b, int a = 255, int cursor = -1, int maxwidth = -1);
	static void DrawOutline2d_Color(float xMin, float yMin, float xMax, float yMax, float colorR, float colorG, float colorB);

	static void DrawNametags();
	static void DrawPlayerOutlines2d();

	static void TraceLine(Vector3f from, Vector3f to, dynent_wrapper* pTracer, bool CheckPlayers, traceresult_wrapper* tr);
	static playerent_wrapper* FindNearestTarget();
	static void DoAimbot();

	static playerent_wrapper* player1;
	static physent_wrapper* camera1;
	static int* VIRTW;
	static font_wrapper* curfont;
	static Matrixf* mvpmatrix;
	static Vector<playerent_wrapper*>* bots;

	static ttext_width otext_width;
	static tdraw_text odraw_text;
	static tTraceLine oTraceLine;
};