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
	TXT_ENH_LAST
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

		void	Load_Settings(INIClass const & ini);
		void	Save_Settings(INIClass & ini) const;
		void	Reset_Rules(void);
		void	Rules(INIClass const & ini);
		void	Adjust_Objects(void);

		/*
		**	Is the enhancement in effect in this game? Those that change the game rules apply to
		**	campaign and skirmish games: every player in a network game would have to have the
		**	same ones, so they are off there for now.
		*/
		bool	Is_Veterancy(void) const {return(Veterancy && Rules_Apply());}
		bool	Is_Shared_Tech(void) const {return(SharedTech && Session.Type == GAME_SKIRMISH);}
		bool	Is_Support_Powers(void) const {return(SupportPowers && Rules_Apply());}
		bool	Is_Attack_Move(void) const {return(AttackMove && Rules_Apply());}

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
bool	Is_Attack_Moving(FootClass const * unit);

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
