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
**	SMARTAI.CPP -- the smarter computer player, an optional enhancement (ENHANCE.H) for
**	skirmish games.
**
**	The original computer player (HouseClass::Expert_AI and the AI_ routines in HOUSE.CPP)
**	builds its base well enough, but picks its units at random and, every few minutes, sends
**	most of them off one by one to hunt whatever is nearest; it never builds a radar dome
**	unless an enemy has aircraft (and so never reaches the technology that needs one), nor
**	ships, nor the super weapons. This one, for each computer player:
**
**	- gathers its forces near its base and attacks in waves: together, at a building picked for
**	  its worth (refineries, power, factories) and nearness, moving with attack-move (taking on
**	  what they meet on the way), going on to the next building near the last one, and falling
**	  back to join the next wave when too weakened. Each wave is bigger than the last;
**	- sends a few fast vehicles to raid the enemy's harvesters out in the field;
**	- builds a mix of units against what the enemy has (anti-armor against tanks, anti-infantry
**	  against soldiers, anti-aircraft against aircraft, artillery against base defenses);
**	- builds a radar dome early, a second refinery, more harvesters, a service depot (and sends
**	  damaged vehicles to it), a navy when the enemy can be reached by sea, and the super weapons,
**	  and uses the iron curtain and the chronosphere on its tanks;
**	- has one of four personalities: balanced, rusher (small attacks early, more infantry), turtle
**	  (more defenses, big attacks later) and armored (more tanks).
**
**	What each computer player knows is kept in the Brains array, which goes into saved games after
**	the original's data (ENHANCE.CPP, Enhance_Save). It all runs in the game logic, with the
**	game's random numbers, so it would play out the same on every computer in a network game.
*/

#include	"FUNCTION.H"

#ifdef RA_ENHANCE

/*
**	How often each computer player looks at its forces (game frames).
*/
#define	THINK_DELAY		(TICKS_PER_SECOND*2)

#define	CELLS(n)			((n) * CELL_LEPTON_W)

/***********************************************************************************************
**	Personalities.
*/
enum PersonalityType {
	PERSONALITY_BALANCED,
	PERSONALITY_RUSHER,
	PERSONALITY_TURTLE,
	PERSONALITY_ARMORED,
	PERSONALITY_COUNT
};

static struct PersonalityClass {
	char const * Name;
	int	FirstWaveTime;		// Minutes before the first attack.
	int	Daring;				// Attack when its forces are this much (percent) of the enemy's.
	int	FirstWave;			// Worth (cost) of the forces for the first attack,
	int	WaveGrowth;			// and more for each attack after it,
	int	MaxWave;				// up to this.
	int	RaidDelay;			// Minutes between raids on harvesters.
	int	DefensePercent;	// Defensive buildings, as a share of the base (the original's is 40%).
	int	InfantryPercent;	// Infantry's share of the forces.
	int	ArmorBonus;			// Added to the tanks' share of the forces.
} const Personalities[PERSONALITY_COUNT] = {
	{"Balanced",	6,		140,	4000,	1500,	12000,	4,	40,	30,	0},
	{"Rusher",		3,		110,	1600,	800,	6000,		3,	25,	50,	-10},
	{"Turtle",		10,	170,	7000,	2000,	16000,	7,	48,	30,	0},
	{"Armored",		8,		130,	5000,	1500,	14000,	5,	35,	15,	20},
};


/***********************************************************************************************
**	The part each kind of unit plays, worked out from its weapons (Role_Of), so that units of
**	mods are understood as well.
*/
enum RoleType {
	ROLE_NONE,				// Not for fighting.
	ROLE_ANTI_ARMOR,		// Tanks, rocket soldiers.
	ROLE_ANTI_INFANTRY,	// Machine guns, flame throwers, dogs.
	ROLE_ARTILLERY,		// Long range, against bases.
	ROLE_ANTI_AIR,			// Only shoots at aircraft.
	ROLE_SUPPORT,			// The medic.
	ROLE_COUNT
};


enum WaveStateType {
	WAVE_NONE,				// Building up.
	WAVE_GATHER,			// Gathering at the staging point.
	WAVE_ATTACK				// On the attack.
};

enum {
	MAX_WAVE = 120,
	MAX_RAIDERS = 4,
	MAX_FLEET = 30,
	MAX_REPAIRS = 3
};


/***********************************************************************************************
**	What a computer player knows.
*/
struct BrainClass {
	bool	IsInitialized;
	bool	IsOn;
	PersonalityType Personality;
	int	NextThink;

	/*
	**	The enemy's forces (their worth), and its own, by role. Updated as it thinks.
	*/
	int	EnemyInfantry;
	int	EnemyVehicles;
	int	EnemyAircraft;
	int	EnemyShips;
	int	EnemyDefenses;
	int	Own[ROLE_COUNT];
	int	OwnInfantry;
	int	OwnTotal;

	/*
	**	The attack wave.
	*/
	WaveStateType Wave;
	TARGET Member[MAX_WAVE];
	int	Members;
	CELL	Staging;
	TARGET Objective;
	int	PhaseStart;			// When the current phase began.
	int	StartValue;			// The wave's worth as it set out.
	int	Waves;				// Attacks so far.
	int	LastWave;			// When the last one ended.

	/*
	**	The raid on a harvester.
	*/
	TARGET Raider[MAX_RAIDERS];
	int	Raiders;
	TARGET Prey;
	int	RaidStart;
	int	NextRaid;

	/*
	**	The navy.
	*/
	TARGET Ship[MAX_FLEET];
	int	Ships;
	TARGET FleetObjective;
	CELL	FleetCell;
	int	FleetStart;
	int	NavyCheck;			// When to look at the sea again.
	bool	IsNavyWanted;		// The enemy can be reached by sea,
	int	Sea;					// on this sea (MZONE_WATER zone).

	/*
	**	Vehicles sent to the service depot, and the one sent by the chronosphere.
	*/
	TARGET Repair[MAX_REPAIRS];
	int	Repairs;
	TARGET Chrono;
	int	ChronoStart;

	/*
	**	The enemy that last attacked one of its harvesters, and whether help was sent.
	*/
	TARGET Intruder;
	int	IntruderFrame;
	bool	IsIntruderMet;

	/*
	**	What it has done (for testing and debugging).
	*/
	int	RaidCount;
	int	FleetCount;
	int	RepairCount;
	int	CurtainCount;
	int	ChronoCount;
};

static BrainClass Brains[HOUSE_COUNT];


/***********************************************************************************************
 * Brain_Of -- The brain of a computer player run by the smarter AI, or NULL.                  *
 *                                                                                             *
 *    It is set up the first time it is asked for in a scenario (or after loading a game).     *
 *    The personality comes from the house, its country and its color, which are kept in saved *
 *    games, so it doesn't change when a game is loaded.                                       *
 *=============================================================================================*/
static BrainClass * Brain_Of(HouseClass const * house)
{
	if (house == NULL) return(NULL);
	HousesType id = house->Class->House;
	if ((unsigned)id >= HOUSE_COUNT) return(NULL);

	BrainClass & brain = Brains[id];
	if (!brain.IsInitialized) {
		memset(&brain, 0, sizeof(brain));
		brain.IsInitialized = true;
		brain.IsOn = Enhance.Is_Smart_AI();
		brain.Personality = (PersonalityType)(((int)id * 7 + (int)house->RemapColor * 3 + (int)house->ActLike) % PERSONALITY_COUNT);
		brain.NextThink = Frame + (int)id;
		brain.Wave = WAVE_NONE;
		brain.Objective = TARGET_NONE;
		brain.Prey = TARGET_NONE;
		brain.FleetObjective = TARGET_NONE;
		brain.Chrono = TARGET_NONE;
		brain.Intruder = TARGET_NONE;
		brain.LastWave = Frame;
		brain.NextRaid = Frame + TICKS_PER_MINUTE * Personalities[brain.Personality].RaidDelay;
		brain.NavyCheck = Frame;
	}

	/*
	**	A player's house is only run by the computer when no one plays it.
	*/
	if (!brain.IsOn || house->IsHuman) return(NULL);
	return(&brain);
}


/***********************************************************************************************
 * Smart_AI_Clear -- A scenario has started, or a game was loaded: forget everything.          *
 *=============================================================================================*/
void Smart_AI_Clear(void)
{
	for (int index = 0; index < HOUSE_COUNT; index++) {
		Brains[index].IsInitialized = false;
	}
}


/***********************************************************************************************
 * Smart_AI_Save -- The brains go into saved games (ENHANCE.CPP, Enhance_Save).                *
 * Smart_AI_Restore -- And come back with them.                                                *
 *=============================================================================================*/
void Smart_AI_Save(Pipe & pipe)
{
	Enhance_Save_Block(pipe, "BRNS", Brains, sizeof(Brains));
}


void Smart_AI_Restore(void)
{
	void const * data = Enhance_Loaded_Block("BRNS", sizeof(Brains));
	if (data != NULL) {
		memcpy(Brains, data, sizeof(Brains));
	}
}


bool Smart_AI_Is(HouseClass const * house)
{
	return(Brain_Of(house) != NULL);
}


/***********************************************************************************************
**	Lists of units: the wave, the raiders, the fleet. An entry is dropped when its unit is
**	destroyed or leaves the map (Smart_AI_Detach), or no longer belongs to the house.
*/
static bool Is_Owned(TechnoClass const * techno, HouseClass const * house)
{
	return((HouseClass const *)techno->House == house);
}


static FootClass * Unit_Of(TARGET target, HouseClass const * house)
{
	TechnoClass * techno = As_Techno(target);
	if (techno == NULL || !techno->IsActive || techno->IsInLimbo || techno->Strength <= 0) return(NULL);
	if (!Is_Owned(techno, house) || !techno->Is_Foot()) return(NULL);
	return((FootClass *)techno);
}


static int Prune(TARGET * list, int count, HouseClass const * house)
{
	int kept = 0;
	for (int index = 0; index < count; index++) {
		if (Unit_Of(list[index], house) != NULL) {
			list[kept++] = list[index];
		}
	}
	return(kept);
}


static int Remove(TARGET * list, int count, TARGET target)
{
	for (int index = 0; index < count; index++) {
		if (list[index] == target) {
			list[index--] = list[--count];
		}
	}
	return(count);
}


static bool Is_Listed(TARGET const * list, int count, TARGET target)
{
	for (int index = 0; index < count; index++) {
		if (list[index] == target) return(true);
	}
	return(false);
}


void Smart_AI_Detach(TARGET target)
{
	for (int index = 0; index < HOUSE_COUNT; index++) {
		BrainClass & brain = Brains[index];
		if (!brain.IsInitialized || !brain.IsOn) continue;

		brain.Members = Remove(brain.Member, brain.Members, target);
		brain.Raiders = Remove(brain.Raider, brain.Raiders, target);
		brain.Ships = Remove(brain.Ship, brain.Ships, target);
		brain.Repairs = Remove(brain.Repair, brain.Repairs, target);
		if (brain.Objective == target) brain.Objective = TARGET_NONE;
		if (brain.Prey == target) brain.Prey = TARGET_NONE;
		if (brain.FleetObjective == target) brain.FleetObjective = TARGET_NONE;
		if (brain.Chrono == target) brain.Chrono = TARGET_NONE;
		if (brain.Intruder == target) brain.Intruder = TARGET_NONE;
	}
}


/***********************************************************************************************
 * Smart_AI_Is_Away -- Is the unit off on an attack, a raid or at sea?                         *
 *                                                                                             *
 *    The original calls on nearby units to help when the base is attacked                    *
 *    (TechnoClass::Base_Is_Attacked); those away on an attack aren't called back.             *
 *=============================================================================================*/
bool Smart_AI_Is_Away(FootClass const * unit)
{
	BrainClass * brain = Brain_Of(unit->House);
	if (brain == NULL) return(false);

	TARGET who = unit->As_Target();
	if (brain->Wave == WAVE_ATTACK && Is_Listed(brain->Member, brain->Members, who)) return(true);
	if (Is_Listed(brain->Raider, brain->Raiders, who)) return(true);
	if (Is_Listed(brain->Ship, brain->Ships, who)) return(true);
	return(brain->Chrono == who);
}


/***********************************************************************************************
**	Helpers.
*/
static bool Is_Player_House(HousesType house)
{
	return(house >= HOUSE_MULTI1 && house <= HOUSE_MULTI8);
}


/*
**	Is it an enemy (of one of the players, not the civilians)?
*/
static bool Is_Enemy(HouseClass const * house, TechnoClass const * techno)
{
	if (techno == NULL || !techno->IsActive || techno->IsInLimbo || techno->Strength <= 0) return(false);
	HouseClass const * owner = techno->House;
	if (owner == NULL || !Is_Player_House(owner->Class->House) || owner->IsDefeated) return(false);
	return(!house->Is_Ally(owner));
}


static int Worth(TechnoClass const * techno)
{
	return(techno->Techno_Type_Class()->Raw_Cost());
}


/*
**	Damage per ten seconds against the armor (or aircraft), of the better of its weapons.
*/
static int Firepower(TechnoTypeClass const & type, ArmorType armor, bool air)
{
	int power = 0;
	WeaponTypeClass const * weapons[2] = {type.PrimaryWeapon, type.SecondaryWeapon};

	for (int index = 0; index < 2; index++) {
		WeaponTypeClass const * weapon = weapons[index];
		if (weapon == NULL || weapon->Attack <= 0 || weapon->Bullet == NULL || weapon->WarheadPtr == NULL) continue;
		if (air ? !weapon->Bullet->IsAntiAircraft : !weapon->Bullet->IsAntiGround) continue;

		int value = (weapon->Attack * weapon->WarheadPtr->Modifier[armor]) * (TICKS_PER_SECOND * 10) / max((int)weapon->ROF, 1);
		if (type.Is_Two_Shooter()) value *= 2;
		power = max(power, value);
	}
	return(power);
}


static RoleType Role_Of(TechnoTypeClass const & type)
{
	WeaponTypeClass const * weapon = type.PrimaryWeapon;
	if (weapon == NULL) return(ROLE_NONE);
	if (weapon->Attack < 0) return(ROLE_SUPPORT);

	int armor = Firepower(type, ARMOR_STEEL, false);
	int infantry = Firepower(type, ARMOR_NONE, false);
	if (armor == 0 && infantry == 0) {
		return(Firepower(type, ARMOR_ALUMINUM, true) > 0 ? ROLE_ANTI_AIR : ROLE_NONE);
	}
	if (weapon->Bullet != NULL && weapon->Bullet->IsInaccurate && weapon->Range >= CELLS(5)) {
		return(ROLE_ARTILLERY);
	}
	return((armor >= infantry) ? ROLE_ANTI_ARMOR : ROLE_ANTI_INFANTRY);
}


/*
**	Can the house send this unit to attack? (Its combat units, not harvesters, transports,
**	engineers or the members of the scenario's teams.)
*/
static bool Is_Soldier(FootClass const * foot)
{
	if (foot->Team.Is_Valid() || !foot->Is_Weapon_Equipped()) return(false);

	switch (foot->What_Am_I()) {
		case RTTI_UNIT:
			return(!((UnitClass const *)foot)->Class->IsToHarvest && *(UnitClass const *)foot != UNIT_MCV);

		case RTTI_INFANTRY:
			return(!((InfantryClass const *)foot)->Class->IsCapture && !((InfantryClass const *)foot)->Class->IsCivilian);

		default:
			return(false);
	}
}


/*
**	Is it waiting for orders?
*/
static bool Is_Idle(FootClass const * foot)
{
	MissionType mission = foot->Get_Mission();
	return((mission == MISSION_GUARD || mission == MISSION_GUARD_AREA) && !foot->In_Radio_Contact());
}


/*
**	Is it a combat unit of the house at home, waiting for orders?
*/
static bool Is_At_Home(BrainClass const & brain, FootClass const * foot)
{
	if (!Is_Soldier(foot) || !Is_Idle(foot)) return(false);
	TARGET who = foot->As_Target();
	return(!Is_Listed(brain.Member, brain.Members, who) && !Is_Listed(brain.Raider, brain.Raiders, who) &&
		!Is_Listed(brain.Repair, brain.Repairs, who) && brain.Chrono != who);
}


/*
**	Its combat units at home (infantry and vehicles), up to the space in the list.
*/
static int Home_Army(HouseClass const * house, BrainClass const & brain, FootClass ** list, int space, int & worth)
{
	int count = 0;
	worth = 0;

	for (int index = 0; index < Units.Count() && count < space; index++) {
		UnitClass * unit = Units.Ptr(index);
		if (unit != NULL && unit->IsActive && !unit->IsInLimbo && Is_Owned(unit, house) && Is_At_Home(brain, unit)) {
			list[count++] = unit;
			worth += Worth(unit);
		}
	}
	for (int index = 0; index < Infantry.Count() && count < space; index++) {
		InfantryClass * infantry = Infantry.Ptr(index);
		if (infantry != NULL && infantry->IsActive && !infantry->IsInLimbo && Is_Owned(infantry, house) && Is_At_Home(brain, infantry)) {
			list[count++] = infantry;
			worth += Worth(infantry);
		}
	}
	return(count);
}


/*
**	Send it to guard the place, with attack-move (taking on what it meets on the way) or not.
*/
static void Order_Move(FootClass * foot, CELL cell, bool attack)
{
	TARGET place = ::As_Target(cell);

	if (foot->Get_Mission() != MISSION_GUARD_AREA) {
		foot->Assign_Mission(MISSION_GUARD_AREA);
	}
	foot->ArchiveTarget = place;
	foot->Assign_Destination(place);
	Attack_Move_Set(foot, attack);
}


static void Order_Home(HouseClass const * house, FootClass * foot)
{
	if (foot->What_Am_I() == RTTI_VESSEL) {
		BuildingClass * yard = house->Find_Building(STRUCT_SHIP_YARD);
		if (yard == NULL) yard = house->Find_Building(STRUCT_SUB_PEN);
		CELL cell = Coord_Cell((yard != NULL) ? yard->Center_Coord() : foot->Center_Coord());
		Order_Move(foot, Map.Nearby_Location(cell, SPEED_FLOAT, Map[foot->Center_Coord()].Zones[MZONE_WATER], MZONE_WATER), false);
		return;
	}
	if (house->Center == 0) {
		Order_Move(foot, Coord_Cell(foot->Center_Coord()), false);
		return;
	}
	Order_Move(foot, house->Where_To_Go(foot), false);
}


/***********************************************************************************************
 * Building_Worth -- How much an enemy building is worth attacking.                            *
 *=============================================================================================*/
static int Building_Worth(BuildingClass const * building)
{
	/*
	**	Base defenses are better taken on the way, or left to the artillery.
	*/
	if (building->Class->PrimaryWeapon != NULL) return(6);

	switch (building->Class->Type) {
		case STRUCT_REFINERY:
			return(30);

		case STRUCT_CONST:
			return(28);

		case STRUCT_WEAP:
		case STRUCT_POWER:
		case STRUCT_ADVANCED_POWER:
			return(25);

		case STRUCT_ADVANCED_TECH:
		case STRUCT_SOVIET_TECH:
		case STRUCT_MSLO:
		case STRUCT_IRON_CURTAIN:
		case STRUCT_CHRONOSPHERE:
			return(22);

		case STRUCT_BARRACKS:
		case STRUCT_TENT:
		case STRUCT_RADAR:
		case STRUCT_HELIPAD:
		case STRUCT_AIRSTRIP:
		case STRUCT_REPAIR:
			return(15);

		case STRUCT_STORAGE:
			return(5);

		default:
			return(10);
	}
}


/*
**	Is it an enemy building worth attacking?
*/
static bool Is_Objective(HouseClass const * house, BuildingClass const * building)
{
	if (!Is_Enemy(house, building)) return(false);
	if (building->Class->IsInvisible || building->Class->IsWall || building->Class->IsInsignificant) return(false);
	return(true);
}


/*
**	The worth of the enemy's defensive buildings that cover the place (within their range,
**	and two cells).
*/
static int Defenses_Near(HouseClass const * house, COORDINATE coord)
{
	int worth = 0;
	for (int index = 0; index < Buildings.Count(); index++) {
		BuildingClass * building = Buildings.Ptr(index);
		if (building == NULL || building->Class->PrimaryWeapon == NULL || !Is_Enemy(house, building)) continue;
		if (Distance(coord, building->Center_Coord()) <= building->Class->PrimaryWeapon->Range + CELLS(2)) {
			worth += Worth(building);
		}
	}
	return(worth);
}


/*
**	The worth of a house's armed forces on the ground (not its harvesters).
*/
static int Army_Worth(HouseClass const * owner)
{
	int worth = 0;
	for (int index = 0; index < Units.Count(); index++) {
		UnitClass * unit = Units.Ptr(index);
		if (unit != NULL && unit->IsActive && !unit->IsInLimbo && Is_Owned(unit, owner) && unit->Is_Weapon_Equipped() && !unit->Class->IsToHarvest) {
			worth += Worth(unit);
		}
	}
	for (int index = 0; index < Infantry.Count(); index++) {
		InfantryClass * infantry = Infantry.Ptr(index);
		if (infantry != NULL && infantry->IsActive && !infantry->IsInLimbo && Is_Owned(infantry, owner) && infantry->Is_Weapon_Equipped()) {
			worth += Worth(infantry);
		}
	}
	return(worth);
}


/***********************************************************************************************
 * Pick_Objective -- Picks the enemy building to attack next.                                  *
 *                                                                                             *
 *    The most worth attacking, for its kind and nearness to the place given, that the house's *
 *    ground forces there can reach (zone, on the MZONE_NORMAL map), and the less defended the *
 *    better. The house's chosen enemy (HouseClass::Enemy) is preferred.                       *
 *=============================================================================================*/
static BuildingClass * Pick_Objective(HouseClass const * house, COORDINATE from, int zone)
{
	BuildingClass * best = NULL;
	int bestscore = 0;

	for (int index = 0; index < Buildings.Count(); index++) {
		BuildingClass * building = Buildings.Ptr(index);
		if (building == NULL || !Is_Objective(house, building)) continue;

		COORDINATE coord = building->Center_Coord();
		if (zone != 0 && Map[coord].Zones[MZONE_NORMAL] != zone) continue;

		int score = Building_Worth(building) * 4096 / (Distance(from, coord) / CELL_LEPTON_W + 8);
		score = score * 2000 / (2000 + Defenses_Near(house, coord));
		if (building->Owner() == house->Enemy) score *= 2;
		if (best == NULL || score > bestscore) {
			best = building;
			bestscore = score;
		}
	}
	return(best);
}


/*
**	The place to gather an attack: from the base, a third of the way to the target (6 to 14
**	cells out).
*/
static CELL Staging_Cell(HouseClass const * house, COORDINATE target, int zone)
{
	COORDINATE home = house->Center;
	int distance = Distance(home, target);
	int step = Bound(distance / 3, CELLS(6), CELLS(14));
	if (step > distance / 2) step = distance / 2;

	CELL cell = Coord_Cell(Coord_Move(home, Direction(home, target), step));
	return(Map.Nearby_Location(cell, SPEED_TRACK, (zone != 0) ? zone : -1, MZONE_NORMAL));
}


/***********************************************************************************************
 * Assess -- Sizes up the enemy's forces and its own.                                          *
 *=============================================================================================*/
static void Assess(HouseClass const * house, BrainClass & brain)
{
	brain.EnemyInfantry = 0;
	brain.EnemyVehicles = 0;
	brain.EnemyAircraft = 0;
	brain.EnemyShips = 0;
	brain.EnemyDefenses = 0;
	memset(brain.Own, 0, sizeof(brain.Own));
	brain.OwnInfantry = 0;
	brain.OwnTotal = 0;

	for (int index = 0; index < Infantry.Count(); index++) {
		InfantryClass * infantry = Infantry.Ptr(index);
		if (infantry == NULL || !infantry->IsActive || infantry->IsInLimbo) continue;
		if (Is_Owned(infantry, house)) {
			if (Is_Soldier(infantry)) {
				brain.Own[Role_Of(*infantry->Class)] += Worth(infantry);
				brain.OwnInfantry += Worth(infantry);
				brain.OwnTotal += Worth(infantry);
			}
		} else if (Is_Enemy(house, infantry)) {
			brain.EnemyInfantry += Worth(infantry);
		}
	}

	for (int index = 0; index < Units.Count(); index++) {
		UnitClass * unit = Units.Ptr(index);
		if (unit == NULL || !unit->IsActive || unit->IsInLimbo || unit->Class->IsToHarvest) continue;
		if (Is_Owned(unit, house)) {
			if (Is_Soldier(unit)) {
				brain.Own[Role_Of(*unit->Class)] += Worth(unit);
				brain.OwnTotal += Worth(unit);
			}
		} else if (Is_Enemy(house, unit)) {
			brain.EnemyVehicles += Worth(unit);
		}
	}

	for (int index = 0; index < Aircraft.Count(); index++) {
		AircraftClass * aircraft = Aircraft.Ptr(index);
		if (aircraft != NULL && Is_Enemy(house, aircraft)) {
			brain.EnemyAircraft += Worth(aircraft);
		}
	}

	for (int index = 0; index < Vessels.Count(); index++) {
		VesselClass * vessel = Vessels.Ptr(index);
		if (vessel != NULL && Is_Enemy(house, vessel)) {
			brain.EnemyShips += Worth(vessel);
		}
	}

	for (int index = 0; index < Buildings.Count(); index++) {
		BuildingClass * building = Buildings.Ptr(index);
		if (building != NULL && Is_Enemy(house, building)) {
			if (building->Class->PrimaryWeapon != NULL) {
				brain.EnemyDefenses += Worth(building);
			}

			/*
			**	Helipads and airfields mean aircraft to come.
			*/
			if (*building == STRUCT_HELIPAD || *building == STRUCT_AIRSTRIP) {
				brain.EnemyAircraft += 500;
			}
		}
	}
}


/***********************************************************************************************
 * Pick_Role -- Picks the role the next unit should have.                                      *
 *                                                                                             *
 *    The house aims for a mix of forces (by worth) that answers what the enemy has, and       *
 *    picks the role, among those it can build here (allowed), furthest below its share.       *
 *=============================================================================================*/
static RoleType Pick_Role(BrainClass const & brain, bool const allowed[ROLE_COUNT])
{
	PersonalityClass const & traits = Personalities[brain.Personality];
	int army = max(brain.EnemyInfantry + brain.EnemyVehicles + brain.EnemyAircraft, 1);

	int mix[ROLE_COUNT];
	memset(mix, 0, sizeof(mix));
	mix[ROLE_ANTI_ARMOR] = 50 + traits.ArmorBonus;
	mix[ROLE_ANTI_INFANTRY] = 20 + 40 * brain.EnemyInfantry / army;
	mix[ROLE_ARTILLERY] = 10 + ((brain.Personality == PERSONALITY_TURTLE) ? 10 : 0) + ((brain.EnemyDefenses > army / 2) ? 10 : 0);
	mix[ROLE_ANTI_AIR] = (brain.EnemyAircraft > 0) ? min(10 + 40 * brain.EnemyAircraft / army, 35) : 0;
	mix[ROLE_SUPPORT] = 4;

	int mixtotal = 0;
	for (int role = 0; role < ROLE_COUNT; role++) {
		if (allowed[role]) mixtotal += mix[role];
	}
	if (mixtotal == 0) return(ROLE_NONE);
	int owntotal = max(brain.OwnTotal, 1);

	RoleType best = ROLE_NONE;
	int bestgap = 0;
	for (int role = 0; role < ROLE_COUNT; role++) {
		if (!allowed[role] || mix[role] == 0) continue;
		int gap = mix[role] * 1000 / mixtotal - brain.Own[role] * 1000 / owntotal;
		if (best == ROLE_NONE || gap > bestgap) {
			best = (RoleType)role;
			bestgap = gap;
		}
	}
	return(best);
}


/*
**	How much to want each kind of unit in its role: the better ones are wanted more, but the
**	cheaper ones are still built now and then.
*/
static int Type_Weight(TechnoTypeClass const & type)
{
	return(4 + type.Raw_Cost() / 100);
}


/*
**	Short of harvesters and of money: the harvesters come first.
*/
static bool Is_Starved(HouseClass * house)
{
	int refineries = house->QuantityB(STRUCT_REFINERY);
	return(refineries > 0 && !house->IsTiberiumShort && house->QuantityU(UNIT_HARVESTER) < refineries && house->Available_Money() < 1500);
}


/***********************************************************************************************
 * Smart_AI_Unit -- Picks the vehicle for the house to build next (HouseClass::AI_Unit).       *
 *                                                                                             *
 * OUTPUT:  Was it the smarter computer player's choice (whether it picked anything or not)?   *
 *=============================================================================================*/
bool Smart_AI_Unit(HouseClass * house)
{
	BrainClass * brain = Brain_Of(house);
	if (brain == NULL) return(false);

	/*
	**	Without a construction yard, a new one first.
	*/
	if (house->QuantityB(STRUCT_CONST) == 0 && house->QuantityU(UNIT_MCV) == 0) {
		UnitTypeClass const & mcv = UnitTypeClass::As_Reference(UNIT_MCV);
		if (house->Can_Build(&mcv, house->ActLike)) {
			house->BuildUnit = UNIT_MCV;
			return(true);
		}
	}

	/*
	**	Harvesters next: two for each refinery (up to eight), while there's ore to be had.
	*/
	int refineries = house->QuantityB(STRUCT_REFINERY);
	int harvesters = house->QuantityU(UNIT_HARVESTER);
	if (refineries > 0 && !house->IsTiberiumShort && harvesters < min(refineries * 2, 8)) {
		UnitTypeClass const & harvester = UnitTypeClass::As_Reference(UNIT_HARVESTER);
		if (house->Can_Build(&harvester, house->ActLike) && (harvesters < refineries || house->Available_Money() > harvester.Cost_Of() + 500)) {
			house->BuildUnit = UNIT_HARVESTER;
			return(true);
		}
	}

	/*
	**	A combat vehicle, in the role most needed.
	*/
	bool allowed[ROLE_COUNT];
	memset(allowed, 0, sizeof(allowed));
	for (UnitType type = UNIT_FIRST; type < UNIT_COUNT; type++) {
		UnitTypeClass const & utype = UnitTypeClass::As_Reference(type);
		if (!utype.IsToHarvest && type != UNIT_MCV && house->Can_Build(&utype, house->ActLike)) {
			allowed[Role_Of(utype)] = true;
		}
	}
	allowed[ROLE_NONE] = false;
	RoleType role = Pick_Role(*brain, allowed);
	if (role == ROLE_NONE) return(true);

	int weight[UNIT_COUNT];
	int total = 0;
	for (UnitType type = UNIT_FIRST; type < UNIT_COUNT; type++) {
		UnitTypeClass const & utype = UnitTypeClass::As_Reference(type);
		weight[type] = 0;
		if (!utype.IsToHarvest && type != UNIT_MCV && Role_Of(utype) == role && house->Can_Build(&utype, house->ActLike)) {
			weight[type] = Type_Weight(utype);
			total += weight[type];
		}
	}
	if (total > 0) {
		int pick = Random_Pick(0, total-1);
		for (UnitType type = UNIT_FIRST; type < UNIT_COUNT; type++) {
			if (pick < weight[type]) {
				house->BuildUnit = type;
				break;
			}
			pick -= weight[type];
		}
	}
	return(true);
}


/***********************************************************************************************
 * Smart_AI_Infantry -- Picks the soldier for the house to train next (AI_Infantry).           *
 *=============================================================================================*/
bool Smart_AI_Infantry(HouseClass * house)
{
	BrainClass * brain = Brain_Of(house);
	if (brain == NULL) return(false);
	if (Is_Starved(house)) return(true);

	/*
	**	Keep to the personality's share of infantry, unless there's money to spare.
	*/
	PersonalityClass const & traits = Personalities[brain->Personality];
	if (house->QuantityB(STRUCT_WEAP) > 0 && house->CurInfantry >= 4 && brain->OwnInfantry * 100 > brain->OwnTotal * (traits.InfantryPercent + 10) && house->Available_Money() < 3000) {
		return(true);
	}

	bool allowed[ROLE_COUNT];
	memset(allowed, 0, sizeof(allowed));
	for (InfantryType type = INFANTRY_FIRST; type < INFANTRY_COUNT; type++) {
		InfantryTypeClass const & itype = InfantryTypeClass::As_Reference(type);
		if (!itype.IsCapture && !itype.IsCivilian && house->Can_Build(&itype, house->ActLike)) {
			allowed[Role_Of(itype)] = true;
		}
	}
	allowed[ROLE_NONE] = false;

	/*
	**	A medic for every eight soldiers or so.
	*/
	if (brain->Own[ROLE_SUPPORT] * 8 > brain->OwnInfantry) allowed[ROLE_SUPPORT] = false;

	RoleType role = Pick_Role(*brain, allowed);
	if (role == ROLE_NONE) return(true);

	int weight[INFANTRY_COUNT];
	int total = 0;
	for (InfantryType type = INFANTRY_FIRST; type < INFANTRY_COUNT; type++) {
		InfantryTypeClass const & itype = InfantryTypeClass::As_Reference(type);
		weight[type] = 0;
		if (itype.IsCapture || itype.IsCivilian || Role_Of(itype) != role || !house->Can_Build(&itype, house->ActLike)) continue;

		/*
		**	Only one hero (with C4) at a time, and a few dogs.
		*/
		if (itype.IsBomber && house->QuantityI(type) > 0) continue;
		if (itype.IsDog && house->QuantityI(type) >= 3) continue;

		weight[type] = Type_Weight(itype);
		total += weight[type];
	}
	if (total > 0) {
		int pick = Random_Pick(0, total-1);
		for (InfantryType type = INFANTRY_FIRST; type < INFANTRY_COUNT; type++) {
			if (pick < weight[type]) {
				house->BuildInfantry = type;
				break;
			}
			pick -= weight[type];
		}
	}
	return(true);
}


/***********************************************************************************************
 * Smart_AI_Vessel -- Picks the ship for the house to build next (AI_Vessel).                  *
 *=============================================================================================*/
bool Smart_AI_Vessel(HouseClass * house)
{
	BrainClass * brain = Brain_Of(house);
	if (brain == NULL) return(false);
	if (!brain->IsNavyWanted || house->CurVessels >= 10) return(true);

	int weight[VESSEL_COUNT];
	int total = 0;
	for (VesselType type = VESSEL_FIRST; type < VESSEL_COUNT; type++) {
		VesselTypeClass const & vtype = VesselTypeClass::As_Reference(type);
		weight[type] = 0;
		if (vtype.PrimaryWeapon == NULL || !house->Can_Build(&vtype, house->ActLike)) continue;

		/*
		**	Ships that can't hit the shore (submarines) are only any use against ships.
		*/
		bool shore = Firepower(vtype, ARMOR_WOOD, false) > 0;
		if (!shore && brain->EnemyShips == 0) continue;

		weight[type] = Type_Weight(vtype) * ((shore || brain->EnemyShips > 0) ? 2 : 1);
		total += weight[type];
	}
	if (total > 0) {
		int pick = Random_Pick(0, total-1);
		for (VesselType type = VESSEL_FIRST; type < VESSEL_COUNT; type++) {
			if (pick < weight[type]) {
				house->BuildVessel = type;
				break;
			}
			pick -= weight[type];
		}
	}
	return(true);
}


/***********************************************************************************************
 * Smart_AI_Buildings -- Adds its own choices of building, and picks one (AI_Building).        *
 *                                                                                             *
 *    The original's choices are in HouseClass::BuildChoice. To them it adds a radar dome      *
 *    early (the original only builds one against aircraft, and the technology center,         *
 *    helipads and airfields need it), a second and third refinery, a service depot, a naval   *
 *    yard when the enemy can be reached by sea, and the super weapons; it also builds more    *
 *    or fewer defenses, as its personality says. Then it picks one of the most urgent at       *
 *    random (the original took the first), power first when it's short.                      *
 *                                                                                             *
 * OUTPUT:  Was it the smarter computer player's choice?                                       *
 *=============================================================================================*/
static void Choose(HouseClass * house, StructType type, UrgencyType urgency, int money, bool hasincome)
{
	BuildingTypeClass const & btype = BuildingTypeClass::As_Reference(type);
	if (!house->Can_Build(&btype, house->ActLike)) return;
	if (btype.Cost_Of() > money && !hasincome) return;

	HouseClass::BuildChoiceClass * choice = HouseClass::BuildChoice.Alloc();
	if (choice != NULL) {
		*choice = HouseClass::BuildChoiceClass(urgency, type);
	}
}


static bool Is_Defense(StructType type)
{
	BuildingTypeClass const & btype = BuildingTypeClass::As_Reference(type);
	return(btype.PrimaryWeapon != NULL && !btype.IsWall);
}


/*
**	How much the house wants the building, among those as urgent: the essentials first.
*/
static int Building_Rank(HouseClass * house, BrainClass const & brain, StructType type, bool powered)
{
	switch (type) {
		case STRUCT_POWER:
		case STRUCT_ADVANCED_POWER:
			return(powered ? 20 : 60);

		case STRUCT_WEAP:
			return((house->QuantityB(STRUCT_WEAP) == 0) ? 55 : 20);

		case STRUCT_BARRACKS:
		case STRUCT_TENT:
			return((house->QuantityB(STRUCT_BARRACKS) + house->QuantityB(STRUCT_TENT) == 0) ? 50 : 15);

		case STRUCT_REFINERY:
			return((house->QuantityB(STRUCT_REFINERY) < 3) ? 45 : 27);

		case STRUCT_RADAR:
			return(40);

		case STRUCT_ADVANCED_TECH:
		case STRUCT_SOVIET_TECH:
			return(30);

		case STRUCT_SHIP_YARD:
		case STRUCT_SUB_PEN:
			return(18);

		case STRUCT_REPAIR:
			return(15);

		case STRUCT_MSLO:
		case STRUCT_IRON_CURTAIN:
		case STRUCT_CHRONOSPHERE:
			return(12);

		case STRUCT_HELIPAD:
		case STRUCT_AIRSTRIP:
			return(10);

		default:
			if (Is_Defense(type)) {
				return((brain.Personality == PERSONALITY_TURTLE) ? 26 : 20);
			}
			return(5);
	}
}


bool Smart_AI_Buildings(HouseClass * house, int money, bool hasincome)
{
	BrainClass * brain = Brain_Of(house);
	if (brain == NULL) return(false);
	PersonalityClass const & traits = Personalities[brain->Personality];

	int buildings = house->CurBuildings;
	bool powered = house->Power_Fraction() >= 1;

	/*
	**	The factories, before anything else.
	*/
	if (house->QuantityB(STRUCT_BARRACKS) + house->QuantityB(STRUCT_TENT) == 0 && buildings >= 2) {
		Choose(house, STRUCT_BARRACKS, URGENCY_HIGH, money, hasincome);
		Choose(house, STRUCT_TENT, URGENCY_HIGH, money, hasincome);
	}
	if (house->QuantityB(STRUCT_WEAP) == 0 && buildings >= 3) {
		Choose(house, STRUCT_WEAP, URGENCY_HIGH, money, hasincome);
	}

	/*
	**	A radar dome, early.
	*/
	if (house->QuantityB(STRUCT_RADAR) == 0 && buildings >= 5) {
		Choose(house, STRUCT_RADAR, URGENCY_HIGH, money, hasincome);
	}

	/*
	**	More refineries, while there's ore: a second one early, a third and a fourth as the
	**	base grows.
	*/
	if (!house->IsTiberiumShort) {
		int refineries = house->QuantityB(STRUCT_REFINERY);
		if (refineries == 1 && buildings >= 6) {
			Choose(house, STRUCT_REFINERY, URGENCY_HIGH, money, hasincome);
		}
		if ((refineries == 2 && buildings >= 12) || (refineries == 3 && buildings >= 18)) {
			Choose(house, STRUCT_REFINERY, URGENCY_MEDIUM, money, hasincome);
		}
	}

	/*
	**	A service depot.
	*/
	if (house->QuantityB(STRUCT_REPAIR) == 0 && house->QuantityB(STRUCT_WEAP) > 0 && buildings >= 9) {
		Choose(house, STRUCT_REPAIR, URGENCY_MEDIUM, money, hasincome);
	}

	/*
	**	A naval yard, when there's an enemy to reach by sea.
	*/
	if (brain->IsNavyWanted && house->QuantityB(STRUCT_SHIP_YARD) + house->QuantityB(STRUCT_SUB_PEN) == 0) {
		UrgencyType urgency = (brain->EnemyShips > 0) ? URGENCY_HIGH : URGENCY_MEDIUM;
		Choose(house, STRUCT_SHIP_YARD, urgency, money, hasincome);
		Choose(house, STRUCT_SUB_PEN, urgency, money, hasincome);
	}

	/*
	**	The super weapons, for a big base with money to spare.
	*/
	if (buildings >= 12 && powered && money > 2500) {
		static StructType const _super[] = {STRUCT_MSLO, STRUCT_IRON_CURTAIN, STRUCT_CHRONOSPHERE};
		for (int index = 0; index < ARRAY_SIZE(_super); index++) {
			if (house->QuantityB(_super[index]) == 0) {
				Choose(house, _super[index], URGENCY_MEDIUM, money, hasincome);
			}
		}
	}

	/*
	**	The defenses, as the personality says.
	*/
	int defenses = 0;
	for (StructType type = STRUCT_FIRST; type < STRUCT_BARREL; type++) {
		if (Is_Defense(type)) defenses += house->QuantityB(type);
	}
	bool more_defenses = defenses * 100 < buildings * traits.DefensePercent;
	if (more_defenses && traits.DefensePercent > 40) {
		static StructType const _defense[] = {STRUCT_TESLA, STRUCT_FLAME_TURRET, STRUCT_TURRET, STRUCT_PILLBOX};
		for (int index = 0; index < ARRAY_SIZE(_defense); index++) {
			if (_defense[index] == STRUCT_TESLA && !powered) continue;
			if (house->Can_Build(&BuildingTypeClass::As_Reference(_defense[index]), house->ActLike)) {
				Choose(house, _defense[index], URGENCY_MEDIUM, money, hasincome);
				break;
			}
		}
	}

	/*
	**	Pick the most urgent. Among those as urgent, the essentials first: power when it's
	**	short, the factories, refineries, the radar dome, the technology center (the original
	**	took the first in its list). Helipads and airfields wait until those are up (the
	**	original made them urgent whenever the enemy had more aircraft). A personality that
	**	wants fewer defenses than the original puts off those beyond its share.
	*/
	bool starved = Is_Starved(house);
	int bestrank = -1;
	StructType pick = STRUCT_NONE;
	for (int index = 0; index < HouseClass::BuildChoice.Count(); index++) {
		HouseClass::BuildChoiceClass const * choice = HouseClass::BuildChoice.Ptr(index);
		StructType type = choice->Structure;
		UrgencyType urgency = choice->Urgency;

		/*
		**	Short of harvesters and money: only power and what makes money.
		*/
		if (starved && type != STRUCT_POWER && type != STRUCT_ADVANCED_POWER && type != STRUCT_REFINERY && type != STRUCT_WEAP) {
			continue;
		}

		if ((type == STRUCT_POWER || type == STRUCT_ADVANCED_POWER) && !powered && urgency < URGENCY_CRITICAL) {
			urgency++;
		}
		if ((type == STRUCT_HELIPAD || type == STRUCT_AIRSTRIP) && urgency > URGENCY_MEDIUM) {
			urgency = URGENCY_MEDIUM;
		}
		if (Is_Defense(type) && !more_defenses && urgency > URGENCY_LOW) {
			urgency = URGENCY_LOW;
		}
		if (urgency == URGENCY_NONE) continue;

		int rank = (int)urgency * 100 + Building_Rank(house, *brain, type, powered);
		if (rank > bestrank) {
			bestrank = rank;
			pick = type;
		}
	}
	if (pick != STRUCT_NONE) {
		house->BuildStructure = pick;
	}
	return(true);
}


/***********************************************************************************************
 * Wave_AI -- Gathers the forces, and attacks with them.                                       *
 *=============================================================================================*/
static void End_Wave(BrainClass & brain)
{
	brain.Wave = WAVE_NONE;
	brain.Members = 0;
	brain.Objective = TARGET_NONE;
	brain.LastWave = Frame;
}


static void Wave_Home(HouseClass const * house, BrainClass & brain)
{
	for (int index = 0; index < brain.Members; index++) {
		FootClass * foot = Unit_Of(brain.Member[index], house);
		if (foot != NULL) Order_Home(house, foot);
	}
	End_Wave(brain);
}


/*
**	The armed aircraft join the attack.
*/
static void Air_Strike(HouseClass const * house, TARGET target)
{
	for (int index = 0; index < Aircraft.Count(); index++) {
		AircraftClass * aircraft = Aircraft.Ptr(index);
		if (aircraft == NULL || !aircraft->IsActive || aircraft->IsInLimbo || !Is_Owned(aircraft, house)) continue;
		if (aircraft->Team.Is_Valid() || !aircraft->Is_Weapon_Equipped()) continue;
		if (aircraft->Get_Mission() == MISSION_ATTACK || aircraft->Get_Mission() == MISSION_HUNT) continue;

		aircraft->Assign_Mission(MISSION_ATTACK);
		aircraft->Assign_Target(target);
	}
}


static BuildingClass * Valid_Objective(HouseClass const * house, TARGET target)
{
	BuildingClass * building = As_Building(target);
	if (building != NULL && building->IsActive && Is_Objective(house, building)) return(building);
	return(NULL);
}


static void Wave_AI(HouseClass * house, BrainClass & brain)
{
	PersonalityClass const & traits = Personalities[brain.Personality];
	FootClass * home[MAX_WAVE];
	int worth = 0;

	switch (brain.Wave) {

		/*
		**	Building up: when there are enough at home, gather them for an attack.
		*/
		case WAVE_NONE:
		{
			/*
			**	Not while the base is being attacked.
			*/
			if (Frame - house->LATime < TICKS_PER_SECOND * 20 || house->Center == 0) break;
			if (Frame < traits.FirstWaveTime * TICKS_PER_MINUTE) break;

			/*
			**	Enough at home for the next attack? Or, after six minutes without one, at least
			**	half as many.
			*/
			int count = Home_Army(house, brain, home, MAX_WAVE, worth);
			int want = min(traits.FirstWave + traits.WaveGrowth * brain.Waves, traits.MaxWave);
			bool overdue = (Frame - brain.LastWave > TICKS_PER_MINUTE * 6 && worth >= want / 2);
			if (count < 3 || (worth < want && !overdue)) break;

			int zone = Map[home[0]->Center_Coord()].Zones[MZONE_NORMAL];
			BuildingClass * target = Pick_Objective(house, house->Center, zone);
			if (target == NULL) break;

			/*
			**	Only when its forces outweigh the enemy's, and the defenses around the target,
			**	by as much as its personality dares (less as time goes by without an attack).
			*/
			int daring = traits.Daring;
			if (Frame - brain.LastWave > TICKS_PER_MINUTE * 8) daring = daring * 3 / 4;
			int enemy = Army_Worth(target->House) + Defenses_Near(house, target->Center_Coord());
			if (worth * 100 < enemy * daring) break;

			/*
			**	One in six stays at home.
			*/
			brain.Members = 0;
			for (int index = count / 6; index < count; index++) {
				brain.Member[brain.Members++] = home[index]->As_Target();
			}
			brain.Objective = target->As_Target();
			brain.Staging = Staging_Cell(house, target->Center_Coord(), zone);
			for (int index = 0; index < brain.Members; index++) {
				FootClass * foot = Unit_Of(brain.Member[index], house);
				if (foot != NULL) Order_Move(foot, brain.Staging, false);
			}
			brain.Wave = WAVE_GATHER;
			brain.PhaseStart = Frame;
			break;
		}

		/*
		**	Gathering: when most are there (or after a minute and a half), attack.
		*/
		case WAVE_GATHER:
		{
			/*
			**	Those built in the meantime join in (still leaving one in six at home).
			*/
			int count = Home_Army(house, brain, home, MAX_WAVE - brain.Members, worth);
			for (int index = (brain.Members + count) / 6; index < count; index++) {
				brain.Member[brain.Members++] = home[index]->As_Target();
				Order_Move(home[index], brain.Staging, false);
			}
			if (brain.Members == 0) {
				End_Wave(brain);
				break;
			}

			int total = 0;
			int present = 0;
			for (int index = 0; index < brain.Members; index++) {
				FootClass * foot = Unit_Of(brain.Member[index], house);
				if (foot == NULL) continue;
				total += Worth(foot);
				if (Distance(foot->Center_Coord(), Cell_Coord(brain.Staging)) < CELLS(6)) {
					present += Worth(foot);
				}
			}
			if (present * 4 < total * 3 && Frame - brain.PhaseStart < TICKS_PER_SECOND * 90) break;

			int zone = Map[brain.Staging].Zones[MZONE_NORMAL];
			BuildingClass * target = Valid_Objective(house, brain.Objective);
			if (target == NULL) target = Pick_Objective(house, Cell_Coord(brain.Staging), zone);
			if (target == NULL) {
				Wave_Home(house, brain);
				break;
			}
			brain.Objective = target->As_Target();
			brain.StartValue = total;
			brain.Wave = WAVE_ATTACK;
			brain.PhaseStart = Frame;
			brain.Waves++;

			CELL cell = Coord_Cell(target->Center_Coord());
			for (int index = 0; index < brain.Members; index++) {
				FootClass * foot = Unit_Of(brain.Member[index], house);
				if (foot != NULL) Order_Move(foot, cell, true);
			}
			Air_Strike(house, brain.Objective);
			break;
		}

		/*
		**	On the attack: building after building, until too weakened or out of time.
		*/
		case WAVE_ATTACK:
		{
			if (brain.Members == 0) {
				End_Wave(brain);
				break;
			}

			int total = 0;
			int x = 0;
			int y = 0;
			int count = 0;
			for (int index = 0; index < brain.Members; index++) {
				FootClass * foot = Unit_Of(brain.Member[index], house);
				if (foot == NULL) continue;
				total += Worth(foot);
				x += Coord_X(foot->Center_Coord());
				y += Coord_Y(foot->Center_Coord());
				count++;
			}
			if (count == 0) {
				End_Wave(brain);
				break;
			}
			COORDINATE center = XY_Coord(x / count, y / count);

			BuildingClass * target = Valid_Objective(house, brain.Objective);
			bool close = (target != NULL && Distance(center, target->Center_Coord()) < CELLS(8));

			/*
			**	Too weak to go on: back home, to join the next attack. After eight minutes,
			**	the rest come home as well.
			*/
			if ((total * 10 < brain.StartValue * 3 && !close) || Frame - brain.PhaseStart > TICKS_PER_MINUTE * 8) {
				Wave_Home(house, brain);
				break;
			}

			if (target == NULL) {
				target = Pick_Objective(house, center, Map[center].Zones[MZONE_NORMAL]);
				if (target == NULL) target = Pick_Objective(house, center, 0);
				if (target == NULL) {

					/*
					**	Nothing left to attack: hunt down the rest.
					*/
					for (int index = 0; index < brain.Members; index++) {
						FootClass * foot = Unit_Of(brain.Member[index], house);
						if (foot != NULL) {
							Attack_Move_Set(foot, false);
							foot->Assign_Mission(MISSION_HUNT);
						}
					}
					End_Wave(brain);
					break;
				}
				brain.Objective = target->As_Target();
				Air_Strike(house, brain.Objective);
			}

			/*
			**	Those that stopped (after a fight elsewhere) go on to the target; those near it
			**	with nothing else to shoot at, shoot at it.
			*/
			CELL cell = Coord_Cell(target->Center_Coord());
			TARGET place = ::As_Target(cell);
			for (int index = 0; index < brain.Members; index++) {
				FootClass * foot = Unit_Of(brain.Member[index], house);
				if (foot == NULL || !Is_Idle(foot)) continue;

				if (foot->ArchiveTarget != place || !Is_Attack_Moving(foot)) {
					Order_Move(foot, cell, true);
				} else if (!Target_Legal(foot->TarCom) && Distance(foot->Center_Coord(), target->Center_Coord()) < CELLS(6)) {
					foot->Assign_Target(brain.Objective);
				}
			}
			break;
		}
	}
}


/***********************************************************************************************
 * Raid_AI -- Sends a few fast vehicles after a harvester out in the field.                    *
 *=============================================================================================*/
static void Raid_AI(HouseClass * house, BrainClass & brain)
{
	PersonalityClass const & traits = Personalities[brain.Personality];

	if (brain.Raiders > 0) {
		TechnoClass * prey = As_Techno(brain.Prey);
		if (prey == NULL || !Is_Enemy(house, prey) || Frame - brain.RaidStart > TICKS_PER_MINUTE * 2) {
			for (int index = 0; index < brain.Raiders; index++) {
				FootClass * foot = Unit_Of(brain.Raider[index], house);
				if (foot != NULL) Order_Home(house, foot);
			}
			brain.Raiders = 0;
			brain.Prey = TARGET_NONE;
			brain.NextRaid = Frame + TICKS_PER_MINUTE * traits.RaidDelay;
		} else {
			for (int index = 0; index < brain.Raiders; index++) {
				FootClass * foot = Unit_Of(brain.Raider[index], house);
				if (foot != NULL && foot->TarCom != brain.Prey) {
					Attack_Move_Set(foot, false);
					foot->Assign_Mission(MISSION_ATTACK);
					foot->Assign_Target(brain.Prey);
				}
			}
		}
		return;
	}

	if (Frame < brain.NextRaid) return;
	brain.NextRaid = Frame + TICKS_PER_SECOND * 30;

	/*
	**	The fastest vehicles at home (as fast as a heavy tank, or faster).
	*/
	FootClass * home[MAX_WAVE];
	int worth = 0;
	int count = Home_Army(house, brain, home, MAX_WAVE, worth);
	FootClass * raider[MAX_RAIDERS];
	int raiders = 0;
	for (int index = 0; index < count && raiders < MAX_RAIDERS; index++) {
		FootClass * foot = home[index];
		if (foot->What_Am_I() != RTTI_UNIT || Role_Of(*foot->Techno_Type_Class()) == ROLE_ARTILLERY) continue;
		if (foot->Techno_Type_Class()->MaxSpeed < UnitTypeClass::As_Reference(UNIT_MTANK).MaxSpeed) continue;
		if (foot->Health_Ratio() < fixed(3, 4)) continue;
		raider[raiders++] = foot;
	}
	if (raiders < 2) return;

	/*
	**	The nearest harvester away from its base.
	*/
	int zone = Map[raider[0]->Center_Coord()].Zones[MZONE_NORMAL];
	UnitClass * best = NULL;
	int bestdistance = 0;
	for (int index = 0; index < Units.Count(); index++) {
		UnitClass * unit = Units.Ptr(index);
		if (unit == NULL || !unit->Class->IsToHarvest || !Is_Enemy(house, unit)) continue;
		if (Map[unit->Center_Coord()].Zones[MZONE_NORMAL] != zone) continue;

		HouseClass const * owner = unit->House;
		if (owner->Center != 0 && Distance(unit->Center_Coord(), owner->Center) < owner->Radius + CELLS(6)) continue;

		int distance = Distance(house->Center, unit->Center_Coord());
		if (best == NULL || distance < bestdistance) {
			best = unit;
			bestdistance = distance;
		}
	}
	if (best == NULL) return;

	brain.Prey = best->As_Target();
	brain.RaidStart = Frame;
	brain.RaidCount++;
	for (int index = 0; index < raiders; index++) {
		brain.Raider[brain.Raiders++] = raider[index]->As_Target();
		Attack_Move_Set(raider[index], false);
		raider[index]->Assign_Mission(MISSION_ATTACK);
		raider[index]->Assign_Target(brain.Prey);
	}
}


/***********************************************************************************************
 * Navy_AI -- Looks for an enemy to reach by sea, and sends out the fleet.                     *
 *=============================================================================================*/
static bool Is_Sea(CELL cell, int sea)
{
	return(Map.In_Radar(cell) && Map[cell].Zones[MZONE_WATER] == sea);
}


/*
**	The cell of the sea nearest to the place, within the distance (cells), or 0.
*/
static CELL Sea_Near(COORDINATE coord, int sea, int reach)
{
	CELL center = Coord_Cell(coord);
	int cx = Cell_X(center);
	int cy = Cell_Y(center);

	for (int radius = 0; radius <= reach; radius++) {
		for (int y = cy - radius; y <= cy + radius; y++) {
			for (int x = cx - radius; x <= cx + radius; x++) {
				if (abs(x - cx) != radius && abs(y - cy) != radius) continue;
				if (x < 0 || y < 0 || x >= MAP_CELL_W || y >= MAP_CELL_H) continue;
				CELL cell = XY_Cell(x, y);
				if (Is_Sea(cell, sea)) return(cell);
			}
		}
	}
	return(0);
}


/*
**	Is there a place for a naval yard in the base? The sea it would be on, or 0.
*/
static int Yard_Sea(HouseClass const * house)
{
	StructType yard = STRUCT_NONE;
	if (house->Can_Build(&BuildingTypeClass::As_Reference(STRUCT_SHIP_YARD), house->ActLike)) {
		yard = STRUCT_SHIP_YARD;
	} else if (house->Can_Build(&BuildingTypeClass::As_Reference(STRUCT_SUB_PEN), house->ActLike)) {
		yard = STRUCT_SUB_PEN;
	}
	if (yard == STRUCT_NONE || house->Center == 0) return(0);

	BuildingTypeClass const & btype = BuildingTypeClass::As_Reference(yard);
	short const * list = btype.Occupy_List(true);
	CELL center = Coord_Cell(house->Center);
	int reach = Bound(house->Radius * 3 / CELL_LEPTON_W, 6, 20);

	for (int y = Cell_Y(center) - reach; y <= Cell_Y(center) + reach; y++) {
		for (int x = Cell_X(center) - reach; x <= Cell_X(center) + reach; x++) {
			if (x < 0 || y < 0 || x >= MAP_CELL_W || y >= MAP_CELL_H) continue;
			CELL cell = XY_Cell(x, y);
			if (!Map.In_Radar(cell) || house->Which_Zone(cell) == ZONE_NONE) continue;
			if (Map[cell].Land_Type() != LAND_WATER) continue;
			if (!btype.Legal_Placement(cell) || !Map.Passes_Proximity_Check(&btype, house->Class->House, list, cell)) continue;
			return(Map[cell].Zones[MZONE_WATER]);
		}
	}
	return(0);
}


static void Navy_Check(HouseClass * house, BrainClass & brain)
{
	if (Frame < brain.NavyCheck) return;
	brain.NavyCheck = Frame + TICKS_PER_MINUTE;

	/*
	**	The sea the naval yard is on, or would be.
	*/
	int sea = 0;
	BuildingClass * yard = house->Find_Building(STRUCT_SHIP_YARD);
	if (yard == NULL) yard = house->Find_Building(STRUCT_SUB_PEN);
	if (yard != NULL) {
		sea = Map[yard->Center_Coord()].Zones[MZONE_WATER];
	} else if (house->CurBuildings >= 8) {
		sea = Yard_Sea(house);
	}
	brain.Sea = sea;
	brain.IsNavyWanted = false;
	if (sea == 0) return;

	/*
	**	Are there enemy ships on that sea, or enemy buildings on its shores?
	*/
	for (int index = 0; index < Vessels.Count(); index++) {
		VesselClass * vessel = Vessels.Ptr(index);
		if (vessel != NULL && Is_Enemy(house, vessel) && Map[vessel->Center_Coord()].Zones[MZONE_WATER] == sea) {
			brain.IsNavyWanted = true;
			return;
		}
	}
	for (int index = 0; index < Buildings.Count(); index++) {
		BuildingClass * building = Buildings.Ptr(index);
		if (building != NULL && Is_Objective(house, building) && Sea_Near(building->Center_Coord(), sea, 4) != 0) {
			brain.IsNavyWanted = true;
			return;
		}
	}
}


/*
**	What the fleet should attack: an enemy ship on its sea, or a building near the shore. The
**	place to go to (on the sea) is put in cell.
*/
static TechnoClass * Pick_Naval_Objective(HouseClass const * house, COORDINATE from, int sea, CELL & cell)
{
	TechnoClass * best = NULL;
	int bestscore = 0;

	for (int index = 0; index < Vessels.Count(); index++) {
		VesselClass * vessel = Vessels.Ptr(index);
		if (vessel == NULL || !Is_Enemy(house, vessel)) continue;
		CELL where = Coord_Cell(vessel->Center_Coord());
		if (Map[where].Zones[MZONE_WATER] != sea) continue;

		int score = 40 * 4096 / (Distance(from, vessel->Center_Coord()) / CELL_LEPTON_W + 8);
		if (best == NULL || score > bestscore) {
			best = vessel;
			bestscore = score;
			cell = where;
		}
	}

	for (int index = 0; index < Buildings.Count(); index++) {
		BuildingClass * building = Buildings.Ptr(index);
		if (building == NULL || !Is_Objective(house, building)) continue;
		CELL where = Sea_Near(building->Center_Coord(), sea, 4);
		if (where == 0) continue;

		int score = Building_Worth(building) * 4096 / (Distance(from, building->Center_Coord()) / CELL_LEPTON_W + 8);
		if (best == NULL || score > bestscore) {
			best = building;
			bestscore = score;
			cell = where;
		}
	}
	return(best);
}


static void Navy_AI(HouseClass * house, BrainClass & brain)
{
	Navy_Check(house, brain);

	/*
	**	The fleet at sea: on to the next target when the last is gone; home after ten minutes.
	*/
	if (brain.Ships > 0) {
		FootClass * first = Unit_Of(brain.Ship[0], house);
		TechnoClass * target = As_Techno(brain.FleetObjective);
		if (target == NULL || !Is_Enemy(house, target)) {
			target = (first != NULL) ? Pick_Naval_Objective(house, first->Center_Coord(), Map[first->Center_Coord()].Zones[MZONE_WATER], brain.FleetCell) : NULL;
			brain.FleetObjective = (target != NULL) ? target->As_Target() : TARGET_NONE;
		}
		if (target == NULL || Frame - brain.FleetStart > TICKS_PER_MINUTE * 10) {
			for (int index = 0; index < brain.Ships; index++) {
				FootClass * foot = Unit_Of(brain.Ship[index], house);
				if (foot != NULL) Order_Home(house, foot);
			}
			brain.Ships = 0;
			brain.FleetObjective = TARGET_NONE;
			return;
		}

		TARGET place = ::As_Target(brain.FleetCell);
		for (int index = 0; index < brain.Ships; index++) {
			FootClass * foot = Unit_Of(brain.Ship[index], house);
			if (foot != NULL && Is_Idle(foot) && (foot->ArchiveTarget != place || !Is_Attack_Moving(foot))) {
				Order_Move(foot, brain.FleetCell, true);
			}
		}
		return;
	}

	/*
	**	The ships in port: three or more set out.
	*/
	FootClass * port[MAX_FLEET];
	int count = 0;
	for (int index = 0; index < Vessels.Count() && count < MAX_FLEET; index++) {
		VesselClass * vessel = Vessels.Ptr(index);
		if (vessel == NULL || !vessel->IsActive || vessel->IsInLimbo || !Is_Owned(vessel, house)) continue;
		if (vessel->Team.Is_Valid() || !vessel->Is_Weapon_Equipped() || !Is_Idle(vessel)) continue;
		port[count++] = vessel;
	}
	if (count < 3) return;

	int sea = Map[port[0]->Center_Coord()].Zones[MZONE_WATER];
	TechnoClass * target = Pick_Naval_Objective(house, port[0]->Center_Coord(), sea, brain.FleetCell);
	if (target == NULL) return;

	brain.FleetObjective = target->As_Target();
	brain.FleetStart = Frame;
	brain.FleetCount++;
	for (int index = 0; index < count; index++) {
		if (Map[port[index]->Center_Coord()].Zones[MZONE_WATER] != sea) continue;
		brain.Ship[brain.Ships++] = port[index]->As_Target();
		Order_Move(port[index], brain.FleetCell, true);
	}
}


/***********************************************************************************************
 * Repair_AI -- Sends damaged vehicles to the service depot.                                   *
 *=============================================================================================*/
static void Repair_AI(HouseClass * house, BrainClass & brain)
{
	BuildingClass * depot = house->Find_Building(STRUCT_REPAIR);

	/*
	**	Those repaired (or that gave up) go back to their posts.
	*/
	for (int index = 0; index < brain.Repairs; index++) {
		FootClass * foot = Unit_Of(brain.Repair[index], house);
		if (foot == NULL) continue;
		if (depot == NULL || foot->Health_Ratio() >= 1 || (foot->Get_Mission() != MISSION_ENTER && !foot->In_Radio_Contact())) {
			Order_Home(house, foot);
			brain.Repairs = Remove(brain.Repair, brain.Repairs, brain.Repair[index]);
			index--;
		}
	}
	if (depot == NULL || brain.Repairs >= MAX_REPAIRS) return;

	/*
	**	The most damaged vehicle at home, under half strength.
	*/
	UnitClass * worst = NULL;
	for (int index = 0; index < Units.Count(); index++) {
		UnitClass * unit = Units.Ptr(index);
		if (unit == NULL || !unit->IsActive || unit->IsInLimbo || !Is_Owned(unit, house)) continue;
		if (!Is_At_Home(brain, unit) || unit->Health_Ratio() >= fixed::_1_2) continue;
		if (Map[unit->Center_Coord()].Zones[MZONE_NORMAL] != Map[depot->Center_Coord()].Zones[MZONE_NORMAL]) continue;
		if (worst == NULL || unit->Health_Ratio() < worst->Health_Ratio()) {
			worst = unit;
		}
	}
	if (worst != NULL) {
		Attack_Move_Set(worst, false);
		worst->Assign_Mission(MISSION_ENTER);
		worst->Assign_Target(TARGET_NONE);
		worst->Assign_Destination(depot->As_Target());
		brain.Repair[brain.Repairs++] = worst->As_Target();
		brain.RepairCount++;
	}
}


/***********************************************************************************************
 * Super_AI -- Uses the iron curtain and the chronosphere.                                     *
 *                                                                                             *
 *    (The original computer player already fires the others: the nuclear missile, the        *
 *    paratroopers, the parabombs, the spy plane, and the support powers.)                     *
 *=============================================================================================*/
static void Super_AI(HouseClass * house, BrainClass & brain)
{
	/*
	**	The iron curtain: on the best tank of an attack, as it reaches the target.
	*/
	if (house->SuperWeapon[SPC_IRON_CURTAIN].Is_Ready() && brain.Wave == WAVE_ATTACK) {
		BuildingClass * target = Valid_Objective(house, brain.Objective);
		TechnoClass * best = NULL;
		for (int index = 0; index < brain.Members && target != NULL; index++) {
			FootClass * foot = Unit_Of(brain.Member[index], house);
			if (foot == NULL || foot->What_Am_I() != RTTI_UNIT || foot->Health_Ratio() < fixed::_1_2) continue;
			if (Distance(foot->Center_Coord(), target->Center_Coord()) > CELLS(10)) continue;
			if (best == NULL || Worth(foot) > Worth(best)) best = foot;
		}
		if (best != NULL) {
			best->IronCurtainCountDown = Rule.IronCurtainDuration * TICKS_PER_MINUTE;
			best->Mark(MARK_CHANGE);
			Sound_Effect(VOC_IRON1, best->Center_Coord());
			house->SuperWeapon[SPC_IRON_CURTAIN].Discharged(house == PlayerPtr);
			house->IsRecalcNeeded = true;
			brain.CurtainCount++;
		}
	}

	/*
	**	The one sent by the chronosphere comes back after a while (ChronoDuration), and then
	**	goes back to its post.
	*/
	if (brain.Chrono != TARGET_NONE && Frame - brain.ChronoStart > Rule.ChronoDuration * TICKS_PER_MINUTE + TICKS_PER_SECOND * 5) {
		FootClass * foot = Unit_Of(brain.Chrono, house);
		if (foot != NULL) Order_Home(house, foot);
		brain.Chrono = TARGET_NONE;
	}

	/*
	**	The chronosphere: the best tank at home, into the enemy's base, next to the building
	**	most worth attacking.
	*/
	if (house->SuperWeapon[SPC_CHRONOSPHERE].Is_Ready() && brain.Chrono == TARGET_NONE) {
		FootClass * home[MAX_WAVE];
		int worth = 0;
		int count = Home_Army(house, brain, home, MAX_WAVE, worth);
		FootClass * best = NULL;
		for (int index = 0; index < count; index++) {
			FootClass * foot = home[index];
			if (foot->What_Am_I() != RTTI_UNIT || foot->Health_Ratio() < fixed(3, 4)) continue;
			if (*(UnitClass *)foot == UNIT_CHRONOTANK || ((UnitClass *)foot)->IsDeploying) continue;
			if (Role_Of(*foot->Techno_Type_Class()) != ROLE_ANTI_ARMOR) continue;
			if (best == NULL || Worth(foot) > Worth(best)) best = foot;
		}
		HouseClass const * enemy = (house->Enemy != HOUSE_NONE) ? HouseClass::As_Pointer(house->Enemy) : NULL;
		if (best != NULL && enemy != NULL && enemy->Center != 0) {
			BuildingClass * target = Pick_Objective(house, enemy->Center, 0);
			if (target != NULL) {
				CELL cell = Map.Nearby_Location(Coord_Cell(target->Center_Coord()), best->Techno_Type_Class()->Speed);
				if (cell != 0 && Map[cell].Is_Clear_To_Move(best->Techno_Type_Class()->Speed, false, false)) {
					house->UnitToTeleport = best->As_Target();
					house->Place_Special_Blast(SPC_CHRONO2, cell);
					brain.Chrono = best->As_Target();
					brain.ChronoStart = Frame;
					brain.ChronoCount++;
					Order_Move(best, cell, true);
				}
			}
		}
	}
}


/***********************************************************************************************
 * Smart_AI_Harvester_Attacked -- An enemy fired on one of its harvesters (UnitClass::Take_Damage). *
 * Defend_AI -- Sends the nearest forces at home after it.                                     *
 *                                                                                             *
 *    The original only calls for help when a building is attacked                            *
 *    (TechnoClass::Base_Is_Attacked), and its harvesters are easy prey.                      *
 *=============================================================================================*/
void Smart_AI_Harvester_Attacked(UnitClass const * harvester, TechnoClass const * source)
{
	if (source == NULL) return;
	BrainClass * brain = Brain_Of(harvester->House);
	if (brain == NULL || !Is_Enemy(harvester->House, source)) return;

	if (brain->Intruder != source->As_Target()) {
		brain->Intruder = source->As_Target();
		brain->IsIntruderMet = false;
	}
	brain->IntruderFrame = Frame;
}


static void Defend_AI(HouseClass * house, BrainClass & brain)
{
	if (brain.Intruder == TARGET_NONE) return;

	TechnoClass * intruder = As_Techno(brain.Intruder);
	if (intruder == NULL || !Is_Enemy(house, intruder) || Frame - brain.IntruderFrame > TICKS_PER_SECOND * 15) {
		brain.Intruder = TARGET_NONE;
		return;
	}
	if (brain.IsIntruderMet) return;
	brain.IsIntruderMet = true;

	/*
	**	The nearest at home that can hurt it, twice its worth or so.
	*/
	FootClass * home[MAX_WAVE];
	int worth = 0;
	int count = Home_Army(house, brain, home, MAX_WAVE, worth);
	int zone = Map[intruder->Center_Coord()].Zones[MZONE_NORMAL];
	ArmorType armor = intruder->Techno_Type_Class()->Armor;
	int want = Worth(intruder) * 2 + 500;
	int sent = 0;
	while (sent < want) {
		int best = -1;
		int bestdistance = 0;
		for (int index = 0; index < count; index++) {
			FootClass * foot = home[index];
			if (foot == NULL) continue;
			WeaponTypeClass const * weapon = foot->Techno_Type_Class()->PrimaryWeapon;
			if (weapon == NULL || weapon->WarheadPtr == NULL || weapon->WarheadPtr->Modifier[armor] == 0) continue;
			if (Map[foot->Center_Coord()].Zones[MZONE_NORMAL] != zone) continue;

			int distance = Distance(foot->Center_Coord(), intruder->Center_Coord());
			if (distance > CELLS(30)) continue;
			if (best == -1 || distance < bestdistance) {
				best = index;
				bestdistance = distance;
			}
		}
		if (best == -1) break;

		FootClass * foot = home[best];
		home[best] = NULL;
		Attack_Move_Set(foot, false);
		foot->Assign_Mission(MISSION_ATTACK);
		foot->Assign_Target(brain.Intruder);
		sent += Worth(foot);
	}
}


/***********************************************************************************************
 * Smart_AI -- The smarter computer player's turn (HouseClass::AI, every game frame).          *
 *=============================================================================================*/
void Smart_AI(HouseClass * house)
{
	BrainClass * brain = Brain_Of(house);
	if (brain == NULL || house->IsDefeated) return;

	/*
	**	A unit the factory can no longer make would hold up its production for good.
	*/
	if (house->BuildUnit != UNIT_NONE && !house->Can_Build(&UnitTypeClass::As_Reference(house->BuildUnit), house->ActLike)) {
		house->BuildUnit = UNIT_NONE;
	}
	if (house->BuildInfantry != INFANTRY_NONE && !house->Can_Build(&InfantryTypeClass::As_Reference(house->BuildInfantry), house->ActLike)) {
		house->BuildInfantry = INFANTRY_NONE;
	}
	if (house->BuildVessel != VESSEL_NONE && !house->Can_Build(&VesselTypeClass::As_Reference(house->BuildVessel), house->ActLike)) {
		house->BuildVessel = VESSEL_NONE;
	}

	if (Frame < brain->NextThink) return;
	brain->NextThink = Frame + THINK_DELAY;

	brain->Members = Prune(brain->Member, brain->Members, house);
	brain->Raiders = Prune(brain->Raider, brain->Raiders, house);
	brain->Ships = Prune(brain->Ship, brain->Ships, house);
	brain->Repairs = Prune(brain->Repair, brain->Repairs, house);

	/*
	**	With no base left (and no construction yard to come), everything goes hunting, as with
	**	the original.
	*/
	if (house->CurBuildings == 0 && house->QuantityU(UNIT_MCV) == 0 && Frame > TICKS_PER_MINUTE) {
		if (brain->Members || brain->Raiders || brain->Ships) {
			brain->Members = brain->Raiders = brain->Ships = 0;
			brain->Wave = WAVE_NONE;
		}
		house->Do_All_To_Hunt();
		return;
	}

	/*
	**	A new construction yard deploys (made in a war factory, it would wait in guard area).
	*/
	if (house->QuantityB(STRUCT_CONST) == 0 && house->QuantityU(UNIT_MCV) > 0) {
		for (int index = 0; index < Units.Count(); index++) {
			UnitClass * unit = Units.Ptr(index);
			if (unit != NULL && unit->IsActive && !unit->IsInLimbo && Is_Owned(unit, house) && *unit == UNIT_MCV &&
					unit->Get_Mission() != MISSION_UNLOAD && !unit->IsDeploying) {
				unit->Assign_Mission(MISSION_UNLOAD);
			}
		}
	}

	Assess(house, *brain);
	Defend_AI(house, *brain);
	Wave_AI(house, *brain);
	Raid_AI(house, *brain);
	Navy_AI(house, *brain);
	Repair_AI(house, *brain);
	Super_AI(house, *brain);
}

#endif
