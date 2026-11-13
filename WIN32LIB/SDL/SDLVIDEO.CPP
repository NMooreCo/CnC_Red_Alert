/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
**	SDLVIDEO.CPP -- SDL2 display and input for the modern port.
**
**	The game renders into 8 bit palettised memory buffers exactly as it did under DirectDraw.
**	The buffer flagged as the visible page is converted through the current palette into a
**	streaming texture and presented, scaled to the window, whenever it has changed. The
**	mouse cursor is composited on top at that point (the original drew it into video memory
**	from a timer interrupt).
**
**	SDL input events are translated into the Win32 messages the game's window procedure
**	expects (see compat/win32.cpp).
**
**	Environment variables:
**		RA_FULLSCREEN=1			start in (desktop) full screen mode; Alt+Enter toggles.
**		RA_SCALE=n					initial window scale factor (default 2).
**		RA_ASPECT=1					stretch to 4:3 like a CRT monitor did.
**		RA_SCREENSHOT=file.bmp	(debugging) write the next frame presented after
**										RA_SCREENSHOT_DELAY ms to a BMP file.
*/

#include "PLATFORM.H"
#include "../../compat/win32_internal.h"

#include <SDL.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static SDL_Window *		Window = NULL;
static SDL_Renderer *	Renderer = NULL;
static SDL_Texture *		Texture = NULL;
static int					LogicalWidth = 640;
static int					LogicalHeight = 400;
static bool					AspectCorrect = false;
static bool					StartFullscreen = false;		// Video_Start_Fullscreen.

static unsigned char const *	Surface = NULL;
static int					SurfaceWidth = 0;
static int					SurfaceHeight = 0;
static int					SurfacePitch = 0;

static Uint32				Palette[256];
static volatile bool		Dirty = true;
static Uint32				LastPresent = 0;

static unsigned char *	CursorImage = NULL;
static int					CursorWidth = 0;
static int					CursorHeight = 0;
static int					CursorHotX = 0;
static int					CursorHotY = 0;
static bool					CursorVisible = false;

static int					MouseX = 0;
static int					MouseY = 0;
static int					MouseWheel = 0;		// Wheel steps not taken yet (up > 0).
static int					LastDrawnMouseX = -1;
static int					LastDrawnMouseY = -1;
static bool					HasFocus = true;
static bool					MouseConfined = false;		// The game asked for it (ClipCursor).
static void					Apply_Mouse_Confinement(void);
static bool					QuitRequested = false;

static char					WindowTitle[128] = "Red Alert";

/*
**	Game controllers (SDLPAD.CPP).
*/
void	Gamepad_Init(void);
bool	Gamepad_Event(SDL_Event const & event);
void	Gamepad_Update(void);


/***************************************************************************************
**	Setup.
*/
/*
**	How tall the picture is shown, in pixels of its width. With RA_ASPECT, each pixel is 1.2
**	times taller, as on a CRT: 640x400 is shown as 4:3. (The 640x480 screen was 4:3 already.)
*/
static int Shown_Height(void)
{
	if (!AspectCorrect) return(LogicalHeight);
	if (LogicalWidth == 640 && LogicalHeight == 480) return(480);
	return((LogicalHeight * 6) / 5);
}

static void Update_Logical_Size(void)
{
	if (Renderer == NULL) return;
	SDL_RenderSetLogicalSize(Renderer, LogicalWidth, Shown_Height());
}

bool Video_Init(int width, int height)
{
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	if (!SDL_WasInit(SDL_INIT_VIDEO)) {
		if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
			fprintf(stderr, "Unable to initialise SDL video: %s\n", SDL_GetError());
			return(false);
		}
	}

	LogicalWidth = width;
	LogicalHeight = height;
	AspectCorrect = (getenv("RA_ASPECT") != NULL && atoi(getenv("RA_ASPECT")) != 0);

	if (Window == NULL) {
		int scale = 2;
		if (getenv("RA_SCALE")) scale = SDL_max(1, atoi(getenv("RA_SCALE")));
		int ww = width * scale;
		int wh = Shown_Height() * scale;

		/*
		**	Don't open a window bigger than the desktop (less the task bar, and room for the
		**	window's title bar). If even the smallest scale is too big (a big screen for
		**	battle), open it maximized: it is shown as big as fits.
		*/
		bool fits = true;
		SDL_Rect usable;
		if (SDL_GetDisplayUsableBounds(0, &usable) == 0) {
			int const titlebar = 40;
			while (scale > 1 && (ww > usable.w || wh + titlebar > usable.h)) {
				scale--;
				ww = width * scale;
				wh = Shown_Height() * scale;
			}
			fits = (ww <= usable.w && wh + titlebar <= usable.h);
		}

		Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
		if (!fits) {
			flags |= SDL_WINDOW_MAXIMIZED;
		}
		bool full = StartFullscreen;
		if (getenv("RA_FULLSCREEN")) full = (atoi(getenv("RA_FULLSCREEN")) != 0);
		if (full) {
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		Window = SDL_CreateWindow(WindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ww, wh, flags);
		if (Window == NULL) {
			fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
			return(false);
		}

		Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if (Renderer == NULL) {
			Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_SOFTWARE);
		}
		if (Renderer == NULL) {
			fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
			return(false);
		}
		SDL_ShowCursor(SDL_DISABLE);

		/*
		**	Windows activates a newly shown window (WM_ACTIVATEAPP), and the game waits for that
		**	before it draws anything. Don't rely on the window manager to report it (there may
		**	not be one); later focus changes are reported as they happen.
		*/
		HasFocus = true;
		Win32_Post_Input(WM_ACTIVATEAPP, TRUE, 0);
		Apply_Mouse_Confinement();

		Gamepad_Init();
	}

	if (Texture) {
		SDL_DestroyTexture(Texture);
		Texture = NULL;
	}
	Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (Texture == NULL) {
		fprintf(stderr, "Unable to create texture: %s\n", SDL_GetError());
		return(false);
	}
	Update_Logical_Size();

	SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
	SDL_RenderClear(Renderer);
	SDL_RenderPresent(Renderer);
	Dirty = true;
	return(true);
}

/*
**	Show only the top left part of the visible surface (width x height), as the whole of the
**	window: the game draws its menus at 640x400 on a surface that is bigger when the battle
**	screen is (higher resolutions). Mouse positions are in the same terms.
*/
void Video_Set_Present_Size(int width, int height)
{
	if (width == LogicalWidth && height == LogicalHeight) return;
	LogicalWidth = width;
	LogicalHeight = height;
	if (Renderer == NULL) return;

	if (Texture) {
		SDL_DestroyTexture(Texture);
		Texture = NULL;
	}
	Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	Update_Logical_Size();
	if (MouseX >= width) MouseX = width - 1;
	if (MouseY >= height) MouseY = height - 1;
	SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
	SDL_RenderClear(Renderer);
	Dirty = true;
}


/*
**	Open the window full screen (before Video_Init; REDALERT.INI's choice). RA_FULLSCREEN
**	overrides it.
*/
void Video_Start_Fullscreen(bool on)
{
	StartFullscreen = on;
}


/*
**	The size of the desktop, for choosing a resolution that fits it.
*/
bool Video_Get_Desktop_Size(int & width, int & height)
{
	if (!SDL_WasInit(SDL_INIT_VIDEO)) {
		if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) return(false);
	}
	SDL_DisplayMode mode;
	if (SDL_GetDesktopDisplayMode(0, &mode) != 0) return(false);
	width = mode.w;
	height = mode.h;
	return(true);
}


void Video_Shutdown(void)
{
	if (Texture) SDL_DestroyTexture(Texture);
	if (Renderer) SDL_DestroyRenderer(Renderer);
	if (Window) SDL_DestroyWindow(Window);
	Texture = NULL;
	Renderer = NULL;
	Window = NULL;
	Surface = NULL;
}

void Video_Register_Visible_Surface(void const * pixels, int width, int height, int pitch)
{
	Surface = (unsigned char const *)pixels;
	SurfaceWidth = width;
	SurfaceHeight = height;
	SurfacePitch = pitch;
	Dirty = true;
}

void Video_Mark_Dirty(void)
{
	Dirty = true;
}

void Video_Set_Title(const char * title)
{
	if (title == NULL) return;
	SDL_strlcpy(WindowTitle, title, sizeof(WindowTitle));
	if (Window) SDL_SetWindowTitle(Window, WindowTitle);
}

void * Video_Get_SDL_Window(void)
{
	return(Window);
}

void Video_Get_Logical_Size(int & w, int & h)
{
	w = LogicalWidth;
	h = LogicalHeight;
}

void Video_Toggle_Fullscreen(void)
{
	if (Window == NULL) return;
	Uint32 flags = SDL_GetWindowFlags(Window);
	if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
		SDL_SetWindowFullscreen(Window, 0);
	} else {
		SDL_SetWindowFullscreen(Window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	Dirty = true;
}

void Video_Set_Relative_Mouse(bool on)
{
	(void)on;
}

/*
**	The game keeps the mouse in its screen while it is active (ClipCursor), as the original
**	did: in a window, this lets the map scroll at its edges. RA_NO_MOUSE_GRAB=1 turns it off.
*/
static void Apply_Mouse_Confinement(void)
{
	static int allowed = -1;
	if (allowed == -1) {
		allowed = !(getenv("RA_NO_MOUSE_GRAB") && atoi(getenv("RA_NO_MOUSE_GRAB")) != 0);
	}
	if (Window) SDL_SetWindowGrab(Window, (allowed && MouseConfined && HasFocus) ? SDL_TRUE : SDL_FALSE);
}

void Video_Confine_Mouse(bool on)
{
	MouseConfined = on;
	Apply_Mouse_Confinement();
}

/*
**	Lets the mouse out for a moment, for a system message box (another window).
*/
bool Video_Release_Mouse(void)
{
	if (Window == NULL || !SDL_GetWindowGrab(Window)) return(false);
	SDL_SetWindowGrab(Window, SDL_FALSE);
	return(true);
}

void Video_Restore_Mouse(void)
{
	Apply_Mouse_Confinement();
}


/***************************************************************************************
**	Palette. The game supplies 256 RGB triplets with 6 bit (0..63) components.
*/
void Video_Set_Palette(void const * palette)
{
	unsigned char const * p = (unsigned char const *)palette;
	for (int i = 0; i < 256; i++) {
		unsigned r = p[i*3+0] & 0x3F;
		unsigned g = p[i*3+1] & 0x3F;
		unsigned b = p[i*3+2] & 0x3F;
		r = (r << 2) | (r >> 4);
		g = (g << 2) | (g >> 4);
		b = (b << 2) | (b >> 4);
		Palette[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
	}
	Dirty = true;
	Video_Present(true);
}


/***************************************************************************************
**	Mouse cursor. "image" is an 8 bit image where colour 0 is transparent.
*/
void Video_Set_Cursor(void const * image, int width, int height, int hotx, int hoty)
{
	if (width <= 0 || height <= 0 || image == NULL) {
		CursorWidth = CursorHeight = 0;
		Dirty = true;
		return;
	}
	if (width * height > CursorWidth * CursorHeight || CursorImage == NULL) {
		free(CursorImage);
		CursorImage = (unsigned char *)malloc(width * height);
	}
	memcpy(CursorImage, image, width * height);
	CursorWidth = width;
	CursorHeight = height;
	CursorHotX = hotx;
	CursorHotY = hoty;
	Dirty = true;
}

void Video_Show_Cursor(bool show)
{
	if (show != CursorVisible) {
		CursorVisible = show;
		Dirty = true;
	}
}

void Video_Get_Mouse(int & x, int & y)
{
	x = MouseX;
	y = MouseY;
}

void Video_Set_Mouse(int x, int y)
{
	MouseX = x;
	MouseY = y;
	if (Window == NULL || Renderer == NULL) return;

	/*
	**	Convert logical coordinates back into window coordinates.
	*/
	SDL_Rect viewport;
	float sx, sy;
	SDL_RenderGetViewport(Renderer, &viewport);
	SDL_RenderGetScale(Renderer, &sx, &sy);
	int ly = (y * Shown_Height()) / LogicalHeight;
	int wx = (int)((x + viewport.x) * sx);
	int wy = (int)((ly + viewport.y) * sy);
	int ow, oh, ww, wh;
	SDL_GetRendererOutputSize(Renderer, &ow, &oh);
	SDL_GetWindowSize(Window, &ww, &wh);
	if (ow > 0 && oh > 0) {
		wx = (wx * ww) / ow;
		wy = (wy * wh) / oh;
	}
	SDL_WarpMouseInWindow(Window, wx, wy);
}


/***************************************************************************************
**	Presentation.
*/
static void Write_Screenshot(Uint32 * pixels, int pitch)
{
	static bool done = false;
	const char * name = getenv("RA_SCREENSHOT");
	if (done || name == NULL) return;
	Uint32 delay = getenv("RA_SCREENSHOT_DELAY") ? (Uint32)atoi(getenv("RA_SCREENSHOT_DELAY")) : 0;
	if (SDL_GetTicks() < delay) return;
	SDL_Surface * shot = SDL_CreateRGBSurfaceWithFormatFrom(pixels, SurfaceWidth, SurfaceHeight, 32, pitch, SDL_PIXELFORMAT_ARGB8888);
	if (shot) {
		SDL_SaveBMP(shot, name);
		SDL_FreeSurface(shot);
	}
	done = true;
}

void Video_Present(bool force)
{
	if (Renderer == NULL || Texture == NULL || Surface == NULL) return;

	bool mouse_moved = CursorVisible && (MouseX != LastDrawnMouseX || MouseY != LastDrawnMouseY);
	if (!force && !Dirty && !mouse_moved) return;

	/*
	**	Don't present faster than the display can show (vsync would block us anyway).
	*/
	Uint32 now = SDL_GetTicks();
	if (!force && (now - LastPresent) < 8) return;
	LastPresent = now;
	Dirty = false;

	void * texels;
	int pitch;
	if (SDL_LockTexture(Texture, NULL, &texels, &pitch) != 0) return;

	int w = SDL_min(SurfaceWidth, LogicalWidth);
	int h = SDL_min(SurfaceHeight, LogicalHeight);
	for (int y = 0; y < h; y++) {
		unsigned char const * src = Surface + y * SurfacePitch;
		Uint32 * dst = (Uint32 *)((unsigned char *)texels + y * pitch);
		for (int x = 0; x < w; x++) {
			dst[x] = Palette[src[x]];
		}
	}

	/*
	**	Composite the mouse cursor.
	*/
	if (CursorVisible && CursorImage && CursorWidth > 0) {
		int cx = MouseX - CursorHotX;
		int cy = MouseY - CursorHotY;
		for (int y = 0; y < CursorHeight; y++) {
			int py = cy + y;
			if (py < 0 || py >= h) continue;
			Uint32 * dst = (Uint32 *)((unsigned char *)texels + py * pitch);
			unsigned char const * src = CursorImage + y * CursorWidth;
			for (int x = 0; x < CursorWidth; x++) {
				int px = cx + x;
				if (px < 0 || px >= w) continue;
				if (src[x]) dst[px] = Palette[src[x]];
			}
		}
	}
	LastDrawnMouseX = MouseX;
	LastDrawnMouseY = MouseY;

	Write_Screenshot((Uint32 *)texels, pitch);

	SDL_UnlockTexture(Texture);
	SDL_RenderClear(Renderer);
	SDL_RenderCopy(Renderer, Texture, NULL, NULL);
	SDL_RenderPresent(Renderer);
}


/***************************************************************************************
**	Input. Translate SDL key codes into Win32 virtual keys.
*/
static int SDL_To_VK(SDL_Keysym const & key)
{
	SDL_Keycode sym = key.sym;
	SDL_Scancode sc = key.scancode;

	if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z) return('A' + (sc - SDL_SCANCODE_A));
	if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9) return('1' + (sc - SDL_SCANCODE_1));
	if (sc == SDL_SCANCODE_0) return('0');
	if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12) return(VK_F1 + (sc - SDL_SCANCODE_F1));

	bool numlock = (key.mod & KMOD_NUM) != 0;
	switch (sc) {
		case SDL_SCANCODE_KP_0:			return(numlock ? VK_NUMPAD0 : VK_INSERT);
		case SDL_SCANCODE_KP_1:			return(numlock ? VK_NUMPAD1 : VK_END);
		case SDL_SCANCODE_KP_2:			return(numlock ? VK_NUMPAD2 : VK_DOWN);
		case SDL_SCANCODE_KP_3:			return(numlock ? VK_NUMPAD3 : VK_NEXT);
		case SDL_SCANCODE_KP_4:			return(numlock ? VK_NUMPAD4 : VK_LEFT);
		case SDL_SCANCODE_KP_5:			return(numlock ? VK_NUMPAD5 : VK_CLEAR);
		case SDL_SCANCODE_KP_6:			return(numlock ? VK_NUMPAD6 : VK_RIGHT);
		case SDL_SCANCODE_KP_7:			return(numlock ? VK_NUMPAD7 : VK_HOME);
		case SDL_SCANCODE_KP_8:			return(numlock ? VK_NUMPAD8 : VK_UP);
		case SDL_SCANCODE_KP_9:			return(numlock ? VK_NUMPAD9 : VK_PRIOR);
		case SDL_SCANCODE_KP_PERIOD:	return(numlock ? VK_DECIMAL : VK_DELETE);
		case SDL_SCANCODE_KP_DIVIDE:	return(VK_DIVIDE);
		case SDL_SCANCODE_KP_MULTIPLY:	return(VK_MULTIPLY);
		case SDL_SCANCODE_KP_MINUS:		return(VK_SUBTRACT);
		case SDL_SCANCODE_KP_PLUS:		return(VK_ADD);
		case SDL_SCANCODE_KP_ENTER:		return(VK_RETURN);
		default: break;
	}

	switch (sym) {
		case SDLK_RETURN:			return(VK_RETURN);
		case SDLK_ESCAPE:			return(VK_ESCAPE);
		case SDLK_BACKSPACE:		return(VK_BACK);
		case SDLK_TAB:				return(VK_TAB);
		case SDLK_SPACE:			return(VK_SPACE);
		case SDLK_CAPSLOCK:		return(VK_CAPITAL);
		case SDLK_PRINTSCREEN:	return(VK_PRINT);
		case SDLK_SCROLLLOCK:	return(VK_SCROLL);
		case SDLK_PAUSE:			return(VK_PAUSE);
		case SDLK_INSERT:			return(VK_INSERT);
		case SDLK_HOME:			return(VK_HOME);
		case SDLK_PAGEUP:			return(VK_PRIOR);
		case SDLK_DELETE:			return(VK_DELETE);
		case SDLK_END:				return(VK_END);
		case SDLK_PAGEDOWN:		return(VK_NEXT);
		case SDLK_RIGHT:			return(VK_RIGHT);
		case SDLK_LEFT:			return(VK_LEFT);
		case SDLK_DOWN:			return(VK_DOWN);
		case SDLK_UP:				return(VK_UP);
		case SDLK_NUMLOCKCLEAR:	return(VK_NUMLOCK);
		case SDLK_LCTRL:
		case SDLK_RCTRL:			return(VK_CONTROL);
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:			return(VK_SHIFT);
		case SDLK_LALT:
		case SDLK_RALT:			return(VK_MENU);
		case SDLK_LGUI:			return(VK_LWIN);
		case SDLK_RGUI:			return(VK_RWIN);
		case SDLK_APPLICATION:	return(VK_APPS);
		case SDLK_SEMICOLON:		return(VK_OEM_1);
		case SDLK_EQUALS:			return(VK_OEM_PLUS);
		case SDLK_COMMA:			return(VK_OEM_COMMA);
		case SDLK_MINUS:			return(VK_OEM_MINUS);
		case SDLK_PERIOD:			return(VK_OEM_PERIOD);
		case SDLK_SLASH:			return(VK_OEM_2);
		case SDLK_BACKQUOTE:		return(VK_OEM_3);
		case SDLK_LEFTBRACKET:	return(VK_OEM_4);
		case SDLK_BACKSLASH:		return(VK_OEM_5);
		case SDLK_RIGHTBRACKET:	return(VK_OEM_6);
		case SDLK_QUOTE:			return(VK_OEM_7);
		default: break;
	}
	return(0);
}

static LPARAM Mouse_LParam(void)
{
	return(MAKELPARAM(MouseX, MouseY));
}

static void Update_Mouse_Position(int x, int y)
{
	/*
	**	With a logical renderer size, SDL already reports coordinates in logical pixels.
	*/
	if (AspectCorrect) {
		y = (y * LogicalHeight) / Shown_Height();
	}
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= LogicalWidth) x = LogicalWidth - 1;
	if (y >= LogicalHeight) y = LogicalHeight - 1;
	MouseX = x;
	MouseY = y;
}

void Platform_Pump_Events(void)
{
	SDL_Event event;

	if (!SDL_WasInit(SDL_INIT_EVENTS)) return;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				/*
				**	The window was closed (or the process asked to terminate). The original
				**	ignored close requests; quit through its forced shutdown path (WM_DESTROY
				**	outside of a normal exit), which saves the settings and exits.
				*/
				if (!QuitRequested) {
					QuitRequested = true;
					Win32_Post_Input(WM_DESTROY, 0, 0);
				}
				break;

			case SDL_WINDOWEVENT:
				switch (event.window.event) {
					case SDL_WINDOWEVENT_FOCUS_GAINED:
						if (!HasFocus) {
							HasFocus = true;
							Apply_Mouse_Confinement();
							Win32_Post_Input(WM_ACTIVATEAPP, TRUE, 0);
						}
						break;
					case SDL_WINDOWEVENT_FOCUS_LOST:
						if (HasFocus) {
							HasFocus = false;
							Apply_Mouse_Confinement();
							Win32_Clear_Key_States();
							Win32_Post_Input(WM_ACTIVATEAPP, FALSE, 0);
						}
						break;
					case SDL_WINDOWEVENT_EXPOSED:
					case SDL_WINDOWEVENT_RESIZED:
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						Dirty = true;
						break;
					default:
						break;
				}
				break;

			case SDL_KEYDOWN:
			case SDL_KEYUP: {
				bool down = (event.type == SDL_KEYDOWN);
				/*
				**	Alt+Enter toggles full screen; it is not passed to the game.
				*/
				if (down && event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT)) {
					Video_Toggle_Fullscreen();
					break;
				}
				int vk = SDL_To_VK(event.key.keysym);
				if (vk == 0) break;
				Win32_Set_Toggle_State(VK_CAPITAL, (event.key.keysym.mod & KMOD_CAPS) != 0);
				Win32_Set_Toggle_State(VK_NUMLOCK, (event.key.keysym.mod & KMOD_NUM) != 0);
				if (vk == VK_SHIFT) Win32_Set_Key_State(event.key.keysym.sym == SDLK_LSHIFT ? VK_LSHIFT : VK_RSHIFT, down);
				if (vk == VK_CONTROL) Win32_Set_Key_State(event.key.keysym.sym == SDLK_LCTRL ? VK_LCONTROL : VK_RCONTROL, down);
				if (vk == VK_MENU) Win32_Set_Key_State(event.key.keysym.sym == SDLK_LALT ? VK_LMENU : VK_RMENU, down);

				bool alt = (event.key.keysym.mod & KMOD_ALT) != 0;
				LPARAM lparam = 1 | ((LPARAM)event.key.keysym.scancode << 16);
				if (event.key.repeat) lparam |= (1 << 30);
				if (!down) lparam |= (1 << 30) | (1U << 31);
				UINT message;
				if (alt || vk == VK_MENU || vk == VK_F10) {
					message = down ? WM_SYSKEYDOWN : WM_SYSKEYUP;
				} else {
					message = down ? WM_KEYDOWN : WM_KEYUP;
				}
				Win32_Post_Input(message, (WPARAM)vk, lparam);
				break;
			}

			case SDL_MOUSEMOTION:
				Update_Mouse_Position(event.motion.x, event.motion.y);
				Win32_Post_Input(WM_MOUSEMOVE, 0, Mouse_LParam());
				break;

			case SDL_MOUSEWHEEL:
				MouseWheel += event.wheel.y;		// The original had no wheel: Video_Take_Mouse_Wheel.
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP: {
				Update_Mouse_Position(event.button.x, event.button.y);
				bool down = (event.type == SDL_MOUSEBUTTONDOWN);
				UINT message = 0;
				switch (event.button.button) {
					case SDL_BUTTON_LEFT:	message = down ? WM_LBUTTONDOWN : WM_LBUTTONUP; break;
					case SDL_BUTTON_RIGHT:	message = down ? WM_RBUTTONDOWN : WM_RBUTTONUP; break;
					case SDL_BUTTON_MIDDLE:	message = down ? WM_MBUTTONDOWN : WM_MBUTTONUP; break;
					default: break;
				}
				if (message) Win32_Post_Input(message, 0, Mouse_LParam());
				break;
			}

			default:
				Gamepad_Event(event);
				break;
		}
	}
	Gamepad_Update();

	Video_Present();
#ifdef __EMSCRIPTEN__
	Win32_Web_Poll();		// In a browser: the page's turn (compat/win32.cpp).
#endif
}


/*
**	The pointer and buttons, worked by a game controller (SDLPAD.CPP): as the mouse would.
*/
void Video_Pad_Move_Mouse(int dx, int dy)
{
	int x = MouseX + dx;
	int y = MouseY + dy;
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= LogicalWidth) x = LogicalWidth - 1;
	if (y >= LogicalHeight) y = LogicalHeight - 1;
	if (x == MouseX && y == MouseY) return;
	MouseX = x;
	MouseY = y;
	Win32_Post_Input(WM_MOUSEMOVE, 0, Mouse_LParam());
}

void Video_Pad_Button(int button, bool down)
{
	UINT message = (button == 0) ? (down ? WM_LBUTTONDOWN : WM_LBUTTONUP) : (down ? WM_RBUTTONDOWN : WM_RBUTTONUP);
	Win32_Post_Input(message, 0, Mouse_LParam());
}

void Video_Pad_Wheel(int steps)
{
	MouseWheel += steps;
}

int Video_Take_Mouse_Wheel(void)
{
	int steps = MouseWheel;
	MouseWheel = 0;
	return(steps);
}

bool Platform_Has_Focus(void)
{
	return(HasFocus);
}

bool Platform_Quit_Requested(void)
{
	return(QuitRequested);
}

void Platform_Yield(void)
{
	SDL_Delay(0);
}

char const * Platform_Get_Base_Path(void)
{
	static char * base = NULL;
	if (base == NULL) base = SDL_GetBasePath();
	return(base ? base : "");
}

unsigned int Platform_Get_Ticks(void)
{
	return(SDL_GetTicks());
}

void Platform_Sleep(unsigned int milliseconds)
{
	SDL_Delay(milliseconds);
#ifdef __EMSCRIPTEN__
	Win32_Web_Poll();
#endif
}
