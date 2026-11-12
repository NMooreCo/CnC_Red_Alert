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

/***************************************************************************
 **     C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S       **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Timer Class Functions                    *
 *                                                                         *
 *                    File Name : TIMER.H                                  *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : July 6, 1994                             *
 *                                                                         *
 *                  Last Update : July 12, 1994   [SKB]                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef TIMER_H
#define TIMER_H


#ifndef WIN32
#define WIN32 1
#endif
#include <windows.h>
#include <windowsx.h>

/*=========================================================================*/
/* The following prototypes are for the file: TIMERA.ASM							*/
/*=========================================================================*/

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Externs /////////////////////////////////////////////
extern BOOL 		TimerSystemOn;
extern	HANDLE	TimerThreadHandle;		//Handle of timer thread
extern	int		InTimerCallback;			//true if we are currently in a callback


/*=========================================================================*/
typedef enum BaseTimerEnum {
	BT_SYSTEM,			// System timer (60 / second).
	BT_USER				// User controllable timer (? / second).
} BaseTimerEnum;

class TimerClass {
 	public:
		// Constructor.  Timers set before low level init has been done will not
		// be able to be 'Started' or 'on' until timer system is in place.
		TimerClass(BaseTimerEnum timer=BT_SYSTEM, BOOL start=FALSE);

		// No destructor.
		~TimerClass(void){}

		//
		int Set(int value, BOOL start=TRUE);	// Set initial timer value.
		int Stop(void);				// Pause timer.
		int Start(void);				// Resume timer.
		int Reset(BOOL start=TRUE);	// Reset timer to zero.
		int Time(void);				// Fetch current timer value.

	protected:
		int Started;					// Time last started (0 == not paused).
		int Accumulated;				//	Total accumulated ticks.

	private:
//		long (*Get_Ticks)(void);	// System timer fetch.
		BaseTimerEnum	TickType;
		int Get_Ticks (void);
};


inline int TimerClass::Reset(BOOL start)
{
	return(Set(0, start));
}


class CountDownTimerClass : private TimerClass {
	public:
		// Constructor.  Timers set before low level init has been done will not
		// be able to be 'Started' or 'on' until timer system is in place.
		CountDownTimerClass(BaseTimerEnum timer, int set, int on);	// (no default: was "long set", 2 argument calls meant the other constructor)
		CountDownTimerClass(BaseTimerEnum timer=BT_SYSTEM, int on=FALSE);

		// No destructor.
		~CountDownTimerClass(void){}

		// Public functions
		int Set(int set, BOOL start=TRUE);	// Set count down value.
		int Reset(BOOL start=TRUE);	// Reset timer to zero.
		int Stop(void);			// Pause timer.
		int Start(void);			// Resume timer.
		int Time(void);			// Fetch current count down value.

	protected:
		int DelayTime;			// Ticks remaining before countdown timer expires.
};

inline int CountDownTimerClass::Stop(void)
{
	TimerClass::Stop();
	return(Time());
}

inline int CountDownTimerClass::Start(void)
{
	TimerClass::Start();
	return(Time());
}

inline int CountDownTimerClass::Reset(BOOL start)
{
	return (TimerClass::Reset(start));
}




class WinTimerClass {

	public:
		WinTimerClass ( UINT freq=60 , BOOL partial=0 );
		~WinTimerClass();

		void 		Update_Tick_Count ( void );
		unsigned	Get_System_Tick_Count ( void );
		unsigned	Get_User_Tick_Count ( void );

	private:

		unsigned		TimerHandle;	//Handle for windows timer event
		unsigned		Frequency;		//Frequency of our windows timer in ticks per second

		unsigned		TrueRate;		//True rate of clock. (only use word)
		unsigned		SysTicks;		//Tick count of timer.
		unsigned		UserTicks;		//Tick count of timer.
		unsigned		UserRate;		//Desired rate of timer.


};


extern	WinTimerClass	*WindowsTimer;








//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// externs  //////////////////////////////////////////
#ifndef FUNCTION_H
#ifdef RA_PORT
/*
**	RA_PORT: The game has a global "TickCount" of its own, of a different type (and hides
**	this declaration, see FUNCTION_H). Watcom put the type into the decorated name of a
**	variable, so the two never clashed; modern C++ ABIs don't, so the library's one gets a
**	name of its own.
*/
#define TickCount	WWLib_TickCount
#endif
extern TimerClass					TickCount;
#endif
extern CountDownTimerClass		CountDown;

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Prototypes //////////////////////////////////////////

extern "C" {
	int __cdecl Get_System_Tick_Count(void);
	int __cdecl Get_User_Tick_Count(void);
	void far __cdecl Timer_Interrupt_Func(void);
//	long Get_Num_Interrupts(unsigned int realmode);
	void __cdecl Disable_Timer_Interrupt(void);
	void __cdecl Enable_Timer_Interrupt(void);
}

/*=========================================================================*/
/* The following prototypes are for the file: TIMER.CPP							*/
/*=========================================================================*/
BOOL __cdecl Init_Timer_System(unsigned int freq, int partial = FALSE);
BOOL __cdecl Remove_Timer_System(VOID);


#endif // TIMER_H

