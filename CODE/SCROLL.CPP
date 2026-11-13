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

/* $Header: /CounterStrike/SCROLL.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : SCROLL.CPP                                                   *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 01/08/95                                                     *
 *                                                                                             *
 *                  Last Update : August 25, 1995 [JLB]                                        *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   ScrollClass::AI -- Handles scroll AI processing.                                          *
 *   ScrollClass::ScrollClass -- Constructor for the scroll class object.                      *
 *   ScrollClass::Set_Autoscroll -- Turns autoscrolling on or off.                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include	"FUNCTION.H"
#ifdef RA_PORT
#include	"PLATFORM.H"
#endif

#ifdef WIN32
#define	SCROLL_DELAY	1
#else
#define	SCROLL_DELAY	2
#endif

CDTimerClass<SystemTimerClass> ScrollClass::Counter RA_CONSTRUCT_EARLY;		// RA_PORT: before Map, whose constructor sets it.

#ifdef RA_PORT
/*
**	RA_PORT: the original moved the map one step each time ScrollClass::AI ran, and that is
**	each time the game loop goes round while it waits for the next game frame: about as often
**	as the computers of the day could redraw the map. Today the loop goes round hundreds of
**	times a second, and the map flew past. This paces the steps to the system tick (60 a
**	second, the clock that also builds up the scrolling speed): it returns the number of steps
**	due, usually 0 or 1, and more when the game was too busy to scroll for a tick or two.
*/
static int Scroll_Steps(void)
{
	static unsigned last = 0;
	SystemTimerClass timer;

	unsigned now = (unsigned)timer();
	unsigned elapsed = now - last;
	if (elapsed == 0) return(0);
	last = now;

	/*
	**	A longer gap means that scrolling has just begun (or that the game stalled): one step.
	*/
	return((elapsed > 4) ? 1 : (int)elapsed);
}
#endif


/***********************************************************************************************
 * ScrollClass::ScrollClass -- Constructor for the scroll class object.                        *
 *                                                                                             *
 *    This is the constructor for the scroll class object.                                     *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/10/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
ScrollClass::ScrollClass(void) :
	IsAutoScroll(true)
{
	Counter = SCROLL_DELAY;
	Inertia = 0;
}


/***********************************************************************************************
 * ScrollClass::AI -- Handles scroll AI processing.                                            *
 *                                                                                             *
 *    This routine is called every game frame for purposes of input processing.                *
 *                                                                                             *
 * INPUT:   input    -- Reference to the keyboard/mouse event that just occurred.              *
 *                                                                                             *
 *          x,y      -- The mouse coordinates.                                                 *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/10/1995 JLB : Created.                                                                 *
 *   08/10/1995 JLB : Revamped for free smooth scrolling.                                      *
 *   08/25/1995 JLB : Handles new scrolling option.                                            *
 *=============================================================================================*/
#define	EVA_WIDTH		80
void ScrollClass::AI(KeyNumType &input, int x, int y)
{
	bool		player_scrolled=false;
	static 	DirType	direction;
	int		rate;


	/*
	**	If rubber band mode is in progress, then don't allow scrolling of the tactical map.
	*/
	if (!IsRubberBand /*&& !IsTentative*/) {

		/*
		**	Special check to not scroll within the special no-scroll regions.
		*/
		bool noscroll = false;

		if (!noscroll) {
			bool at_screen_edge = (y == 0 || x == 0 || x >= SeenBuff.Get_Width()-1 || y >= SeenBuff.Get_Height()-1);

#ifdef RA_PORT
			/*
			**	RA_PORT: a game controller's right stick scrolls the map, the faster the further
			**	it is pushed (SDLPAD.CPP), at the scroll rate of the options.
			*/
			int padx, pady;
			if (!at_screen_edge && Video_Get_Pad_Scroll(padx, pady)) {
				int steps = Scroll_Steps();
				if (steps) {
					static int const _padrate[7] = {0x00A0, 0x0080, 0x0060, 0x0040, 0x0030, 0x0020, 0x0010};
					int push = max(abs(padx), abs(pady));
					int scrollrate = Options.ScrollRate;
					scrollrate = (scrollrate < 0) ? 0 : ((scrollrate > 6) ? 6 : scrollrate);
					int distance = (_padrate[scrollrate] * RESFACTOR * push * push) / (256 * 256);
					DirType dir = (DirType)Desired_Facing256(0, 0, padx, pady);
					int amount = distance * steps;
					if (amount > 0) Scroll_Map(dir, amount, true);
				}
			}
#endif

			/*
			**	Verify that the mouse is over a scroll region.
			*/
			if (Inertia || at_screen_edge) {
				if (at_screen_edge) {

					player_scrolled=true;

					/*
					**	Adjust the mouse coordinates to emphasize the
					**	cardinal directions over the diagonals.
					*/
#ifdef RA_PORT
					/*
					**	RA_PORT: in terms of the screen's size (HIRES.CPP); the same at 640x400.
					*/
					int sw = SeenBuff.Get_Width();
					int sh = SeenBuff.Get_Height();
					int margin = 50 * RESFACTOR;
					int altx = x;
					if (altx < margin) altx -= (margin-altx);
					altx = max(altx, 0);
					if (altx > (sw-margin)) altx += altx-(sw-margin);
					altx = min(altx, sw);
					if (altx > margin && altx < (sw-margin)) {
						altx += ((sw/2)-altx)/2;
					}

					int alty = y;
					if (alty < margin) alty -= margin-alty;
					alty = max(alty, 0);
					if (alty > (sh-margin)) alty += alty-(sh-margin);
					alty = min(alty, sh);

					direction = (DirType)Desired_Facing256(sw/2, sh/2, altx, alty);
#else
					int altx = x;
					if (altx < 50 * RESFACTOR) altx -= ((50 * RESFACTOR)-altx);
					altx = max(altx, 0);
					if (altx > ((320-50) * RESFACTOR)) altx += altx-((320-50) * RESFACTOR);
					altx = min(altx, (320 * RESFACTOR));
					if (altx > (50 * RESFACTOR) && altx < ((320-50) * RESFACTOR)) {
						altx += (((320/2) * RESFACTOR)-altx)/2;
					}

					int alty = y;
					if (alty < (50 * RESFACTOR)) alty -= (50 * RESFACTOR)-alty;
					alty = max(alty, 0);
					if (alty > (150 * RESFACTOR)) alty += alty-(150 * RESFACTOR);
					alty = min(alty, 200 * RESFACTOR);

					direction = (DirType)Desired_Facing256((320/2) * RESFACTOR, (200/2) * RESFACTOR, altx, alty);
#endif
				}

				int control = Dir_Facing(direction);

				/*
				**	The mouse is over a scroll region so set the mouse shape accordingly if the map
				**	can be scrolled in the direction indicated.
				*/
				static int _rate[9] = {
					0x00E0*RESFACTOR,
					0x00C0*RESFACTOR,
					0x00A0*RESFACTOR,
					0x0080*RESFACTOR,
					0x0060*RESFACTOR,
					0x0040*RESFACTOR,
					0x0020*RESFACTOR,
					0x0010*RESFACTOR,
					0x0008*RESFACTOR
				};
				if (Debug_Map) {
					rate = Options.ScrollRate+1;
				} else {
					rate = 8-Inertia;
				}

				if (rate < Options.ScrollRate+1) {
					rate = Options.ScrollRate+1;
					Inertia = 8-rate;
				}

				/*
				**	Increase the scroll rate if the mouse button is held down.
				*/
	//			if (Keyboard->Down(KN_LMOUSE)) {
	//				rate = Bound(rate-3, 0, 4);
	//			}
				if (Keyboard->Down(KN_RMOUSE)) {
					rate = Bound(rate+1, 4, (int)(sizeof(_rate)/sizeof(_rate[0]))-1);
				}

				/*
				**	If options indicate that scrolling should be forced to
				**	one of the 8 facings, then adjust the direction value
				**	accordingly.
				*/
				direction = Facing_Dir(Dir_Facing(direction));

				int distance = _rate[rate]/2;

				if (!Scroll_Map(direction, distance, false)) {
					Override_Mouse_Shape((MouseType)(MOUSE_NO_N+control), false);
				} else {
					Override_Mouse_Shape((MouseType)(MOUSE_N+control), false);

					/*
					**	If the mouse button is pressed or auto scrolling is active, then scroll
					**	the map if the delay counter indicates.
					*/
					if (Keyboard->Down(KN_LMOUSE) || IsAutoScroll) {
						distance = _rate[rate];
#ifdef RA_PORT
						int steps = Scroll_Steps();		// RA_PORT: at a steady pace, as above.
						distance *= steps;
#else
						int const steps = 1;
#endif

						if (Debug_Map) {
							if (steps) Scroll_Map(direction, distance, true);
							Counter = SCROLL_DELAY;
						} else {
							if (steps) Scroll_Map(direction, distance, true);

							if (Counter == 0 && player_scrolled) {
								Counter = SCROLL_DELAY;
								Inertia++;
							}
						}
					}
				}

			}

			if (!Debug_Map && !player_scrolled) {
				if (!Counter) {
					Inertia--;
					if (Inertia<0) Inertia++;
					Counter = SCROLL_DELAY;
				}
			}

		}
	}

	HelpClass::AI(input, x, y);
}


/***********************************************************************************************
 * ScrollClass::Set_Autoscroll -- Turns autoscrolling on or off.                               *
 *                                                                                             *
 *    This routine controls the autoscrolling setting. Autoscroll, when active, will cause the *
 *    map to scroll if the mouse is held over the scroll region. This is regardless of whether *
 *    any mouse button is held down or not.                                                    *
 *                                                                                             *
 * INPUT:   control  -- Should the autoscroll be turned on?                                    *
 *                      0  = turn off                                                          *
 *                      1  = turn on                                                           *
 *                      -1 = toggle current setting                                            *
 *                                                                                             *
 * OUTPUT:  Returns with the old setting of the autoscroll flag.                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/10/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
bool ScrollClass::Set_Autoscroll(int control)
{
	bool old = IsAutoScroll;

	switch (control) {
		case -1:
			IsAutoScroll = !IsAutoScroll;
			break;

		default:
			IsAutoScroll = control;
			break;
	}
	return(old);
}


