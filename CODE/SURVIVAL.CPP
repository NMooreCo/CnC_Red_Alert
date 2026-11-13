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
**	SURVIVAL.CPP -- survival, an option of skirmish games (ENHANCE.H).
**
**	The computer players don't get a base or forces of their own. Instead, every few minutes,
**	each sends a wave of forces, bigger each time (every fifth one bigger still), from one of
**	the start positions that no player took, to hunt down the player's. The mission timer counts
**	down to the next wave. Whoever survives them all (20 by default) and destroys the last one
**	wins. Tuning: RULES.INI [Survival].
**
**	The state goes into saved games after the original's data (ENHANCE.CPP, Enhance_Save);
**	the time of the next wave is the mission timer's, which the original saves.
*/

#include	"FUNCTION.H"

#ifdef RA_ENHANCE

#define	CELLS(n)			((n) * CELL_LEPTON_W)

/*
**	The skirmish dialog's choice (NULLDLG.CPP).
*/
bool Survival_Chosen = false;

static struct SurvivalStateClass {
	bool	IsOn;				// This game is a survival game.
	bool	IsStarting;		// Its scenario is being read (Survival_Prepare).
	int	Wave;				// Waves sent so far.
	int	NextWave;		// The game frame of the next.
	bool	IsOver;			// The last wave was destroyed.
	unsigned	Enemies;		// The computer players at the start (HOUSEF_ bits): they send the waves.
	bool	IsAnnounced;	// The player was told what survival is about.
} Survival;


bool Survival_Is_On(void)
{
	return(Survival.IsOn);
}


/*
**	A computer player of a survival game: it sends the waves. (They are noted when the game
**	starts: a player who leaves a network game is played by the computer, but isn't one.)
*/
bool Survival_Is_Enemy(HouseClass const * house)
{
	if (!Survival.IsOn || house == NULL) return(false);
	if (Survival.Enemies != 0) {
		return((Survival.Enemies & (1U << house->Class->House)) != 0);
	}
	return(!house->IsHuman);
}


/***********************************************************************************************
 * Survival_Prepare -- A scenario is about to be read (Start_Scenario).                        *
 *                                                                                             *
 *    Decided before the scenario is read, so that the computer players get no forces at the  *
 *    start (Create_Units).                                                                    *
 *=============================================================================================*/
void Survival_Prepare(void)
{
	memset(&Survival, 0, sizeof(Survival));
	if (Enhance.Is_Network()) {
		Survival.IsOn = (Enhance.NetFlags & NET_ENH_SURVIVAL) != 0;		// The host's choice.
	} else {
		Survival.IsOn = Survival_Chosen && Session.Type == GAME_SKIRMISH;
	}
	Survival.IsStarting = Survival.IsOn;
}


/***********************************************************************************************
 * Survival_Start -- A scenario has started (Enhancements_Start_Scenario).                     *
 *=============================================================================================*/
void Survival_Start(void)
{
	/*
	**	(Not for a loaded game: Survival_Restore puts its state back.)
	*/
	if (!Survival.IsOn || !Survival.IsStarting) return;
	Survival.IsStarting = false;

	Survival.Wave = 0;
	Survival.IsOver = false;
	Survival.NextWave = Frame + Enhance.SurvivalFirst * TICKS_PER_MINUTE;
	Survival.Enemies = 0;
	for (HousesType house = HOUSE_MULTI1; house <= HOUSE_MULTI8; house++) {
		HouseClass const * hptr = HouseClass::As_Pointer(house);
		if (hptr != NULL && hptr->IsActive && !hptr->IsDefeated && !hptr->IsHuman) {
			Survival.Enemies |= (1U << house);
		}
	}
	Scen.MissionTimer = Survival.NextWave - Frame;
	Scen.MissionTimer.Start();
}


/***********************************************************************************************
 * Survival_Save / Survival_Restore -- In saved games (ENHANCE.CPP).                           *
 *=============================================================================================*/
void Survival_Save(Pipe & pipe)
{
	Enhance_Save_Block(pipe, "SURV", &Survival, sizeof(Survival));
}


void Survival_Restore(void)
{
	memset(&Survival, 0, sizeof(Survival));
	void const * data = Enhance_Loaded_Block("SURV", sizeof(Survival));
	if (data != NULL) {
		memcpy(&Survival, data, sizeof(Survival));
	}
}


/***********************************************************************************************
 * Spawn_Point -- Where a wave comes from.                                                     *
 *                                                                                             *
 *    One of the start positions (waypoints 0 to 7) that is away from the players' bases and  *
 *    from which they can be reached on the ground; failing that, a cell on the edge of the    *
 *    map that can.                                                                            *
 *=============================================================================================*/
static CELL Spawn_Point(HouseClass const * victim)
{
	int zone = Map[victim->Center].Zones[MZONE_NORMAL];

	CELL points[8];
	int count = 0;
	for (int index = 0; index < 8; index++) {
		CELL cell = Scen.Waypoint[index];
		if (cell == -1 || !Map.In_Radar(cell)) continue;
		if (zone != 0 && Map[cell].Zones[MZONE_NORMAL] != zone) continue;

		bool close = false;
		for (HousesType house = HOUSE_MULTI1; house <= HOUSE_MULTI8; house++) {
			HouseClass const * hptr = HouseClass::As_Pointer(house);
			if (hptr != NULL && hptr->IsActive && hptr->IsHuman && !hptr->IsDefeated && hptr->Center != 0 &&
					Distance(Cell_Coord(cell), hptr->Center) < CELLS(14)) {
				close = true;
			}
		}
		if (!close) points[count++] = cell;
	}
	if (count > 0) {
		return(points[Random_Pick(0, count-1)]);
	}

	/*
	**	The edge of the map: a few tries at random.
	*/
	for (int tries = 0; tries < 200; tries++) {
		int x, y;
		switch (Random_Pick(0, 3)) {
			case 0:	x = Map.MapCellX + Random_Pick(0, Map.MapCellWidth-1); y = Map.MapCellY; break;
			case 1:	x = Map.MapCellX + Random_Pick(0, Map.MapCellWidth-1); y = Map.MapCellY + Map.MapCellHeight-1; break;
			case 2:	x = Map.MapCellX; y = Map.MapCellY + Random_Pick(0, Map.MapCellHeight-1); break;
			default:	x = Map.MapCellX + Map.MapCellWidth-1; y = Map.MapCellY + Random_Pick(0, Map.MapCellHeight-1); break;
		}
		CELL cell = XY_Cell(x, y);
		if (Distance(Cell_Coord(cell), victim->Center) < CELLS(14)) continue;
		if (zone != 0 && Map[cell].Zones[MZONE_NORMAL] != zone) continue;
		if (!Map[cell].Is_Clear_To_Move(SPEED_TRACK, false, false)) continue;
		return(cell);
	}
	return(Coord_Cell(victim->Center));
}


/***********************************************************************************************
 * Pick_Type -- Picks a unit or soldier for a wave.                                            *
 *                                                                                             *
 *    Any armed unit or soldier of the house's side (not engineers, harvesters, spies...),      *
 *    of the technology the wave has reached: the first waves are of the early units, and the  *
 *    later ones of everything up to the game's technology level. Dearer ones are wanted more, *
 *    if they fit in what is left of the wave's worth.                                         *
 *=============================================================================================*/
static int Wave_Weight(TechnoTypeClass const & type, HouseClass const * house, int level, int remaining)
{
	if (type.Level < 0 || type.Level > level) return(0);
	if ((type.Get_Ownable() & (1 << house->ActLike)) == 0) return(0);
	if (type.PrimaryWeapon == NULL || type.PrimaryWeapon->Attack <= 0) return(0);
	if (type.Raw_Cost() > remaining + 300) return(0);
	return(4 + type.Raw_Cost() / 100);
}


static TechnoTypeClass const * Pick_Type(HouseClass const * house, int wave, int remaining)
{
	int level = min((int)house->Control.TechLevel, 2 + wave);
	TechnoTypeClass const * types[UNIT_COUNT + INFANTRY_COUNT];
	int weights[UNIT_COUNT + INFANTRY_COUNT];
	int count = 0;
	int total = 0;

	for (UnitType unit = UNIT_FIRST; unit < UNIT_COUNT; unit++) {
		if (!NewUnitsEnabled && unit >= UNIT_RA_COUNT) continue;
		UnitTypeClass const & type = UnitTypeClass::As_Reference(unit);
		if (type.IsToHarvest || unit == UNIT_MCV) continue;
		int weight = Wave_Weight(type, house, level, remaining);
		if (weight > 0) {
			types[count] = &type;
			weights[count++] = weight;
			total += weight;
		}
	}
	for (InfantryType infantry = INFANTRY_FIRST; infantry < INFANTRY_COUNT; infantry++) {
		if (!NewUnitsEnabled && infantry >= INFANTRY_RA_COUNT) continue;
		InfantryTypeClass const & type = InfantryTypeClass::As_Reference(infantry);
		if (type.IsCapture || type.IsCivilian || type.IsBomber) continue;
		int weight = Wave_Weight(type, house, level, remaining);
		if (weight > 0) {
			types[count] = &type;
			weights[count++] = weight;
			total += weight;
		}
	}

	if (total == 0) return(NULL);
	int pick = Random_Pick(0, total-1);
	for (int index = 0; index < count; index++) {
		if (pick < weights[index]) return(types[index]);
		pick -= weights[index];
	}
	return(NULL);
}


/***********************************************************************************************
 * Send_Wave -- A computer player sends its wave.                                              *
 *=============================================================================================*/
static void Send_Wave(HouseClass * house, HouseClass const * victim, int wave)
{
	int worth = Enhance.SurvivalWorth + Enhance.SurvivalGrowth * (wave - 1);
	if ((wave % 5) == 0) worth = worth * 3 / 2;

	CELL origin = Spawn_Point(victim);
	int spent = 0;
	for (int tries = 0; spent < worth && tries < 100; tries++) {
		TechnoTypeClass const * type = Pick_Type(house, wave, worth - spent);
		if (type == NULL) break;

		TechnoClass * techno = (TechnoClass *)type->Create_One_Of(house);
		if (techno == NULL) break;

		CELL cell = Map.Nearby_Location(origin, type->Speed);
		ScenarioInit++;
		bool placed = (cell != 0 && techno->Unlimbo(Cell_Coord(cell), Direction(cell, Coord_Cell(victim->Center))));
		ScenarioInit--;
		if (!placed) {
			delete techno;
			break;
		}
		techno->Assign_Mission(MISSION_HUNT);
		spent += type->Raw_Cost();
	}
}


/***********************************************************************************************
 * Survival_AI -- Sends the waves, and ends the game when the last one is destroyed.           *
 *                                                                                             *
 *    Called every game frame (LogicClass::AI).                                               *
 *=============================================================================================*/
void Survival_AI(void)
{
	if (!Survival.IsOn || Survival.IsOver || PlayerPtr == NULL) return;

	/*
	**	What it's about (once the game is under way: the messages are cleared as it starts).
	*/
	if (!Survival.IsAnnounced && Frame > TICKS_PER_SECOND) {
		Survival.IsAnnounced = true;
		char text[128];
		if (Enhance.SurvivalWaves > 0) {
			snprintf(text, sizeof(text), Text_String(TXT_ENH_SURVIVAL_START), Enhance.SurvivalWaves);
		} else {
			snprintf(text, sizeof(text), "%s", Text_String(TXT_ENH_SURVIVAL_ENDLESS));
		}
		Session.Messages.Add_Message(NULL, 0, text, PCOLOR_GOLD, TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_FULLSHADOW, Rule.MessageDelay * TICKS_PER_MINUTE * 2);
	}

	bool endless = (Enhance.SurvivalWaves <= 0);
	bool more = endless || Survival.Wave < Enhance.SurvivalWaves;

	if (more) {

		/*
		**	The mission timer shows the time to the next wave.
		*/
		if (Frame < Survival.NextWave) {
			int left = Survival.NextWave - Frame;
			if (!Scen.MissionTimer.Is_Active()) Scen.MissionTimer.Start();
			if ((int)(unsigned int)Scen.MissionTimer != left) {
				Scen.MissionTimer = left;
			}
			return;
		}

		/*
		**	The next wave, from every computer player, at the nearest of the players.
		*/
		Survival.Wave++;
		HouseClass const * victim = PlayerPtr;
		for (HousesType house = HOUSE_MULTI1; house <= HOUSE_MULTI8; house++) {
			HouseClass * hptr = HouseClass::As_Pointer(house);
			if (hptr == NULL || !hptr->IsActive || hptr->IsDefeated || !Survival_Is_Enemy(hptr)) continue;

			HouseClass const * target = NULL;
			for (HousesType other = HOUSE_MULTI1; other <= HOUSE_MULTI8; other++) {
				HouseClass const * optr = HouseClass::As_Pointer(other);
				if (optr != NULL && optr->IsActive && optr->IsHuman && !optr->IsDefeated && optr->Center != 0) {
					if (target == NULL || Random_Pick(0, 1)) target = optr;
				}
			}
			Send_Wave(hptr, (target != NULL) ? target : victim, Survival.Wave);
		}

		char text[80];
		if (!endless && Survival.Wave == Enhance.SurvivalWaves) {
			snprintf(text, sizeof(text), "%s", Text_String(TXT_ENH_FINAL_WAVE));
		} else if (endless) {
			snprintf(text, sizeof(text), Text_String(TXT_ENH_WAVE_ENDLESS), Survival.Wave);
		} else {
			snprintf(text, sizeof(text), Text_String(TXT_ENH_WAVE), Survival.Wave, Enhance.SurvivalWaves);
		}
		Session.Messages.Add_Message(NULL, 0, text, PCOLOR_RED, TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_FULLSHADOW, Rule.MessageDelay * TICKS_PER_MINUTE);

		/*
		**	"Soviet forces approaching", or Allied.
		*/
		HouseClass const * first = NULL;
		for (HousesType house = HOUSE_MULTI1; house <= HOUSE_MULTI8 && first == NULL; house++) {
			HouseClass const * hptr = HouseClass::As_Pointer(house);
			if (hptr != NULL && hptr->IsActive && Survival_Is_Enemy(hptr)) first = hptr;
		}
		if (first != NULL && (first->ActLike == HOUSE_USSR || first->ActLike == HOUSE_UKRAINE)) {
			Speak(VOX_SOVIET_FORCES_APPROACHING);
		} else {
			Speak(VOX_ALLIED_FORCES_APPROACHING);
		}

		Survival.NextWave = Frame + Enhance.SurvivalInterval * TICKS_PER_MINUTE;
		if (endless || Survival.Wave < Enhance.SurvivalWaves) {
			Scen.MissionTimer = Survival.NextWave - Frame;
			Scen.MissionTimer.Start();
		} else {
			Scen.MissionTimer.Stop();
		}
		Map.Flag_To_Redraw(true);
		return;
	}

	/*
	**	After the last wave: when nothing is left of it, the computer players are defeated, and
	**	the player wins.
	*/
	for (HousesType house = HOUSE_MULTI1; house <= HOUSE_MULTI8; house++) {
		HouseClass const * hptr = HouseClass::As_Pointer(house);
		if (hptr != NULL && hptr->IsActive && !hptr->IsDefeated && Survival_Is_Enemy(hptr)) {
			if (hptr->ActiveAScan || hptr->UScan || hptr->ActiveIScan || hptr->ActiveVScan) return;
		}
	}
	Survival.IsOver = true;
	for (HousesType house = HOUSE_MULTI1; house <= HOUSE_MULTI8; house++) {
		HouseClass * hptr = HouseClass::As_Pointer(house);
		if (hptr != NULL && hptr->IsActive && !hptr->IsDefeated && Survival_Is_Enemy(hptr)) {
			hptr->MPlayer_Defeated();
		}
	}
}

#endif
