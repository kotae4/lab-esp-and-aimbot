#include "pch.h"
#include "opengl_wrapper.h"

tglMatrixMode opengl_wrapper::oglMatrixMode = NULL;
tglPushMatrix opengl_wrapper::oglPushMatrix = NULL;
tglPopMatrix opengl_wrapper::oglPopMatrix = NULL;
tglLoadIdentity opengl_wrapper::oglLoadIdentity = NULL;
tglOrtho opengl_wrapper::oglOrtho = NULL;
tglFrustum opengl_wrapper::oglFrustum = NULL;
tglEnable opengl_wrapper::oglEnable = NULL;
tglDisable opengl_wrapper::oglDisable = NULL;
tglBlendFunc opengl_wrapper::oglBlendFunc = NULL;
tglBegin opengl_wrapper::oglBegin = NULL;
tglEnd opengl_wrapper::oglEnd = NULL;
tglColor4f opengl_wrapper::oglColor4f = NULL;
tglVertex2f opengl_wrapper::oglVertex2f = NULL;
tglVertex3f opengl_wrapper::oglVertex3f = NULL;
tglRotatef opengl_wrapper::oglRotatef = NULL;
tglScalef opengl_wrapper::oglScalef = NULL;
tglTranslatef opengl_wrapper::oglTranslatef = NULL;
tglGetFloatv opengl_wrapper::oglGetFloatv = NULL;

bool opengl_wrapper::Initialize()
{
	HMODULE openGLBase = GetModuleHandleA("OPENGL32.dll");
	if (openGLBase == NULL)
	{
		return false;
	}

#define LoadFcnAddr(fnName) o##fnName = (t##fnName)GetProcAddress(openGLBase, #fnName); \
		if (o##fnName == NULL) \
		{ \
			return false; \
		}

// absolute madness: https://www.scs.stanford.edu/~dm/blog/va-opt.html
// effective madness though...
#define PARENS ()

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH(macro, ...)                                    \
  __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define FOR_EACH_HELPER(macro, a1, ...)                         \
  macro(a1)                                                     \
  __VA_OPT__(FOR_EACH_AGAIN PARENS (macro, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER

#define LoadAllFcnAddr(...) FOR_EACH(LoadFcnAddr, __VA_ARGS__)

	LoadAllFcnAddr(glMatrixMode, glPushMatrix, glPopMatrix, glLoadIdentity,
		glOrtho, glFrustum, glEnable, glDisable, glBlendFunc, glBegin,
		glEnd, glColor4f, glVertex2f, glVertex3f, glRotatef, glScalef, glTranslatef, glGetFloatv);


	return true;
}
