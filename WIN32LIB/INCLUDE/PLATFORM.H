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
**	PLATFORM.H -- interface to the SDL2 platform layer (WIN32LIB/SDL).
**
**	The original game talked to Windows 95 through DirectDraw, DirectSound, the Win32
**	message queue and multimedia timers. The modern port routes all of that through the
**	small set of functions declared here. Nothing in this header depends on SDL itself, so
**	game code can call it freely.
*/
#ifndef RA_PLATFORM_H
#define RA_PLATFORM_H

/*
**	Video. The game draws into 8 bit memory buffers; the buffer registered as the visible
**	page is converted through the current palette and presented to the window.
*/
bool	Video_Init(int width, int height);
void	Video_Set_Present_Size(int width, int height);		// Show this top left part of the surface.
bool	Video_Get_Desktop_Size(int & width, int & height);
void	Video_Start_Fullscreen(bool on);
void	Video_Shutdown(void);
void	Video_Register_Visible_Surface(void const * pixels, int width, int height, int pitch);
void	Video_Mark_Dirty(void);
void	Video_Set_Palette(void const * palette);			// 256 * RGB, 6 bit (VGA) components
void	Video_Present(bool force = false);
void	Video_Toggle_Fullscreen(void);

/*
**	Software mouse cursor, composited on top of the visible page at present time.
*/
void	Video_Set_Cursor(void const * image, int width, int height, int hotx, int hoty);
void	Video_Show_Cursor(bool show);
void	Video_Get_Mouse(int & x, int & y);
void	Video_Set_Mouse(int x, int y);
int	Video_Take_Mouse_Wheel(void);		// Wheel steps since the last call (up > 0).

/*
**	Events. Pumps the SDL event queue, feeding keyboard/mouse input into the game's
**	keyboard class and refreshing the screen if needed.
*/
void	Platform_Pump_Events(void);
bool	Platform_Has_Focus(void);
bool	Platform_Quit_Requested(void);
void	Platform_Yield(void);

/*
**	Time. Milliseconds since start up (monotonic).
*/
unsigned int	Platform_Get_Ticks(void);
char const *	Platform_Get_Base_Path(void);			// Directory of the executable (with trailing separator).
void				Platform_Sleep(unsigned int milliseconds);

#endif
