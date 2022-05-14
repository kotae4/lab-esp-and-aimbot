#include "pch.h"
#include "CheatMain.h"
#include "opengl_wrapper.h"
// for vararg processing in CheatMain::DbgPrint and CheatMain::draw_textf
#include <stdio.h>
#include <stdarg.h>

HMODULE CheatMain::hMod = NULL;
uintptr_t CheatMain::ProcessBaseAddress = NULL;
tgl_drawhud CheatMain::ogl_drawhud_trampoline = NULL;
playerent_wrapper* CheatMain::player1 = NULL;
physent_wrapper* CheatMain::camera1 = NULL;
int* CheatMain::VIRTW = NULL;
font_wrapper* CheatMain::curfont = NULL;
Matrixf* CheatMain::mvpmatrix = NULL;
Vector<playerent_wrapper*>* CheatMain::bots = NULL;
tintersectclosest CheatMain::ointersectclosest = NULL;
ttext_width CheatMain::otext_width = NULL;
tdraw_text CheatMain::odraw_text = NULL;

bool CheatMain::AttachDebugConsole(void)
{
	if (FreeConsole())
	{
		if (!AllocConsole())
			return false;
	}
	else if (!AllocConsole())
		return false;

	FILE* pConOutput = nullptr;
	return (freopen_s(&pConOutput, "CONOUT$", "w", stdout) == 0);
}

void CheatMain::DbgPrint(const char* fmt, ...)
{
#ifdef DEBUGPRINT
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
#endif
	return;
}

void CheatMain::SetupHUDDrawing()
{
	opengl_wrapper::oglDisable(GL_DEPTH_TEST);
	opengl_wrapper::oglMatrixMode(GL_MODELVIEW);
	opengl_wrapper::oglLoadIdentity();
	opengl_wrapper::oglMatrixMode(GL_PROJECTION);
	opengl_wrapper::oglLoadIdentity();
	opengl_wrapper::oglEnable(GL_BLEND);
	opengl_wrapper::oglEnable(GL_TEXTURE_2D);
	opengl_wrapper::oglOrtho(0, (double)*VIRTW * 2, (double)VIRTH * 2, 0, -1, 1);
}

void CheatMain::draw_textf(int x, int y, int r, int g, int b, const char* fmt, ...)
{
	char buf[260];
	va_list args; 
	va_start(args, fmt); 
	int retVal = vsnprintf_s(buf, 260, _TRUNCATE, fmt, args);
	va_end(args);

	draw_text(buf, x, y, r, g, b);
}

void CheatMain::draw_text(const char* str, int x, int y, int r, int g, int b, int a /*= 255*/, int cursor /*= -1*/, int maxwidth /*= -1*/)
{
	__asm
	{
		mov ecx, str
		mov edx, x
		push maxwidth
		push cursor
		push a
		push b
		push g
		push r
		push y
		call odraw_text
		add esp, 28
	}
}

void hk_gl_drawhud(int w, int h, int curfps, int nquads, int curvert, bool underwater, int elapsed)
{
	CheatMain::SetupHUDDrawing();
	CheatMain::draw_text("Hello World", 100, 100, 255, 255, 255);

	CheatMain::draw_textf(100, 200, 255, 255, 0, "MyModule: %tx", CheatMain::hMod);

	CheatMain::ogl_drawhud_trampoline(w, h, curfps, nquads, curvert, underwater, elapsed);
}

void CheatMain::ThreadedInitialize(HMODULE hMod)
{
	CheatMain::hMod = hMod;
	CheatMain::ProcessBaseAddress = (uintptr_t)GetModuleHandleA(NULL);
#ifdef DEBUGPRINT
	CheatMain::AttachDebugConsole();
#endif

	if (opengl_wrapper::Initialize() == false)
	{
		DbgPrint("Failed to initialize opengl_wrapper\n");
		return;
	}

	CheatMain::player1 = *(playerent_wrapper**)(0x58ac00);
	CheatMain::camera1 = *(physent_wrapper**)(0x57e0a8);

	CheatMain::VIRTW = (int*)(0x57ed2c);
	CheatMain::curfont = (font_wrapper*)(0x57ED28);
	CheatMain::mvpmatrix = (Matrixf*)(0x57dfd0);
	CheatMain::bots = (Vector<playerent_wrapper*>*)(0x591FCC);

	CheatMain::ointersectclosest = (tintersectclosest)(0x4CA250);
	CheatMain::otext_width = (ttext_width)(0x46E370);
	CheatMain::odraw_text = (tdraw_text)(0x46DD20);

	MH_STATUS mhStatus = MH_Initialize();
	if (mhStatus != MH_OK)
	{
		DbgPrint("Could not initialize MinHook (status: %d)\n", mhStatus);
		return;
	}

	uintptr_t gl_drawhud_address = 0x45F1C0;
	mhStatus = MH_CreateHook(reinterpret_cast<LPVOID>(gl_drawhud_address), &hk_gl_drawhud,  reinterpret_cast<LPVOID*>(&CheatMain::ogl_drawhud_trampoline));
	if (mhStatus != MH_OK)
	{
		DbgPrint("Could not create gl_drawhud hook (status: %d)\n", mhStatus);
		return;
	}

	mhStatus = MH_EnableHook(reinterpret_cast<LPVOID>(gl_drawhud_address));
	if (mhStatus != MH_OK)
	{
		DbgPrint("Could not enable gl_drawhud hook (status: %d)\n", mhStatus);
		return;
	}

	DbgPrint("Successfully initialized.\n");

	DbgPrint("Player: %tx\n&Player->health: %tx\nPlayer->health: %d\n", player1, &player1->health, player1->health);
	DbgPrint("Camera: %tx\n&Camera->o: %tx\nCamera->o: (%.2f, %.2f, %.2f)\n", camera1, &camera1->o, camera1->o.x, camera1->o.y, camera1->o.z);
	DbgPrint("Bots: %tx\n&Bots->count: %tx\nBots->count: %d\n", bots, &bots->count, bots->count);
}