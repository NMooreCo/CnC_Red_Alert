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
**	COOP.CPP -- the campaign together, in network games (RA_ENHANCE).
**
**	At the end of the network games' scenario list come the campaign missions ("Co-op Allied
**	1: In the thick of it"...). The players of a game on one of them play it together, as its
**	side: they share its forces, its base and its money, and each can give any order.
**
**	The mission runs as it does for one player. The game takes it for a single player game
**	(Session.Type is GAME_NORMAL, PlayerPtr the mission's side), so its triggers, its computer
**	players and its rules are the campaign's. Only the network code knows better (Net_Type:
**	the network's game type): the players' orders travel as in a network game, each from the
**	multiplayer house it was given (Coop.NetHouse: the network code tells the players apart
**	by their houses), and they are carried out for the mission's side (Event_House). Won or
**	lost, the mission ends for everyone and they go back to the menus.
*/

#include	"FUNCTION.H"

#ifdef RA_ENHANCE

static struct {
	bool IsOn;
	GameType NetType;					// The network's game type (Session.Type says GAME_NORMAL).
	int NetHouse;						// This player's house, to the network (a multiplayer house).
} Coop = {false, GAME_NORMAL, 0};


bool Coop_Is_On(void)
{
	return(Coop.IsOn);
}


/*
**	The game type, to the network code: in a co-op mission, the network's.
*/
GameType Net_Type(void)
{
	return(Coop.IsOn ? Coop.NetType : Session.Type);
}


/*
**	Does the game stop while a dialog is up (as it does for one player)? In a network game, and
**	so in a co-op mission, it goes on (the dialogs call Main_Loop).
*/
bool Game_Pauses_In_Dialogs(void)
{
	if (Coop.IsOn) return(false);
	return(Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH);
}


/*
**	Who this player's orders are from, to the network (EventClass::ID).
*/
int Local_Event_ID(void)
{
	return(Coop.IsOn ? Coop.NetHouse : PlayerPtr->ID);
}


/*
**	The house an order is carried out for: in a co-op mission, every player's is the side's.
*/
HouseClass * Event_House(int id)
{
	if (Coop.IsOn && id >= HOUSE_MULTI1 && id < HOUSE_MULTI1 + MAX_PLAYERS) {
		return(PlayerPtr);
	}
	return(Houses.Raw_Ptr(id));
}


/*
**	Is this the file of a campaign mission ("SCG01EA.INI": Allied or Soviet, its number, its
**	version)?
*/
bool Coop_Is_Mission(char const * filename)
{
	if (filename == NULL || strlen(filename) != 11) return(false);
	if (toupper(filename[0]) != 'S' || toupper(filename[1]) != 'C') return(false);
	char side = (char)toupper(filename[2]);
	if (side != 'G' && side != 'U') return(false);
	if (!isdigit(filename[3]) || !isdigit(filename[4])) return(false);
	char dir = (char)toupper(filename[5]);
	if (dir != 'E' && dir != 'W') return(false);
	return(stricmp(&filename[7], ".INI") == 0);
}


/*
**	Adds the campaign missions there are to the end of the scenario list (after the random
**	maps): "Co-op Allied 3B: Dead End" and so on. The skirmish dialog leaves them out.
*/
void Coop_Add_Missions(void)
{
	static char const * const sides[2] = {"G", "U"};
	for (int s = 0; s < 2; s++) {
		for (int number = 1; number <= 99; number++) {
			bool found = false;
			char main_name[48] = "";
			for (char const * version = "EAEBECEDWAWBWCWD"; *version != '\0'; version += 2) {
				char file[16];
				snprintf(file, sizeof(file), "SC%s%02d%c%c.INI", sides[s], number, version[0], version[1]);
				CCFileClass scenario(file);
				if (!scenario.Is_Available()) continue;

				char name[48] = "";
				CCINIClass ini;
				if (ini.Load(scenario, false)) {
					ini.Get_String("Basic", "Name", "", name, sizeof(name));
				}
				if (name[0] == '\0' || stricmp(name, "<none>") == 0) {
					strcpy(name, main_name);
				}
				if (!found) strcpy(main_name, name);

				char text[DESCRIP_MAX + 32];
				char label[24];
				if (!found) {
					snprintf(label, sizeof(label), "%s %d", (s == 0) ? "Allied" : "Soviet", number);
				} else {
					snprintf(label, sizeof(label), "%s %d%s%c", (s == 0) ? "Allied" : "Soviet", number, (version[0] == 'W') ? "W" : "", version[1]);
				}
				if (name[0] != '\0') {
					snprintf(text, sizeof(text), "Co-op %s: %s", label, name);
				} else {
					snprintf(text, sizeof(text), "Co-op %s", label);
				}
				text[DESCRIP_MAX - 1] = '\0';
				Session.Scenarios.Add(new MultiMission(file, text, NULL, true, false));
				found = true;
			}
			if (!found && number > 1) break;
		}
	}
}


/*
**	Is entry "index" of the scenario list a co-op mission?
*/
bool Coop_Is_Entry(int index)
{
	if (index < 0 || index >= Session.Scenarios.Count()) return(false);
	return(Coop_Is_Mission(Session.Scenarios[index]->Get_Filename()));
}


/***********************************************************************************************
 * Coop_Begin -- A network game on a campaign mission is played as the campaign, together.    *
 *                                                                                             *
 *    Called as the game starts, before the scenario is read (Select_Game).                    *
 *=============================================================================================*/
void Coop_Begin(void)
{
	Coop.IsOn = false;
	if (Session.Type != GAME_IPX && Session.Type != GAME_INTERNET) return;
	if (Session.LoadGame || Session.Play || !Coop_Is_Mission(Scen.ScenarioName)) return;

	Coop.IsOn = true;
	Coop.NetType = Session.Type;
	Session.Type = GAME_NORMAL;
	char name[_MAX_FNAME+_MAX_EXT];
	strncpy(name, Scen.ScenarioName, sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';
	Scen.Set_Scenario_Name(name);
	Scen.CarryOverMoney = 0;

	/*
	**	Of the host's enhancements, those that suit a campaign mission (the computer players'
	**	are the mission's, and there are no survival waves or co-op alliances).
	*/
	Enhance.Net_Set(Enhance.NetFlags & (NET_ENH_VETERANCY | NET_ENH_SUPPORT_POWERS | NET_ENH_ATTACK_MOVE));
	Scen.CDifficulty = DIFF_NORMAL;
	Scen.Difficulty = DIFF_NORMAL;
}


/***********************************************************************************************
 * Coop_Assign -- Gives each player a multiplayer house to be known by on the network.         *
 *                                                                                             *
 *    As Assign_Houses does in a network game (the players in order of their colors, which all *
 *    the computers agree on), but the houses only stand for the players: PlayerPtr stays the  *
 *    mission's side. Called once the scenario is read.                                        *
 *=============================================================================================*/
void Coop_Assign(void)
{
	if (!Coop.IsOn) return;

	bool assigned[MAX_PLAYERS];
	for (int i = 0; i < MAX_PLAYERS; i++) assigned[i] = false;

	for (int i = 0; i < Session.Players.Count() && i < MAX_PLAYERS; i++) {
		int index = -1;
		int lowest = 255;
		for (int j = 0; j < Session.Players.Count() && j < MAX_PLAYERS; j++) {
			if (!assigned[j] && Session.Players[j]->Player.Color < lowest) {
				lowest = Session.Players[j]->Player.Color;
				index = j;
			}
		}
		if (index < 0) break;
		assigned[index] = true;

		HousesType house = (HousesType)(HOUSE_MULTI1 + i);
		HouseClass * housep = HouseClass::As_Pointer(house);
		if (housep != NULL) {
			memset((char *)housep->IniName, 0, MPLAYER_NAME_MAX);
			strncpy((char *)housep->IniName, Session.Players[index]->Name, MPLAYER_NAME_MAX - 1);
			housep->IsHuman = true;
		}
		Session.Players[index]->Player.ID = house;
		if (index == 0) Coop.NetHouse = house;
	}
	Session.NumPlayers = Session.Players.Count();
}


/***********************************************************************************************
 * Coop_Show_Briefing -- Shows the mission's briefing over the map, for a minute.              *
 *                                                                                             *
 *    One player sees it in a dialog that stops the game; in a co-op mission the game can't    *
 *    stop, so its lines go to the message list at the top of the map instead. At the start,   *
 *    and from the options menu's briefing button. Only this player's screen changes.          *
 *=============================================================================================*/
void Coop_Show_Briefing(void)
{
	if (!Coop.IsOn || Scen.BriefingText[0] == '\0') return;

	/*
	**	The lines of an earlier showing go first (the messages' id says which they are).
	*/
	int const BRIEFING_ID = 0x7C00;
	bool removed = false;
	for (int index = 0; index < MAX_NUM_MESSAGES; index++) {
		TextLabelClass * label = Session.Messages.Get_Label(BRIEFING_ID + index);
		if (label != NULL) {
			label->UserData1 = 1;			// Its time is up.
			removed = true;
		}
	}
	if (removed && Session.Messages.Manage()) {
		HiddenPage.Clear();
		Map.Flag_To_Redraw(true);
	}

	static char text[sizeof(Scen.BriefingText)];
	strncpy(text, Scen.BriefingText, sizeof(text) - 1);
	text[sizeof(text) - 1] = '\0';

	/*
	**	Broken into lines that fit (in the messages' font), as a dialog would break it; '@'
	**	starts a new line.
	*/
	TextPrintType const style = TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW;
	Fancy_Text_Print(TXT_NONE, 0, 0, &ColorRemaps[PCOLOR_GREEN], TBLACK, style);
	int width = min(Lepton_To_Pixel(Map.TacLeptonWidth) - 16, 220 * RESFACTOR);
	int w, h;
	Format_Window_String(text, width, w, h);

	char * line = text;
	int count = 0;
	while (line != NULL && count < MAX_NUM_MESSAGES) {
		char * end = strchr(line, '\r');
		if (end != NULL) *end = '\0';
		while (*line == ' ') line++;
		if (*line != '\0') {
			Session.Messages.Add_Message(NULL, BRIEFING_ID + count++, line, PCOLOR_GREEN, style, TIMER_SECOND * 60);
		}
		line = (end != NULL) ? end + 1 : NULL;
	}
	Map.Flag_To_Redraw(false);
}


/***********************************************************************************************
 * Coop_End -- The mission is over: the network game it was.                                   *
 *                                                                                             *
 *    Session.Type is the network's again, for the network to be shut down as after any       *
 *    network game. Does nothing outside a co-op mission.                                      *
 *=============================================================================================*/
void Coop_End(void)
{
	if (!Coop.IsOn) return;
	Coop.IsOn = false;
	Session.Type = Coop.NetType;
}

#endif
