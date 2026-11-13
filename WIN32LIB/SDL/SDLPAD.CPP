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
**	SDLPAD.CPP -- game controllers (gamepads) for the SDL2 platform layer.
**
**	A controller works the mouse and a few keys, so it plays everything the mouse does:
**
**		left stick           moves the pointer (the further, the faster)
**		right stick          scrolls the map (Video_Get_Pad_Scroll)
**		A                    the left mouse button: select, give orders, click buttons;
**		                     hold it and move the pointer to drag a box around units
**		B                    the right mouse button
**		X                    Stop (S)
**		Y                    Guard (G)
**		LB, RB               the mouse wheel, up and down: the sidebar column under the
**		                     pointer, lists
**		LT (held)            Ctrl: force fire
**		RT (held)            Shift: add to the selection
**		left stick (press)   Alt (held): force move
**		right stick (press)  the next unit (N)
**		d-pad                the arrow keys
**		Back                 the base (Home)
**		Start                Options (Esc)
**
**	RA_GAMEPAD=0 turns controllers off. RA_TEST_GAMEPAD=1 adds a virtual controller for
**	automated tests, driven through GamepadTestJoystick (SDL_JoystickSetVirtualAxis/Button).
*/

#include "PLATFORM.H"
#include "../../compat/win32_internal.h"

#include <SDL.h>
#include <math.h>
#include <stdlib.h>

SDL_Joystick *		GamepadTestJoystick = NULL;

static bool					Enabled = false;
static SDL_GameController *	Pads[4] = {NULL, NULL, NULL, NULL};
static int					Axis[SDL_CONTROLLER_AXIS_MAX];
static Uint32				LastUpdate = 0;
static float				RestX = 0;			// The pointer's movement, less the whole pixels made.
static float				RestY = 0;
static bool					CtrlHeld = false;
static bool					ShiftHeld = false;

static float const			DEADZONE = 0.22f;


static float Stick(int axis)
{
	return(Axis[axis] / 32767.0f);
}


/*
**	The key as the keyboard would give it.
*/
static void Key(int vk, bool down)
{
	bool alt = (vk == VK_MENU);
	UINT message = down ? (alt ? WM_SYSKEYDOWN : WM_KEYDOWN) : (alt ? WM_SYSKEYUP : WM_KEYUP);
	LPARAM lparam = 1;
	if (!down) lparam |= (1 << 30) | (1U << 31);
	if (vk == VK_CONTROL) Win32_Set_Key_State(VK_LCONTROL, down);
	if (vk == VK_SHIFT) Win32_Set_Key_State(VK_LSHIFT, down);
	if (vk == VK_MENU) Win32_Set_Key_State(VK_LMENU, down);
	Win32_Post_Input(message, (WPARAM)vk, lparam);
}


/*
**	Let go of every key and button the controllers hold (when one is unplugged).
*/
static void Release_All(void)
{
	if (CtrlHeld) Key(VK_CONTROL, false);
	if (ShiftHeld) Key(VK_SHIFT, false);
	CtrlHeld = ShiftHeld = false;
	for (int index = 0; index < SDL_CONTROLLER_AXIS_MAX; index++) {
		Axis[index] = 0;
	}
}


void Gamepad_Init(void)
{
	if (getenv("RA_GAMEPAD") != NULL && atoi(getenv("RA_GAMEPAD")) == 0) return;
	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) return;
	Enabled = true;
	SDL_GameControllerEventState(SDL_ENABLE);

	if (getenv("RA_TEST_GAMEPAD") != NULL && atoi(getenv("RA_TEST_GAMEPAD")) != 0) {
		int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
		if (index >= 0) GamepadTestJoystick = SDL_JoystickOpen(index);
	}
	/*
	**	The controllers there are come as SDL_CONTROLLERDEVICEADDED events.
	*/
}


/*
**	Handles a controller event; false if it isn't one.
*/
bool Gamepad_Event(SDL_Event const & event)
{
	if (!Enabled) return(false);

	switch (event.type) {
		case SDL_CONTROLLERDEVICEADDED: {
			for (int index = 0; index < 4; index++) {
				if (Pads[index] == NULL) {
					Pads[index] = SDL_GameControllerOpen(event.cdevice.which);
					break;
				}
			}
			return(true);
		}

		case SDL_CONTROLLERDEVICEREMOVED: {
			for (int index = 0; index < 4; index++) {
				if (Pads[index] != NULL && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(Pads[index])) == event.cdevice.which) {
					SDL_GameControllerClose(Pads[index]);
					Pads[index] = NULL;
				}
			}
			Release_All();
			return(true);
		}

		case SDL_CONTROLLERAXISMOTION: {
			int axis = event.caxis.axis;
			if (axis < 0 || axis >= SDL_CONTROLLER_AXIS_MAX) return(true);
			Axis[axis] = event.caxis.value;

			/*
			**	The triggers hold Ctrl and Shift (pressed past half way, let go below a quarter).
			*/
			if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
				if (!CtrlHeld && event.caxis.value > 16000) Key(VK_CONTROL, CtrlHeld = true);
				else if (CtrlHeld && event.caxis.value < 8000) Key(VK_CONTROL, CtrlHeld = false);
			}
			if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
				if (!ShiftHeld && event.caxis.value > 16000) Key(VK_SHIFT, ShiftHeld = true);
				else if (ShiftHeld && event.caxis.value < 8000) Key(VK_SHIFT, ShiftHeld = false);
			}
			return(true);
		}

		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP: {
			bool down = (event.type == SDL_CONTROLLERBUTTONDOWN);
			switch (event.cbutton.button) {
				case SDL_CONTROLLER_BUTTON_A:					Video_Pad_Button(0, down); break;
				case SDL_CONTROLLER_BUTTON_B:					Video_Pad_Button(1, down); break;
				case SDL_CONTROLLER_BUTTON_X:					Key('S', down); break;
				case SDL_CONTROLLER_BUTTON_Y:					Key('G', down); break;
				case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:	if (down) Video_Pad_Wheel(1); break;
				case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:	if (down) Video_Pad_Wheel(-1); break;
				case SDL_CONTROLLER_BUTTON_LEFTSTICK:		Key(VK_MENU, down); break;
				case SDL_CONTROLLER_BUTTON_RIGHTSTICK:		Key('N', down); break;
				case SDL_CONTROLLER_BUTTON_DPAD_UP:			Key(VK_UP, down); break;
				case SDL_CONTROLLER_BUTTON_DPAD_DOWN:		Key(VK_DOWN, down); break;
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:		Key(VK_LEFT, down); break;
				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:		Key(VK_RIGHT, down); break;
				case SDL_CONTROLLER_BUTTON_BACK:				Key(VK_HOME, down); break;
				case SDL_CONTROLLER_BUTTON_START:			Key(VK_ESCAPE, down); break;
				default: break;
			}
			return(true);
		}

		default:
			break;
	}
	return(false);
}


/*
**	Moves the pointer by the left stick, as time passes (called as the events are pumped).
*/
void Gamepad_Update(void)
{
	if (!Enabled) return;

	Uint32 now = SDL_GetTicks();
	float seconds = (now - LastUpdate) / 1000.0f;
	LastUpdate = now;
	if (seconds > 0.05f) seconds = 0.05f;

	float x = Stick(SDL_CONTROLLER_AXIS_LEFTX);
	float y = Stick(SDL_CONTROLLER_AXIS_LEFTY);
	float length = sqrtf(x*x + y*y);
	if (length < DEADZONE) {
		RestX = RestY = 0;
		return;
	}

	/*
	**	Slow near the middle, for aiming; across the screen in about a second at full tilt.
	*/
	float push = (length - DEADZONE) / (1.0f - DEADZONE);
	if (push > 1.0f) push = 1.0f;
	int width, height;
	Video_Get_Logical_Size(width, height);
	float speed = (0.08f + 0.92f * push * push) * width * 1.1f;
	RestX += x / length * speed * seconds;
	RestY += y / length * speed * seconds;
	int dx = (int)RestX;
	int dy = (int)RestY;
	if (dx != 0 || dy != 0) {
		RestX -= dx;
		RestY -= dy;
		Video_Pad_Move_Mouse(dx, dy);
	}
}


/*
**	How the right stick asks the map to scroll: dx, dy from -256 to 256 (none inside the dead
**	zone). The game scrolls the map by it (ScrollClass::AI).
*/
bool Video_Get_Pad_Scroll(int & dx, int & dy)
{
	dx = dy = 0;
	if (!Enabled) return(false);
	float x = Stick(SDL_CONTROLLER_AXIS_RIGHTX);
	float y = Stick(SDL_CONTROLLER_AXIS_RIGHTY);
	float length = sqrtf(x*x + y*y);
	if (length < DEADZONE) return(false);
	float push = (length - DEADZONE) / (1.0f - DEADZONE);
	if (push > 1.0f) push = 1.0f;
	dx = (int)(x / length * push * 256);
	dy = (int)(y / length * push * 256);
	return(dx != 0 || dy != 0);
}
