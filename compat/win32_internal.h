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
**	win32_internal.h -- glue between the Win32 emulation (compat/) and the SDL2 platform
**	layer (WIN32LIB/SDL). Not for use by game code.
*/
#ifndef RA_WIN32_INTERNAL_H
#define RA_WIN32_INTERNAL_H

#include "windows.h"
#include <string>

/*
**	Implemented in compat/win32.cpp
*/
void		Win32_Post_Input(UINT message, WPARAM wparam, LPARAM lparam);
void		Win32_Set_Key_State(int vkey, bool down);
void		Win32_Set_Toggle_State(int vkey, bool on);
void		Win32_Clear_Key_States(void);
LRESULT	Win32_Call_Window_Procedure(UINT message, WPARAM wparam, LPARAM lparam);
std::string Win32_Resolve_Path(const char * name, bool for_create);
bool		Win32_Wildcard_Match(const char * pattern, const char * name);

/*
**	Implemented in compat/winsock.cpp -- delivers WSAAsyncSelect() notifications.
*/
void		Winsock_Poll(void);

/*
**	Implemented in WIN32LIB/SDL/SDLVIDEO.CPP
*/
void		Platform_Pump_Events(void);
void		Video_Get_Mouse(int & x, int & y);
void		Video_Set_Mouse(int x, int y);
void		Video_Get_Logical_Size(int & w, int & h);
void		Video_Set_Title(const char * title);
void *	Video_Get_SDL_Window(void);
void		Video_Set_Relative_Mouse(bool on);

#endif
