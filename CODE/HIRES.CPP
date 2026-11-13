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
**	HIRES.CPP -- higher resolutions for the battle screen (RA_PORT).
**
**	The original drew everything at 640x400 (its 640x480 option only added black bars above
**	and below). A bigger battle screen can be chosen: REDALERT.INI, [Video] Resolution=1280x720
**	(or "auto", for a size that fills the desktop at a whole scale, full screen);
**	RA_RESOLUTION overrides it. [Video] Fullscreen=yes opens the game full screen.
**
**	The visible and hidden pages are made that big. In battle, the sidebar, the radar and the
**	power bar go along the right edge and the tactical map takes up the rest, so more of the
**	battlefield is in view; everything else (the menus, movies, score screens, the map
**	selection) keeps to 640x400 in the top left corner of the pages, which is then all that is
**	shown, filling the window as before (Video_Set_Present_Size).
*/

#include	"FUNCTION.H"
#include	"PLATFORM.H"

int ScreenDX = 0;					// How far right the sidebar, radar and power bar are moved.

#ifdef RA_PORT

int HiresWidth = 640;			// The battle screen's size.
int HiresHeight = 400;
static bool IsGameScreen = false;


bool Hires_Is_On(void)
{
	return(HiresWidth != 640 || HiresHeight != 400);
}


/***********************************************************************************************
 * Hires_Choose -- Reads the battle screen's size (at start up, before the video mode is set). *
 *=============================================================================================*/
void Hires_Choose(INIClass const & ini)
{
	char text[32];
	char const * env = getenv("RA_RESOLUTION");
	if (env != NULL && *env != '\0') {
		strncpy(text, env, sizeof(text)-1);
		text[sizeof(text)-1] = '\0';
	} else {
#ifdef __ANDROID__
		ini.Get_String("Video", "Resolution", "auto", text, sizeof(text));		// A phone's whole (wide) screen.
#else
		ini.Get_String("Video", "Resolution", "640x400", text, sizeof(text));
#endif
	}

	int width = 640;
	int height = 400;
	bool full = false;
	if (stricmp(text, "auto") == 0) {
		full = true;

		/*
		**	The desktop, at the biggest whole scale that still has 400 lines or more.
		*/
		int dw, dh;
		if (Video_Get_Desktop_Size(dw, dh) && dh >= 400) {
			int scale = max(1, dh / 400);
			width = dw / scale;
			height = dh / scale;
		}
	} else if (sscanf(text, "%dx%d", &width, &height) != 2) {
		width = 640;
		height = 400;
	}

	HiresWidth = Bound(width, 640, 4096) & ~7;
	HiresHeight = Bound(height, 400, 4096) & ~1;

	Video_Start_Fullscreen(ini.Get_Bool("Video", "Fullscreen", full));
}


/***********************************************************************************************
 * Screen_Game_Mode -- Switches between the battle screen and the 640x400 screen.              *
 *                                                                                             *
 *    On: the pages' whole size, for battle. Off: the 640x400 top left corner, for the menus,  *
 *    movies, score screens and the scenario editor. The map is laid out for the screen        *
 *    either way (SidebarClass::Screen_Resized).                                               *
 *=============================================================================================*/
void Screen_Game_Mode(bool on)
{
	if (!Hires_Is_On() || on == IsGameScreen) return;
	IsGameScreen = on;

	int width = on ? HiresWidth : 640;
	int height = on ? HiresHeight : 400;

	VisiblePage.Clear();
	HiddenPage.Clear();
	SeenBuff.Attach(&VisiblePage, 0, 0, width, height);
	HidPage.Attach(&HiddenPage, 0, 0, width, height);
	ScreenDX = width - 640;
	Video_Set_Present_Size(width, height);

	/*
	**	The main window (what shapes are clipped to) is the whole screen.
	*/
	WindowList[WINDOW_MAIN][WINDOWWIDTH] = width;
	WindowList[WINDOW_MAIN][WINDOWHEIGHT] = height;
	if (Window == WINDOW_MAIN) {
		Change_Window(WINDOW_MAIN);
	}

	/*
	**	The options menu centres itself on the screen.
	*/
	Options.Adjust_Variables_For_Resolution();

	/*
	**	A taller screen shows more slots in the sidebar's strips, and their art stretches to
	**	match (SidebarClass::Draw_It, PowerClass::Draw_It).
	*/
	int slots = SidebarClass::StripClass::MAX_VISIBLE;
	if (on) {
		slots += (height - 400) / (SidebarClass::StripClass::OBJECT_HEIGHT * RESFACTOR);
	}
	SidebarClass::StripClass::Visible = min(slots, (int)SidebarClass::StripClass::MAX_SLOTS);

	Map.Screen_Resized();
}


/***********************************************************************************************
 * Draw_Shape_Rows -- Draws some of a shape's rows: a band of it.                              *
 *                                                                                             *
 *    Rows "top" to "top+rows-1" of the shape go at x,y on the screen, and nothing else of it. *
 *    For stretching the art of the sidebar and the power bar, made for a 400 line screen.     *
 *=============================================================================================*/
void Draw_Shape_Rows(void const * shape, int frame, int x, int y, int top, int rows, ShapeFlags_Type flags, void const * remap)
{
	int saved[4];
	for (int index = 0; index < 4; index++) {
		saved[index] = WindowList[WINDOW_PARTIAL][index];
	}
	WindowList[WINDOW_PARTIAL][WINDOWX] = x;
	WindowList[WINDOW_PARTIAL][WINDOWY] = y;
	WindowList[WINDOW_PARTIAL][WINDOWWIDTH] = max(0, LogicPage->Get_Width() - x);
	WindowList[WINDOW_PARTIAL][WINDOWHEIGHT] = rows;
	CC_Draw_Shape(shape, frame, 0, -top, WINDOW_PARTIAL, flags | SHAPE_WIN_REL, remap);
	for (int index = 0; index < 4; index++) {
		WindowList[WINDOW_PARTIAL][index] = saved[index];
	}
}

#endif
