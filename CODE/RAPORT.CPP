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
 *                 Project Name : Command & Conquer                                            *
 *                    File Name : RAPORT.CPP                                                   *
 *                                                                                             *
 *   Game side glue for the modern (SDL2) port:                                                *
 *     - locating the game data directory,                                                     *
 *     - stand-ins for modules that depended on services which no longer exist               *
 *       (Westwood Online, WChat internet games).                                              *
 *---------------------------------------------------------------------------------------------*/

#include	"FUNCTION.H"
#include	"RAPORT.H"
#include	"PLATFORM.H"


/***********************************************************************************************
 * Port_Allow_Debugger -- Let a debugger attach for automated tests (RA_ALLOW_PTRACE=1).       *
 *                                                                                             *
 *    The test tools in tools/autotest read and set game variables by attaching gdb for a     *
 *    moment. Linux distributions usually allow that only to the process's parent (Yama      *
 *    ptrace_scope 1), unless the process opts in, which this does when asked to.             *
 *=============================================================================================*/
#ifdef __linux__
#include <sys/prctl.h>
#endif
void Port_Allow_Debugger(void)
{
#if defined(__linux__) && defined(PR_SET_PTRACER)
	char const * env = getenv("RA_ALLOW_PTRACE");
	if (env != NULL && atoi(env) != 0) {
		prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
	}
#endif
}


/***********************************************************************************************
 * Port_Extract_Files -- Copy files out of the game's MIX archives.                            *
 *                                                                                             *
 *    RA_EXTRACT=NAME[,NAME...] writes each named file, as the game sees it, to the directory  *
 *    named by RA_EXTRACT_DIR (default: the data directory). Mainly for modding: a RULES.INI   *
 *    or scenario extracted into the data directory and edited there replaces the original.  *
 *                                                                                             *
 * OUTPUT:  Was extraction requested? (The game then exits.)                                   *
 *=============================================================================================*/
bool Port_Extract_Files(void)
{
	char const * list = getenv("RA_EXTRACT");
	if (list == NULL || *list == '\0') return(false);
	char const * dir = getenv("RA_EXTRACT_DIR");

	char names[1024];
	strncpy(names, list, sizeof(names) - 1);
	names[sizeof(names) - 1] = '\0';

	for (char * name = strtok(names, ", "); name != NULL; name = strtok(NULL, ", ")) {
		CCFileClass file(name);
		if (!file.Is_Available()) {
			fprintf(stderr, "%s: not found\n", name);
			continue;
		}
		int size = file.Size();
		char * data = new char[size > 0 ? size : 1];
		int read = file.Read(data, size);
		file.Close();

		char path[_MAX_PATH];
		snprintf(path, sizeof(path), "%s%s%s", dir ? dir : "", (dir && *dir) ? "/" : "", name);
		FILE * out = fopen(path, "wb");
		if (out == NULL || (int)fwrite(data, 1, read, out) != read) {
			fprintf(stderr, "%s: unable to write %s\n", name, path);
		} else {
			fprintf(stderr, "%s: %d bytes written to %s\n", name, read, path);
		}
		if (out != NULL) fclose(out);
		delete [] data;
	}
	return(true);
}


/***********************************************************************************************
 * Port_Select_Data_Directory -- Make the game data directory the current directory.          *
 *                                                                                             *
 *    The game opens all of its files relative to the current directory. Use, in order:       *
 *       1. the directory named by the RA_DATA_DIR environment variable,                      *
 *       2. the current directory, if it holds REDALERT.MIX,                                  *
 *       3. the directory holding the executable, if it holds REDALERT.MIX.                   *
 *=============================================================================================*/
void Port_Select_Data_Directory(int , char * [])
{
	char const * env = getenv("RA_DATA_DIR");
	if (env != NULL && *env) {
		if (!SetCurrentDirectory(env)) {
			fprintf(stderr, "Red Alert: unable to change to RA_DATA_DIR \"%s\".\n", env);
		}
		return;
	}

	if (GetFileAttributes("REDALERT.MIX") != INVALID_FILE_ATTRIBUTES) return;

	char const * base = Platform_Get_Base_Path();
	if (base != NULL && *base) {
		char path[_MAX_PATH];
		sprintf(path, "%sREDALERT.MIX", base);
		if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
			SetCurrentDirectory(base);
			return;
		}
	}

	fprintf(stderr,
		"Red Alert: the game data files were not found.\n"
		"  Run the game from the directory that holds REDALERT.MIX (and MAIN.MIX or\n"
		"  CD1/MAIN.MIX, CD2/MAIN.MIX), or set RA_DATA_DIR to that directory.\n");
}


/***********************************************************************************************
 * Get_CPU_Clock -- Returns a high resolution time stamp (originally the RDTSC instruction).  *
 *=============================================================================================*/
#include <chrono>
unsigned int Get_CPU_Clock(unsigned int & high)
{
	unsigned long long ticks = (unsigned long long)std::chrono::duration_cast<std::chrono::nanoseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
	high = (unsigned int)(ticks >> 32);
	return((unsigned int)ticks);
}


/***********************************************************************************************
 *   Stand-ins for modules that are not part of the modern port.                               *
 *                                                                                             *
 *   WChat / Planet Westwood (INTERNET.CPP, CCDDE.CPP, TCPIP.CPP) launched internet games and *
 *   collected statistics. Those services no longer exist; these definitions keep the shared  *
 *   game code linking and behave as "not launched from WChat".                               *
 *=============================================================================================*/
#include	"CCDDE.H"
#include	"INTERNET.H"

char PlanetWestwoodIPAddress[IP_ADDRESS_MAX] = "";
int PlanetWestwoodPortNumber = 1234;
bool PlanetWestwoodIsHost = false;
bool GameStatisticsPacketSent = false;
bool ConnectionLost = false;
void * PacketLater = NULL;
bool SpawnedFromWChat = false;
int ShowCommand = SW_SHOWNORMAL;
bool WindowsNT = true;
int RandNumb = 0x12349876;

void Register_Game_Start_Time(void) {}
void Register_Game_End_Time(void) {}
void Send_Statistics_Packet(void) {GameStatisticsPacketSent = true;}
void Check_From_WChat(char * ) {}
bool Spawn_WChat(bool ) {return(false);}
int Read_Game_Options(char * ) {return(0);}
BOOL Send_Data_To_DDE_Server(char * , int , int ) {return(FALSE);}

DDEServerClass DDEServer;
DDEServerClass::DDEServerClass(void) : MPlayerGameInfo(NULL), MPlayerGameInfoLength(0), IsEnabled(FALSE), LastHeartbeat(0) {}
DDEServerClass::~DDEServerClass(void) {}
char * DDEServerClass::Get_MPlayer_Game_Info(void) {return(NULL);}
BOOL DDEServerClass::Callback(unsigned char * , int ) {return(FALSE);}
void DDEServerClass::Delete_MPlayer_Game_Info(void) {}
void DDEServerClass::Enable(void) {IsEnabled = TRUE;}
void DDEServerClass::Disable(void) {IsEnabled = FALSE;}
int DDEServerClass::Time_Since_Heartbeat(void) {return(0);}

/*
**	From WOL_GSUP.CPP: Aftermath multiplayer maps with embedded Aftermath only units.
*/
bool bSpecialAftermathScenario( const char* szScenarioDescription )
{
	if( strcmp( szScenarioDescription, "Booby Traps (Mega 8 players)" ) == 0 ||
		strcmp( szScenarioDescription, "Central Conflict Extreme (Mega 8 players)" ) == 0 ||
		strcmp( szScenarioDescription, "Circles of Death (Mega 8 players)" ) == 0 ||
		strcmp( szScenarioDescription, "Holy Grounds (Mega 8 players)" ) == 0 ||
		strcmp( szScenarioDescription, "Island Wars Extreme (Mega 8 players)" ) == 0 ||
		strcmp( szScenarioDescription, "King of the Hills Extreme (Mega 8 players)" ) == 0 ||
		strcmp( szScenarioDescription, "The Hills Have Eyes (Mega 8 players)" ) == 0 )
		return true;
	return false;
}

/*
**	Debugging hooks that used to break into the debugger / control the Watcom profiler.
*/
extern "C" void Stop_Execution(void) {}
extern "C" void Stop_Profiler(void) {}
extern "C" void Set_Palette_Register(int , int , int , int ) {}
