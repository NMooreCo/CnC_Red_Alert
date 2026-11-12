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

/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Drawbuff - Westwood win95 library                            *
 *                                                                                             *
 *                    File Name : Iconcach.CPP                                                 *
 *                                                                                             *
 *                   Programmer : Steve Tall                                                   *
 *                                                                                             *
 *                   Start Date : November 8th, 1995                                           *
 *                                                                                             *
 *                  Last Update : November 13th, 1995 [ST]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview: This file cantains members of the IconCacheClass and associated non member        *
 *           functions. All functions are to do with caching individual icons from icon sets   *
 *           into video memory to improve the speed of subsequent drawing                      *
 *                                                                                             *
 * Functions:                                                                                  *
 *  Cache_New_Icon -- Call the Cache_It member to cache a registered icon to video memory      *
 *  Invalidate_Cached_Icons -- Uncache all the icons                                           *
 *  Restore_Cached_Icons -- restore cached icons after a focus loss                            *
 *  Register_Icon_Set -- register an icon set as cachable                                      *
 *  Get_Free_Cache_Slot -- find an empty cache slot                                            *
 *  IconCacheClass::IconCacheClass -- IconCacheClass constructor                               *
 *  IconCacheClass::~IconCacheClass -- IconCacheClass destructor                               *
 *  IconCacheClass::Restore -- restore the icons surface and recache it                        *
 *  IconCacheClass::Cache_It -- cache an icon into video memory                                *
 *  IconCacheClass::Uncache_It -- restore the video memory used by a cached icon               *
 *  IconCacheClass::Draw_It -- use the blitter to draw the cached icon                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define	WIN32_LEAN_AND_MEAN

#include <windows.h>
#include "MISC.H"
#include "ICONCACH.H"
#include "GBUFFER.H"

/*
**	Modern port: the DirectDraw video memory icon cache has been removed (see ICONCACH.H).
**	Icons are always drawn in software by Buffer_Draw_Stamp_Clip.
*/
BOOL	IconCacheAllowed		= FALSE;
int	CachedIconsDrawn		= 0;
int	UnCachedIconsDrawn	= 0;

void Invalidate_Cached_Icons (void)
{
}

void Restore_Cached_Icons (void)
{
}

void Register_Icon_Set (void * , BOOL )
{
}
