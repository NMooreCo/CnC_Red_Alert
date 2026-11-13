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
**	ENHANCE.H -- optional gameplay enhancements, which the original game didn't have. Each is
**	off until the player turns it on, so by default the game plays as it always did. See
**	ENHANCE.CPP. The code that calls in here is marked RA_ENHANCE.
*/
#ifndef ENHANCE_H
#define ENHANCE_H

/*
**	Texts that the game's string files don't have: Text_String (INLINE.H) gets them from
**	Enhance_Text.
*/
enum {
	TXT_ENH_FIRST = 20000,
	TXT_ENH_ARTILLERY = TXT_ENH_FIRST,
	TXT_ENH_BOMBARDMENT,
	TXT_ENH_AIRDROP,
	TXT_ENH_SURVIVAL,
	TXT_ENH_SURVIVAL_START,
	TXT_ENH_SURVIVAL_ENDLESS,
	TXT_ENH_WAVE,
	TXT_ENH_WAVE_ENDLESS,
	TXT_ENH_FINAL_WAVE,
	TXT_ENH_CONNECT_HINT,
	TXT_ENH_CONNECTING,
	TXT_ENH_COOP,
	TXT_ENH_LAST
};

/*
**	The enhancements that change the rules, as the host of a network game sends them
**	(EnhancementsClass::NetFlags).
*/
enum {
	NET_ENH_VETERANCY = 0x01,
	NET_ENH_SUPPORT_POWERS = 0x02,
	NET_ENH_ATTACK_MOVE = 0x04,
	NET_ENH_SMART_AI = 0x08,
	NET_ENH_SHARED_TECH = 0x10,
	NET_ENH_SURVIVAL = 0x20,
	NET_ENH_COOP = 0x40
};

class EnhancementsClass
{
	public:
		EnhancementsClass(void);

		/*
		**	The player's choices, kept in REDALERT.INI ([Enhancements]).
		*/
		bool Veterancy;			// Units earn ranks from their kills.
		bool BuildQueue;			// Queue up units at each factory from the sidebar.
		bool SharedTech;			// Skirmish: both sides can build everything.
		bool SmartAI;				// Skirmish: the smarter computer players (SMARTAI.CPP).
		bool SupportPowers;		// Artillery barrage, naval bombardment, Chinook reinforcements.
		bool AttackMove;			// <CTRL><SHIFT> (or <CTRL><ALT>) click on the ground.
		bool ModernMouse;			// The left button selects, the right one gives orders.
		bool MissionSelect;		// Start a new campaign at any mission reached so far,
		bool AllMissions;			// or at any mission at all (REDALERT.INI only).

		/*
		**	The furthest mission reached in each campaign (kept whether mission select is on
		**	or not).
		*/
		int	AlliedMission;
		int	SovietMission;

		/*
		**	Veterancy tuning: RULES.INI [Veterancy], which a scenario can override with its own
		**	[Veterancy] section.
		*/
		int	VeteranKills;			// Kills to become a veteran,
		int	EliteKills;				// and elite.
		fixed	VeteranFirepower;		// Damage dealt (times).
		fixed	VeteranArmor;			// Damage taken (times).
		fixed	VeteranROF;				// Time between shots (times).
		fixed	EliteFirepower;
		fixed	EliteArmor;
		fixed	EliteROF;
		int	EliteRepairDelay;		// Game frames between self repair steps of elite units,
		fixed	EliteRepairStep;		// each repairing this much of full strength.

		/*
		**	Support powers tuning: RULES.INI [SupportPowers].
		*/
		fixed	ArtilleryTime;			// Minutes to recharge.
		int	ArtilleryShells;
		int	ArtilleryDamage;		// Of each shell.
		int	ArtillerySpread;		// Cells from the target.
		fixed	BombardmentTime;
		int	BombardmentShells;
		int	BombardmentDamage;
		int	BombardmentSpread;
		fixed	AirdropTime;
		int	AirdropInfantry;		// Soldiers the Chinook brings (every third one has a rocket launcher).

		/*
		**	Survival tuning: RULES.INI [Survival].
		*/
		fixed	SurvivalFirst;			// Minutes before the first wave,
		fixed	SurvivalInterval;		// and between waves.
		int	SurvivalWaves;			// Waves to survive (0: no end).
		int	SurvivalWorth;			// Worth (cost) of the first wave's forces,
		int	SurvivalGrowth;		// and more for each wave after it (every fifth wave half as much again).

		void	Load_Settings(INIClass const & ini);
		void	Save_Settings(INIClass & ini) const;
		void	Reset_Rules(void);
		void	Rules(INIClass const & ini);
		void	Adjust_Objects(void);

		/*
		**	Network games: every player's game has to make exactly the same moves, so all of them
		**	play by the host's choice of the enhancements that change the rules. The host sends it
		**	with the game's options (NETDLG.CPP), and it is kept here.
		*/
		unsigned NetFlags;
		unsigned Net_Flags(void) const;			// The host's own choice, to send.

		/*
		**	Games over the internet: the address last asked for with /connect, and the port
		**	for network games (REDALERT.INI [Network]).
		*/
		char	NetConnect[64];
		int	NetPort;
		void	Net_Set(unsigned flags) {NetFlags = flags;}	// The choice for the game, as received.

		/*
		**	Is the enhancement in effect in this game? The player's choice in campaign and
		**	skirmish games, the host's in network games.
		*/
		bool	Is_Veterancy(void) const {return(Is_Network() ? (NetFlags & NET_ENH_VETERANCY) != 0 : Veterancy && Rules_Apply());}
		bool	Is_Shared_Tech(void) const {return(Is_Network() ? (NetFlags & NET_ENH_SHARED_TECH) != 0 : SharedTech && Session.Type == GAME_SKIRMISH);}
		bool	Is_Smart_AI(void) const {return(Is_Network() ? (NetFlags & NET_ENH_SMART_AI) != 0 : SmartAI && Session.Type == GAME_SKIRMISH);}
		bool	Is_Support_Powers(void) const {return(Is_Network() ? (NetFlags & NET_ENH_SUPPORT_POWERS) != 0 : SupportPowers && Rules_Apply());}
		bool	Is_Attack_Move(void) const {return(Is_Network() ? (NetFlags & NET_ENH_ATTACK_MOVE) != 0 : AttackMove && Rules_Apply());}
		bool	Is_Network(void) const {return(Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH);}

	private:
		bool	Rules_Apply(void) const;
};

extern EnhancementsClass Enhance;

void	Enhancements_Dialog(void);		// Options, Game Controls, Enhancements.
void	Enhancements_Start_Scenario(void);	// A scenario has started, or a saved game was loaded.

/*
**	The mouse wheel (always on: the original had no wheel). GadgetClass::Input gives it to a
**	list under the mouse, and leaves the rest in Mouse_Wheel_Left for the sidebar's strips.
*/
int	Mouse_Wheel_Take(void);
extern int	Mouse_Wheel_Left;

/*
**	Mod folders: mods/NAME/ in the data directory.
*/
extern char const * Mod_Path;		// "mods/NAME/", or NULL.
void	Mod_Select(int argc, char * argv[]);

/*
**	Network games over the internet: the lobby's /connect command (NETDLG.CPP).
*/
bool	Net_Connect_Command(char const * text);
void	Mod_Register_Mixfiles(void);
bool	Mod_File_Name(char const * filename, char * path, int size);

/*
**	Mission select.
*/
void	Mission_Reached(void);
bool	Mission_Select_Dialog(void);

/*
**	Support powers.
*/
void	Support_Powers_Init(HouseClass * house);
void	Support_Powers_AI(HouseClass * house);
void	Support_Power_Fire(HouseClass * house, SpecialWeaponType power, CELL cell);
void	Support_Powers_Impacts(void);

/*
**	Modern mouse controls (DisplayClass::TacticalClass::Action).
*/
bool	Modern_Mouse_Order(ActionType action);

/*
**	Attack-move.
*/
void	Attack_Move_Order(FootClass * unit, bool attack_move);
void	Attack_Move_Set(FootClass * unit, bool attack_move);
bool	Is_Attack_Moving(FootClass const * unit);

/*
**	An object is gone (TRACKER.CPP, Detach_This_From_All).
*/
void	Enhance_Detach(TARGET target, bool all);

/*
**	The smarter computer players (SMARTAI.CPP), hooked into the original's (HOUSE.CPP).
*/
bool	Smart_AI_Is(HouseClass const * house);
void	Smart_AI(HouseClass * house);
bool	Smart_AI_Unit(HouseClass * house);
bool	Smart_AI_Infantry(HouseClass * house);
bool	Smart_AI_Vessel(HouseClass * house);
bool	Smart_AI_Buildings(HouseClass * house, int money, bool hasincome);
bool	Smart_AI_Is_Away(FootClass const * unit);
void	Smart_AI_Harvester_Attacked(UnitClass const * harvester, TechnoClass const * source);
void	Smart_AI_Detach(TARGET target);
void	Smart_AI_Clear(void);
void	Smart_AI_Save(Pipe & pipe);
void	Smart_AI_Restore(void);

/*
**	Survival, an option of skirmish games (SURVIVAL.CPP).
*/
extern bool Survival_Chosen;
extern bool Coop_Chosen;			// Network games: the players against the computer players (NETDLG.CPP).
bool	Survival_Is_On(void);
bool	Survival_Is_Enemy(HouseClass const * house);
void	Survival_Prepare(void);
void	Survival_Start(void);
void	Survival_AI(void);
void	Survival_Save(Pipe & pipe);
void	Survival_Restore(void);

/*
**	The enhancements' state in saved games: after the original's data (SAVELOAD.CPP).
*/
void	Enhance_Save(Pipe & pipe);
void	Enhance_Load(Straw & straw);
void	Enhancements_Restore(void);
void	Enhance_Save_Block(Pipe & pipe, char const * name, void const * data, int size);
void const * Enhance_Loaded_Block(char const * name, int size);

/*
**	Build queue (the sidebar).
*/
bool	Build_Queue_Add(RTTIType type, int id);
bool	Build_Queue_Remove(RTTIType type, int id);
int	Build_Queue_Count(RTTIType type, int id);
void	Build_Queue_AI(void);
void	Build_Queue_Clear(void);

/*
**	Veterancy. Every unit has always had a kill counter ("When it reaches a certain point, the
**	unit improves", CREW.H), but the original never finished the idea.
*/
enum VeterancyType {
	VETERANCY_ROOKIE,
	VETERANCY_VETERAN,
	VETERANCY_ELITE
};

VeterancyType	Veterancy_Rank(TechnoClass const * techno);
void				Veterancy_Credit_Kill(TechnoClass * killer, TechnoClass const * victim);
fixed				Veterancy_Firepower(TechnoClass const * techno);
fixed				Veterancy_Armor(TechnoClass const * techno);
fixed				Veterancy_ROF(TechnoClass const * techno);
void				Veterancy_AI(TechnoClass * techno);
void				Veterancy_Draw(TechnoClass const * techno, GraphicViewPortClass & window, int x, int y);

#endif
