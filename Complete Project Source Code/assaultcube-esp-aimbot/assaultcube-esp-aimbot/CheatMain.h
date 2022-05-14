#pragma once

#include "game_definitions.h"

#define DEBUGPRINT

void hk_gl_drawhud(int w, int h, int curfps, int nquads, int curvert, bool underwater, int elapsed);

class CheatMain
{
public:
	static HMODULE hMod;
	static uintptr_t ProcessBaseAddress;
	static void ThreadedInitialize(HMODULE hMod);
	static tgl_drawhud ogl_drawhud_trampoline;

	static bool AttachDebugConsole(void);
	static void DbgPrint(const char* fmt, ...);

	static playerent_wrapper* player1;
	static physent_wrapper* camera1;
	static int* VIRTW;
	static font_wrapper* curfont;
	static Matrixf* mvpmatrix;
	static Vector<playerent_wrapper*>* bots;

	static tintersectclosest ointersectclosest;
	static ttext_width otext_width;
	static tdraw_text odraw_text;
};