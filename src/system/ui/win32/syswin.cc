/* 
 *	PearPC
 *	sysx11.cc
 *
 *	Copyright (C) 1999-2002 Stefan Weyergraf (stefan@weyergraf.de)
 *	Copyright (C) 1999-2004 Sebastian Biallas (sb@biallas.net)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>

// for stopping the CPU
#include "cpu_generic/ppc_cpu.h"

#undef FASTCALL

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <process.h>

#undef FASTCALL

#include "system/display.h"
#include "system/keyboard.h"
#include "system/mouse.h"

#include "tools/snprintf.h"

#include "syswin.h"

HWND gHWNDMain;
CRITICAL_SECTION gDrawCS;
int gMenuHeight; 
BITMAPINFO gMenuBitmapInfo;
byte *menuData;

static HINSTANCE gHInst;

static byte scancode_to_ascii[] = {
//00   01   02   03   04   05   06   07   08   09   0a   0b   0c   0d   0e   0f
0x00,'\e', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',0x08,'\t',
 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']','\n',0x00, 'a', 's',
 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',0x00,0x00,0x00,'\\', 'z', 'x', 'c', 'v', 
 'b', 'n', 'm', ',', '.', '/',0x00,0x00,0x00, ' ',0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

//00   01   02   03   04   05   06   07   08   09   0a   0b   0c   0d   0e   0f
0x00,'\e', '1', '2', '3', '4', '5', '6', '7', '*', '(', ')', '_', '+',0x08,'\t',
 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']','\n',0x00, 'A', 'S',
 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',0x00,0x00,0x00,'\\', 'Z', 'X', 'C', 'V', 
 'B', 'N', 'M', '<', '>', '?',0x00,0x00,0x00, ' ',0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

byte scancode_to_mackey[] = {
//00   01   02   03   04   05   06   07   08   09   0a   0b   0c   0d   0e   0f
 0xff,0x35,0x12,0x13,0x14,0x15,0x17,0x16,0x1a,0x1c,0x19,0x1d,0x1b,0x18,0x33,0x30,
 0x0c,0x0d,0x0e,0x0f,0x11,0x10,0x20,0x22,0x1f,0x23,0x21,0x1e,0x24,0x36,0x00,0x01,
 0x02,0x03,0x05,0x04,0x26,0x28,0x25,0x29,0x27,0xff,0x38,0x2a,0x06,0x07,0x08,0x09, 
 0x0b,0x2d,0x2e,0x2b,0x2f,0x2c,0x38,0x43,0x37,0x31,0x39,0x7a,0x78,0x63,0x76,0x60,
 0x61,0x62,0x64,0x65,0x6d,0xff,0xff,0x59,0x5b,0x5c,0x4e,0x56,0x57,0x58,0x45,0x53,
 0x54,0x55,0x52,0x41,0xff,0xff,0xff,0x67,0x6f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,

 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x4c,0x36,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0x4b,0xff,0xff,0x3a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0x47,0xff,0x73,0x3e,0x74,0xff,0x3b,0xff,0x3c,0xff,0x77,
 0x3d,0x79,0x72,0x75,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
};

static bool needUpdateDisplay()
{
	if (gDisplay->isExposed()) {
 		RECT rect;
 		HDC hdc = GetDC(gHWNDMain);
 		int gcb = GetClipBox(hdc, &rect);
 		ReleaseDC(gHWNDMain, hdc);
 		if (gcb != NULLREGION) {
 			return true;
		}
	}
	return false;
}

static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	if (needUpdateDisplay()) gDisplay->displayShow();
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*
 *	This is the thread doing the display
 *	and event handling stuff
 */
static void eventLoop(void *pvoid) 
{
	Win32Display *display = (Win32Display *)pvoid;

	gMenuHeight = display->mMenuHeight;
	WNDCLASS wc;

	memset(&wc,0,sizeof wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.hInstance = gHInst;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = "ClassClass";
	wc.lpszMenuName = 0;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClass(&wc);

	RECT rect;
	rect.top = 0; rect.left = 0;
	rect.bottom = display->mWinChar.height + gMenuHeight;
	rect.right = display->mWinChar.width;
	AdjustWindowRect(&rect, WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
		| WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

	gHWNDMain = CreateWindow("ClassClass", "PearPC",
		WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
		| WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		rect.right-rect.left, rect.bottom-rect.top,
		NULL, NULL, gHInst, NULL);

	display->updateTitle();

	display->createBitmap();

	gDisplay->setExposed(true);
	display->displayShow();
	ShowWindow(gHWNDMain, SW_SHOW);

	SetTimer(gHWNDMain, 0, gDisplay->mRedraw_ms, TimerProc);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	ppc_stop();

	_endthread();
}


void MainWndProc_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id) {
/*	case IDM_EXIT:
		PostMessage(hwnd, WM_CLOSE, 0, 0);
		break;*/
	}
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int shiftDown = false;
	switch (msg) {
	case WM_PAINT: 
	{
		EnterCriticalSection(&gDrawCS);
		PAINTSTRUCT ps;
		
		HDC hdc = BeginPaint(hwnd, &ps);

		SetDIBitsToDevice(hdc, 0, 0, gDisplay->mClientChar.width, gMenuHeight, 0, 0,
			0, gMenuHeight, menuData, &gMenuBitmapInfo, DIB_RGB_COLORS);
		EndPaint(hwnd, &ps); 		
 		LeaveCriticalSection(&gDrawCS);
		damageFrameBufferAll();
		gDisplay->displayShow();						
		break;
	}
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		// This tests if the key is really pressed
		// or if it is only a repeated event
		if (!(lParam & (1<<30))) {
			SystemEvent ev;
			int scancode = HIWORD(lParam) & 0x01FF;
			ev.type = sysevKey;
			ev.key.pressed = true;

			int chr = 0;
			if (scancode < 128) {
				chr = scancode_to_ascii[scancode + shiftDown*128];
			}
			if (scancode == 42 || scancode == 54) shiftDown = 1;
			if (scancode == 0x138) {
				// altgr == ctrl+alt --> release ctrl, press alt
				ev.key.chr = 0;
				ev.key.keycode = scancode_to_mackey[0x1d] | 0x80000000;
				ev.key.pressed = false;
				gKeyboard->handleEvent(ev);
				ev.type = sysevKey;
				ev.key.pressed = true;
				ev.key.chr = 0;
				ev.key.keycode = scancode_to_mackey[0x138];
				gKeyboard->handleEvent(ev);
			} else {
				ev.key.keycode = scancode_to_mackey[scancode];
				if ((ev.key.keycode & 0xff) != 0xff) {
					ev.key.chr = chr;
					gKeyboard->handleEvent(ev);
				}
			}
		}
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP: {
		SystemEvent ev;
		int scancode = HIWORD(lParam) & 0x01FF;
		ev.type = sysevKey;
		ev.key.pressed = false;

		if (scancode == 42 || scancode == 54) shiftDown = 0;

		ev.key.keycode = scancode_to_mackey[scancode] | 0x80000000;

		if ((ev.key.keycode & 0xff) != 0xff) {
			gKeyboard->handleEvent(ev);
		}
		break;
	}
	case WM_CHAR:
  	case WM_DEADCHAR:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
		break;
	case WM_ACTIVATE:
		if (wParam == WA_INACTIVE) {
			if (gDisplay->isMouseGrabbed()) gDisplay->setMouseGrab(false);
		}
		break;
	case WM_LBUTTONUP:
		if (!gDisplay->isMouseGrabbed()) {
/*			if (HIWORD(lParam) < gMenuHeight) {
				gDisplay->clickMenu(LOWORD(lParam), HIWORD(lParam));
			} else {*/
				gDisplay->setMouseGrab(true);
				break;
//			}
		}
		// fall throu
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_MOUSEMOVE: {
		SystemEvent ev;
		gDisplay->mCurMouseX = LOWORD(lParam);
		gDisplay->mCurMouseY = HIWORD(lParam);
		if (!gDisplay->isMouseGrabbed()) break;
		if (msg == WM_MOUSEMOVE) {
			if (gDisplay->mCurMouseX == gDisplay->mHomeMouseX 
			&& gDisplay->mCurMouseY == gDisplay->mHomeMouseY) break;
		}
		ev.type = sysevMouse;
		ev.mouse.button1 = wParam & MK_LBUTTON;
		ev.mouse.button2 = wParam & MK_MBUTTON;
		ev.mouse.button3 = wParam & MK_RBUTTON;
		ev.mouse.relx = gDisplay->mCurMouseX - gDisplay->mHomeMouseX;
		ev.mouse.rely = gDisplay->mCurMouseY - gDisplay->mHomeMouseY;

		gMouse->handleEvent(ev);
		
		if (gDisplay->mFullscreen) {
			SetCursorPos(gDisplay->mHomeMouseX, gDisplay->mHomeMouseY);
		} else {
			RECT wndRect;
			GetWindowRect(hwnd, &wndRect);
			SetCursorPos(wndRect.left + gDisplay->mHomeMouseX + GetSystemMetrics(SM_CXFIXEDFRAME), 
					wndRect.top + gDisplay->mHomeMouseY + GetSystemMetrics(SM_CYFIXEDFRAME)
					+ GetSystemMetrics(SM_CYCAPTION));
		}
		break;
	}
	case WM_SIZE:
		gDisplay->setExposed(wParam != SIZE_MINIMIZED);
		break;
	case WM_COMMAND:
		MainWndProc_OnCommand(hwnd, (int)(LOWORD(wParam)), (HWND)lParam, (UINT)HIWORD(wParam));
		break;     
	case WM_DESTROY:
		gDisplay->setFullscreenMode(false);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd,msg,wParam,lParam);
	}
	return 0;
}

static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);

extern SystemDisplay *allocSystemDisplay(const char *title, const DisplayCharacteristics &chr, int redraw_ms);
extern SystemMouse *allocSystemMouse();
extern SystemKeyboard *allocSystemKeyboard();

void initUI(const char *title, const DisplayCharacteristics &chr, int redraw_ms)
{
	gHInst = GetModuleHandle(NULL);

	gDisplay = allocSystemDisplay(title, chr, redraw_ms);
	gMouse = allocSystemMouse();
	gKeyboard = allocSystemKeyboard();

	_beginthread(eventLoop, 0, gDisplay);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
}

void doneUI()
{
/*	FIXME: races
	delete gDisplay;
	delete gMouse;
	delete gKeyboard;
*/
}
