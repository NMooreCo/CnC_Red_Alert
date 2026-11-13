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

class EnhancementsClass
{
	public:
		EnhancementsClass(void);

		/*
		**	The player's choices, kept in REDALERT.INI ([Enhancements]).
		*/
		bool Veterancy;			// Units earn ranks from their kills.
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

		void	Load_Settings(INIClass const & ini);
		void	Save_Settings(INIClass & ini) const;
		void	Reset_Rules(void);
		void	Rules(INIClass const & ini);

		/*
		**	Is the enhancement in effect in this game? Those that change the game rules apply to
		**	campaign and skirmish games: every player in a network game would have to have the
		**	same ones, so they are off there for now.
		*/
		bool	Is_Veterancy(void) const {return(Veterancy && Rules_Apply());}

	private:
		bool	Rules_Apply(void) const;
};

extern EnhancementsClass Enhance;

void	Enhancements_Dialog(void);		// Options, Game Controls, Enhancements.

/*
**	Mission select.
*/
void	Mission_Reached(void);
bool	Mission_Select_Dialog(void);

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
