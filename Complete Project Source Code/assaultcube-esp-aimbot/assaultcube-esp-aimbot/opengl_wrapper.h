#pragma once


typedef void(__stdcall *tglMatrixMode)(int mode);
typedef void(__stdcall* tglPushMatrix)(void);
typedef void(__stdcall* tglPopMatrix)(void);
typedef void(__stdcall* tglLoadIdentity)(void);
typedef void(__stdcall* tglOrtho)(double left, double right, double bottom, double top, double nearVal, double farVal);
typedef void(__stdcall* tglFrustum)(double left, double right, double bottom, double top, double nearVal, double farVal);
typedef void(__stdcall* tglEnable)(int cap);
typedef void(__stdcall* tglDisable)(int cap);
typedef void(__stdcall* tglBlendFunc)(int sfactor, int dfactor);
typedef void(__stdcall* tglBegin)(int mode);
typedef void(__stdcall* tglEnd)(void);
typedef void(__stdcall* tglColor4f)(float red, float green, float blue, float alpha);
typedef void(__stdcall* tglVertex2f)(float x, float y);
typedef void(__stdcall* tglVertex3f)(float x, float y, float z);
typedef void(__stdcall* tglRotatef)(float angle, float x, float y, float z);
typedef void(__stdcall* tglScalef)(float x, float y, float z);
typedef void(__stdcall* tglTranslatef)(float x, float y, float z);
typedef void(__stdcall* tglGetFloatv)(int pname, float* params);

#define GL_DEPTH_TEST 0xb71
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_BLEND 0xbe2
#define GL_SRC_ALPHA 0x302
#define GL_ONE_MINUS_SRC_ALPHA 0x303
#define GL_TRIANGLE_STRIP 0x5
#define GL_ONE 0x1
#define GL_TEXTURE_2D 0xde1

class opengl_wrapper
{
public:
	// we want this class to be purely static
	opengl_wrapper() = delete;
	opengl_wrapper(const opengl_wrapper&) = delete;
	opengl_wrapper& operator=(const opengl_wrapper& o) = delete;
	~opengl_wrapper() = delete;


	// static fields

	static tglMatrixMode oglMatrixMode;
	static tglPushMatrix oglPushMatrix;
	static tglPopMatrix oglPopMatrix;
	static tglLoadIdentity oglLoadIdentity;
	static tglOrtho oglOrtho;
	static tglFrustum oglFrustum;
	static tglEnable oglEnable;
	static tglDisable oglDisable;
	static tglBlendFunc oglBlendFunc;
	static tglBegin oglBegin;
	static tglEnd oglEnd;
	static tglColor4f oglColor4f;
	static tglVertex2f oglVertex2f;
	static tglVertex3f oglVertex3f;
	static tglRotatef oglRotatef;
	static tglScalef oglScalef;
	static tglTranslatef oglTranslatef;
	static tglGetFloatv oglGetFloatv;

	// static methods
	
	// loads function addresses via GetProcAddress, returns false if any fail.
	static bool Initialize();
};