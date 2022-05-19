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
ttext_width CheatMain::otext_width = NULL;
tdraw_text CheatMain::odraw_text = NULL;
tTraceLine CheatMain::oTraceLine = NULL;

// globals
playerent_wrapper* AimTarget = NULL;
float AimDistance = 3.402823466e+38F;

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

void CheatMain::DrawOutline2d_Color(float xMin, float yMin, float xMax, float yMax, float colorR, float colorG, float colorB)
{
	opengl_wrapper::oglDisable(GL_BLEND);
	opengl_wrapper::oglDisable(GL_TEXTURE_2D);
	opengl_wrapper::oglColor4f(colorR, colorG, colorB, 1.f);
	opengl_wrapper::oglBegin(GL_LINE_LOOP);
	opengl_wrapper::oglVertex2f(xMin, yMin);
	opengl_wrapper::oglVertex2f(xMax, yMin);
	opengl_wrapper::oglVertex2f(xMax, yMax);
	opengl_wrapper::oglVertex2f(xMin, yMax);
	opengl_wrapper::oglEnd();
	opengl_wrapper::oglEnable(GL_BLEND);
	opengl_wrapper::oglEnable(GL_TEXTURE_2D);
}

void DrawLine(float xMin, float yMin, float xMax, float yMax, float colorR, float colorG, float colorB)
{
	opengl_wrapper::oglDisable(GL_BLEND);
	opengl_wrapper::oglDisable(GL_TEXTURE_2D);
	opengl_wrapper::oglColor4f(colorR, colorG, colorB, 1.f);
	opengl_wrapper::oglBegin(GL_LINES);
	opengl_wrapper::oglVertex2f(xMin, yMin);
	opengl_wrapper::oglVertex2f(xMax, yMax);
	opengl_wrapper::oglEnd();
	opengl_wrapper::oglEnable(GL_BLEND);
	opengl_wrapper::oglEnable(GL_TEXTURE_2D);
}

bool WorldToScreen(const Matrixf *transMat, const Vector3f& in, double screenWidth, double screenHeight, Vector3f& out)
{
	// the world-to-screen transformation is some complicated math that i don't fully understand.
	// i usually just copy paste until something works
	// here's some links that may help:
	// https://www.scratchapixel.com/lessons/3d-basic-rendering/computing-pixel-coordinates-of-3d-point/mathematics-computing-2d-coordinates-of-3d-points
	// https://www.3dgep.com/understanding-the-view-matrix/#Transformations
	// https://answers.unity.com/questions/1014337/calculation-behind-cameraworldtoscreenpoint.html
	// https://www.codeproject.com/Articles/42848/A-New-Perspective-on-Viewing
	Vector4f clipCoords(0.f, 0.f, 0.f, 1.f);
	transMat->transform(in, clipCoords);
	if (clipCoords.w < 0.1f) return false;
	Vector3f NDCCoords;
	NDCCoords.x = clipCoords.x / clipCoords.w;
	NDCCoords.y = clipCoords.y / clipCoords.w;
	NDCCoords.z = clipCoords.z / clipCoords.w;
	// normally we'd divide by 2 here as part of the world-to-screen transformation,
	// but the "virtual screen coordinate system" the game uses is actually VIRTW * 2 and VIRTH * 2
	// so we can skip the divide by 2 by not multiplying VIRTW and VIRTH by 2
	out.x = (screenWidth * NDCCoords.x) + (screenWidth + NDCCoords.x);
	out.y = -(screenHeight * NDCCoords.y) + (screenHeight + NDCCoords.y);
	return true;
}

void CheatMain::DrawNametags()
{
	if (player1 == NULL) return;
	if ((bots == NULL) || (bots->count <= 0) || (bots->data[0] == NULL)) return;

	for (int index = 0; index < bots->count; index++)
	{
		playerent_wrapper* bot = bots->data[index];
		if (bot == NULL) continue;

		Vector3f screenPos;
		if (WorldToScreen(mvpmatrix, bot->o, *VIRTW, VIRTH, screenPos) == false)
			continue;

		float dist = Vector3f::dist(player1->o, bot->o);

		if ((bot->state == CS_DEAD) || 
			((player1 == NULL) || (player1->state == CS_SPECTATE) || 
				((player1->state == CS_DEAD) && (player1->spectatemode > SM_NONE))))
			draw_textf(screenPos.x, screenPos.y, 255, 255, 255, "%s [%.2fm]", bot->name, dist);
		else if (bot->team == player1->team)
			draw_textf(screenPos.x, screenPos.y, 0, 255, 0, "%s [%.2fm]", bot->name, dist);
		else
			draw_textf(screenPos.x, screenPos.y, 255, 0, 0, "%s [%.2fm]", bot->name, dist);
	}
}

void CheatMain::DrawPlayerOutlines2d()
{
	if ((bots == NULL) || (bots->count <= 0) || (bots->data[0] == NULL)) return;

	for (int index = 0; index < bots->count; index++)
	{
		playerent_wrapper* bot = bots->data[index];
		if (bot == NULL) continue;

		Vector3f botMin(bot->o.x - bot->radius, bot->o.y, bot->o.z - bot->eyeheight);
		Vector3f botMax(bot->o.x + bot->radius, bot->o.y + bot->radius, bot->o.z);

		Vector3f botScreenMin, botScreenMax;
		bool onScreenMin, onScreenMax;
		onScreenMin = WorldToScreen(mvpmatrix, botMin, *VIRTW, VIRTH, botScreenMin);
		onScreenMax = WorldToScreen(mvpmatrix, botMax, *VIRTW, VIRTH, botScreenMax);

		if ((onScreenMin == false) || (onScreenMax == false)) continue;

		if ((bot->state == CS_DEAD) ||
			((player1 == NULL) || (player1->state == CS_SPECTATE) ||
				((player1->state == CS_DEAD) && (player1->spectatemode > SM_NONE))))
			DrawOutline2d_Color(botScreenMin.x, botScreenMin.y, botScreenMax.x, botScreenMax.y, 1.f, 1.f, 1.f);
		else if (bot == AimTarget)
			DrawOutline2d_Color(botScreenMin.x, botScreenMin.y, botScreenMax.x, botScreenMax.y, 1.f, 0.f, 1.f);
		else if (bot->team == player1->team)
			DrawOutline2d_Color(botScreenMin.x, botScreenMin.y, botScreenMax.x, botScreenMax.y, 0.f, 1.f, 0.f);
		else
			DrawOutline2d_Color(botScreenMin.x, botScreenMin.y, botScreenMax.x, botScreenMax.y, 1.f, 0.f, 0.f);

		// used for figuring out whether positive z is 'up' (it is)
		// and whether physent->o was already at eye level (it is)
		botMin.x -= 0.5f;
		botMin.z = bot->o.z;
		botMax.x += 0.5f;
		botMax.z = bot->o.z;
		Vector3f expMin, expMax;
		onScreenMin = WorldToScreen(mvpmatrix, botMin, *VIRTW, VIRTH, expMin);
		onScreenMax = WorldToScreen(mvpmatrix, botMax, *VIRTW, VIRTH, expMax);
		// yellow line at head-level
		DrawLine(expMin.x, expMin.y, expMax.x, expMax.y, 1.f, 1.f, 0.f);
		botMin.z = bot->o.z - bot->eyeheight;
		botMax.z = bot->o.z - bot->eyeheight;
		onScreenMin = WorldToScreen(mvpmatrix, botMin, *VIRTW, VIRTH, expMin);
		onScreenMax = WorldToScreen(mvpmatrix, botMax, *VIRTW, VIRTH, expMax);
		// cyan line at foot-level
		DrawLine(expMin.x, expMin.y, expMax.x, expMax.y, 0.f, 1.f, 1.f);

	}
}

void CheatMain::TraceLine(Vector3f from, Vector3f to, dynent_wrapper* pTracer, bool CheckPlayers, traceresult_wrapper* tr)
{
	__asm
	{
		push 0
		push tr
		push to.z
		push to.y
		push to.x
		push from.z
		push from.y
		push from.x
		mov ecx, pTracer
		mov dl, CheckPlayers
		call oTraceLine
		add esp, 0x20
	}
}

playerent_wrapper* CheatMain::FindNearestTarget()
{
	if ((player1 == NULL) || (player1->state != CS_ALIVE)) return NULL;
	if ((bots == NULL) || (bots->count <= 0) || (bots->data[0] == NULL)) return NULL;

	playerent_wrapper* closestTarget = NULL;
	for (int index = 0; index < bots->count; index++)
	{
		playerent_wrapper* bot = bots->data[index];
		if ((bot == NULL) || (bot->state == CS_DEAD) || (bot->team == player1->team)) continue;

		traceresult_wrapper tr{};
		TraceLine(player1->o, bot->o, player1, false, &tr);
		if (tr.collided == false)
		{
			float distSquared = Vector3f::squareddist(player1->o, bot->o);
			if (distSquared < AimDistance)
			{
				AimDistance = distSquared;
				closestTarget = bot;
			}
		}
	}

	return closestTarget;
}

// credit: Rake / h4nsbr1x via [https://guidedhacking.com/threads/ultimate-calcangle-thread-how-to-calculate-angle-functions.8165/#post-73276]
Vector3f CalcAngle(Vector3f src, Vector3f dst)
{
	Vector3f angle;
	float dist = Vector3f::dist(src, dst);

	angle.x = -atan2f(dst.x - src.x, dst.y - src.y) / PI * 180.0f + 180.0f;
	angle.y = asinf((dst.z - src.z) / dist) * Rad2Deg;
	angle.z = 0.0f;

	return angle;
}

void CheatMain::DoAimbot()
{
	if ((player1 == NULL) || (player1->state != CS_ALIVE)) return;

	AimDistance = 3.402823466e+38F;
	AimTarget = FindNearestTarget();

	if (AimTarget != NULL)
	{
		Vector3f aimAngle = CalcAngle(player1->o, AimTarget->o);
		player1->rotation.x = aimAngle.x;
		player1->rotation.y = aimAngle.y;
	}
}

void hk_gl_drawhud(int w, int h, int curfps, int nquads, int curvert, bool underwater, int elapsed)
{
	CheatMain::SetupHUDDrawing();
	CheatMain::draw_text("Hello World", 100, 500, 255, 255, 255);

	CheatMain::draw_textf(100, 600, 255, 255, 0, "MyModule: %tx", CheatMain::hMod);
	if (CheatMain::player1 != NULL)
	{
		playerent_wrapper* localPlayer = CheatMain::player1;
		CheatMain::draw_textf(100, 800, 255, 255, 0, "Player1->o: (%.2f, %.2f, %.2f)", localPlayer->o.x, localPlayer->o.y, localPlayer->o.z);
	}
	if (CheatMain::camera1 != NULL)
	{
		physent_wrapper* camera = CheatMain::camera1;
		CheatMain::draw_textf(100, 850, 255, 255, 0, "Camera1->o: (%.2f, %.2f, %.2f)", camera->o.x, camera->o.y, camera->o.z);
	}

	if ((CheatMain::bots != NULL) && (CheatMain::bots->count > 0) && (CheatMain::bots->data[0] != NULL))
	{
		CheatMain::draw_textf(100, 900, 255, 255, 255, "Bot[0]: %tx (@ %tx)\nBot[0]->name: %s (@ %tx)\nBot[0]->o: (%.2f, %.2f, %.2f)",
			CheatMain::bots->data[0], &CheatMain::bots->data[0],
			CheatMain::bots->data[0]->name, &(CheatMain::bots->data[0]->name),
			CheatMain::bots->data[0]->o.x, CheatMain::bots->data[0]->o.y, CheatMain::bots->data[0]->o.z);
	}

	if (AimTarget != NULL)
	{
		CheatMain::draw_textf(2000, 500, 255, 255, 0, "ClosestTarget: %s (%.2f)", AimTarget->name, AimDistance);
	}

	// ESP functionality
	CheatMain::DrawNametags();
	CheatMain::DrawPlayerOutlines2d();

	// aimbot functionality (who would've thunk it)
	CheatMain::DoAimbot();

	// used for debugging :)
	//CheatMain::DrawOutline2d_Color(100, 100, 1000, 1000, 1.f, 0.f, 0.f);

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

	CheatMain::otext_width = (ttext_width)(0x46E370);
	CheatMain::odraw_text = (tdraw_text)(0x46DD20);
	CheatMain::oTraceLine = (tTraceLine)(0x509010);

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
	
	if ((CheatMain::bots != NULL) && (CheatMain::bots->count > 0) && (CheatMain::bots->data[0] != NULL))
	{
		DbgPrint("Bots: %tx\n&Bots->count: %tx\nBots->count: %d\n", bots, &bots->count, bots->count);
		DbgPrint("*Bot[0]: %tx\n&Bot[0]: %tx\n*Bot[0]->name: %s\n", bots->data[0], &bots->data[0], (bots->data[0])->name);
	}
}