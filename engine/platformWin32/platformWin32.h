//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMWIN32_H_
#define _PLATFORMWIN32_H_

// define this so that we can use WM_MOUSEWHEEL messages...
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define POINTER_64

#include <windows.h>
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#if defined(TORQUE_COMPILER_CODEWARRIOR)
#  include <ansi_prefix.win32.h>
#  include <stdio.h>
#  include <string.h>
#else
#  include <stdio.h>
#  include <string.h>
#endif

#if defined(TORQUE_COMPILER_VISUALC) || defined(TORQUE_COMPILER_GCC2)
#define vsnprintf _vsnprintf
#define stricmp _stricmp
#define strnicmp _strnicmp
#define strupr _strupr
#define strlwr _strlwr
#endif

#define NOMINMAX


struct Win32PlatState
{
   FILE *log_fp;
   HINSTANCE hinstOpenGL;
   HINSTANCE hinstGLU;
   HINSTANCE hinstOpenAL;
   HWND appWindow;
   HDC appDC;
   HINSTANCE appInstance;
   HGLRC hGLRC;
   DWORD processId;

   S32 desktopBitsPixel;
   S32 desktopWidth;
   S32 desktopHeight;
   U32 currentTime;
   
   Win32PlatState();
};

extern Win32PlatState winState;

extern bool GL_Init( const char *dllname_gl, const char *dllname_glu );
extern bool GL_EXT_Init();
extern void GL_Shutdown();

extern HWND CreateOpenGLWindow( U32 width, U32 height, bool fullScreen );
extern HWND CreateCurtain( U32 width, U32 height );
extern void CreatePixelFormat( PIXELFORMATDESCRIPTOR *pPFD, S32 colorBits, S32 depthBits, S32 stencilBits, bool stereo );
extern S32  ChooseBestPixelFormat( HDC hDC, PIXELFORMATDESCRIPTOR *pPFD );
extern void setModifierKeys( S32 modKeys );

#define WGLD3D_FUNCTION(fn_type, fn_name, fn_args, fn_value) extern fn_type (__stdcall *dwgl##fn_name)fn_args;
#define WGL_FUNCTION(fn_type, fn_name, fn_args, fn_value) extern fn_type (__stdcall *d##fn_name)fn_args;
#include "platformWin32/GLWinFunc.h"
#undef WGL_FUNCTION
#undef WGLD3D_FUNCTION

#define WGLEXT_FUNCTION(fn_type, fn_name, fn_args, fn_value) extern fn_type (__stdcall *d##fn_name)fn_args;
#include "platformWin32/GLWinExtFunc.h"
#undef WGLEXT_FUNCTION

#endif //_PLATFORMWIN32_H_
