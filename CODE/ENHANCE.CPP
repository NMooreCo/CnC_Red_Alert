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
 *                    File Name : ENHANCE.CPP                                                  *
 *                                                                                             *
 *   Optional gameplay enhancements, which the original game didn't have. Each one is off      *
 *   until the player turns it on (Options, Game Controls, Enhancements, or REDALERT.INI's    *
 *   [Enhancements] section), so by default the game plays exactly as it always did.          *
 *                                                                                             *
 *   Veterancy: units earn ranks from their kills. A veteran deals more damage, takes less    *
 *   and fires faster; an elite unit more so, and repairs itself. Selected units show their   *
 *   rank as chevrons. Tuning: RULES.INI [Veterancy] (see Reset_Rules for the keys).           *
 *                                                                                             *
 *   Mod folders: mods/NAME/ in the data directory holds a mod's files (RULES.INI, scenarios,  *
 *   graphics, its own .MIX archives...), used instead of the game's own; chosen with -mod NAME *
 *   on the command line, RA_MOD=NAME, or Mod=NAME in REDALERT.INI's [Enhancements].            *
 *                                                                                             *
 *   Mission select: after choosing a side for a new game, start at any mission of that        *
 *   campaign reached so far (the furthest one is kept in REDALERT.INI).                       *
 *                                                                                             *
 *   Build queue: click a unit while its factory is busy to queue it; the sidebar starts each  *
 *   one in turn, as if the player clicked on it when the factory became free.                 *
 *                                                                                             *
 *   Shared tech: in skirmish games, both sides can build the units and buildings of both.    *
 *                                                                                             *
 *   Attack-move: <CTRL><SHIFT> or <CTRL><ALT> click on the ground; units move there, taking   *
 *   on the enemies they meet on the way, and guard the place.                                 *
 *                                                                                             *
 *   Modern mouse controls: the left button selects, the right one gives orders.               *
 *                                                                                             *
 *   Support powers: an artillery barrage (with a radar dome and a war factory), a naval       *
 *   bombardment (radar dome, and naval yard or sub pen) and Chinook reinforcements (helipad   *
 *   and barracks), in the sidebar like the original special weapons.                          *
 *---------------------------------------------------------------------------------------------*/

#include	"FUNCTION.H"
#include	"PLATFORM.H"

#ifdef RA_ENHANCE

EnhancementsClass Enhance;

int Mouse_Wheel_Left = 0;

int Mouse_Wheel_Take(void)
{
	return(Video_Take_Mouse_Wheel());
}

static char const ENHANCEMENTS[] = "Enhancements";
static char const VETERANCY[] = "Veterancy";


EnhancementsClass::EnhancementsClass(void) :
	Veterancy(false),
	BuildQueue(false),
	SharedTech(false),
	SupportPowers(false),
	AttackMove(false),
	ModernMouse(false),
	MissionSelect(false),
	AllMissions(false),
	AlliedMission(0),
	SovietMission(0)
{
	Reset_Rules();
}


/***********************************************************************************************
 * EnhancementsClass::Load_Settings -- The player's choices, from REDALERT.INI.                *
 *=============================================================================================*/
void EnhancementsClass::Load_Settings(INIClass const & ini)
{
	Veterancy = ini.Get_Bool(ENHANCEMENTS, "Veterancy", Veterancy);
	BuildQueue = ini.Get_Bool(ENHANCEMENTS, "BuildQueue", BuildQueue);
	SharedTech = ini.Get_Bool(ENHANCEMENTS, "SharedTech", SharedTech);
	SupportPowers = ini.Get_Bool(ENHANCEMENTS, "SupportPowers", SupportPowers);
	AttackMove = ini.Get_Bool(ENHANCEMENTS, "AttackMove", AttackMove);
	ModernMouse = ini.Get_Bool(ENHANCEMENTS, "ModernMouse", ModernMouse);
	MissionSelect = ini.Get_Bool(ENHANCEMENTS, "MissionSelect", MissionSelect);
	AllMissions = ini.Get_Bool(ENHANCEMENTS, "AllMissions", AllMissions);
	AlliedMission = ini.Get_Int(ENHANCEMENTS, "AlliedMission", AlliedMission);
	SovietMission = ini.Get_Int(ENHANCEMENTS, "SovietMission", SovietMission);
}


void EnhancementsClass::Save_Settings(INIClass & ini) const
{
	ini.Put_Bool(ENHANCEMENTS, "Veterancy", Veterancy);
	ini.Put_Bool(ENHANCEMENTS, "BuildQueue", BuildQueue);
	ini.Put_Bool(ENHANCEMENTS, "SharedTech", SharedTech);
	ini.Put_Bool(ENHANCEMENTS, "SupportPowers", SupportPowers);
	ini.Put_Bool(ENHANCEMENTS, "AttackMove", AttackMove);
	ini.Put_Bool(ENHANCEMENTS, "ModernMouse", ModernMouse);
	ini.Put_Bool(ENHANCEMENTS, "MissionSelect", MissionSelect);
	if (AllMissions) ini.Put_Bool(ENHANCEMENTS, "AllMissions", AllMissions);
	ini.Put_Int(ENHANCEMENTS, "AlliedMission", AlliedMission);
	ini.Put_Int(ENHANCEMENTS, "SovietMission", SovietMission);
}


/***********************************************************************************************
 * EnhancementsClass::Reset_Rules -- The default tuning.                                       *
 *=============================================================================================*/
void EnhancementsClass::Reset_Rules(void)
{
	VeteranKills = 3;
	EliteKills = 7;
	VeteranFirepower = fixed(6, 5);		// 1.2
	VeteranArmor = fixed(4, 5);			// 0.8
	VeteranROF = fixed(9, 10);				// 0.9
	EliteFirepower = fixed(7, 5);			// 1.4
	EliteArmor = fixed(13, 20);			// 0.65
	EliteROF = fixed(3, 4);					// 0.75
	EliteRepairDelay = 60;
	EliteRepairStep = fixed(1, 50);		// 2% of full strength

	ArtilleryTime = fixed(5);
	ArtilleryShells = 8;
	ArtilleryDamage = 150;					// The artillery's 155mm shell.
	ArtillerySpread = 2;
	BombardmentTime = fixed(8);
	BombardmentShells = 6;
	BombardmentDamage = 400;
	BombardmentSpread = 3;
	AirdropTime = fixed(6);
	AirdropInfantry = 5;
}


/***********************************************************************************************
 * EnhancementsClass::Rules -- Tuning from a rules file (RULES.INI, then the scenario).        *
 *=============================================================================================*/
void EnhancementsClass::Rules(INIClass const & ini)
{
	VeteranKills = ini.Get_Int(VETERANCY, "VeteranKills", VeteranKills);
	EliteKills = ini.Get_Int(VETERANCY, "EliteKills", EliteKills);
	VeteranFirepower = ini.Get_Fixed(VETERANCY, "VeteranFirepower", VeteranFirepower);
	VeteranArmor = ini.Get_Fixed(VETERANCY, "VeteranArmor", VeteranArmor);
	VeteranROF = ini.Get_Fixed(VETERANCY, "VeteranROF", VeteranROF);
	EliteFirepower = ini.Get_Fixed(VETERANCY, "EliteFirepower", EliteFirepower);
	EliteArmor = ini.Get_Fixed(VETERANCY, "EliteArmor", EliteArmor);
	EliteROF = ini.Get_Fixed(VETERANCY, "EliteROF", EliteROF);
	EliteRepairDelay = ini.Get_Int(VETERANCY, "EliteRepairDelay", EliteRepairDelay);
	EliteRepairStep = ini.Get_Fixed(VETERANCY, "EliteRepairStep", EliteRepairStep);

	static char const SUPPORT[] = "SupportPowers";
	ArtilleryTime = ini.Get_Fixed(SUPPORT, "ArtilleryTime", ArtilleryTime);
	ArtilleryShells = ini.Get_Int(SUPPORT, "ArtilleryShells", ArtilleryShells);
	ArtilleryDamage = ini.Get_Int(SUPPORT, "ArtilleryDamage", ArtilleryDamage);
	ArtillerySpread = ini.Get_Int(SUPPORT, "ArtillerySpread", ArtillerySpread);
	BombardmentTime = ini.Get_Fixed(SUPPORT, "BombardmentTime", BombardmentTime);
	BombardmentShells = ini.Get_Int(SUPPORT, "BombardmentShells", BombardmentShells);
	BombardmentDamage = ini.Get_Int(SUPPORT, "BombardmentDamage", BombardmentDamage);
	BombardmentSpread = ini.Get_Int(SUPPORT, "BombardmentSpread", BombardmentSpread);
	AirdropTime = ini.Get_Fixed(SUPPORT, "AirdropTime", AirdropTime);
	AirdropInfantry = ini.Get_Int(SUPPORT, "AirdropInfantry", AirdropInfantry);
}


/***********************************************************************************************
 * Enhance_Text -- Texts that the game's string files don't have (TXT_ENH_...).                *
 *=============================================================================================*/
char const * Enhance_Text(int string)
{
	static char const * const _texts[TXT_ENH_LAST - TXT_ENH_FIRST] = {
		"Artillery Barrage",
		"Naval Bombardment",
		"Chinook Reinforcements"
	};
	if (string >= TXT_ENH_FIRST && string < TXT_ENH_LAST) {
		return(_texts[string - TXT_ENH_FIRST]);
	}
	return("");
}


/***********************************************************************************************
 * EnhancementsClass::Adjust_Objects -- Changes to the object types, after the rules are read. *
 *                                                                                             *
 *    Shared tech: every unit and building that either side can build, both sides can build.  *
 *    The rules set each type's owners afresh whenever a scenario is read, which undoes this.  *
 *=============================================================================================*/
template<class T>
static void Share_Tech(TFixedIHeapClass<T> & types)
{
	int const both = HOUSEF_ALLIES | HOUSEF_SOVIET;
	for (int index = 0; index < types.Count(); index++) {
		T * type = types.Ptr(index);
		if (type != NULL && (type->Ownable & both) != 0) {
			type->Ownable |= both;
		}
	}
}


void EnhancementsClass::Adjust_Objects(void)
{
	if (Is_Shared_Tech()) {
		Share_Tech(BuildingTypes);
		Share_Tech(UnitTypes);
		Share_Tech(InfantryTypes);
		Share_Tech(VesselTypes);
		Share_Tech(AircraftTypes);
	}
}


/***********************************************************************************************
 * EnhancementsClass::Rules_Apply -- Can enhancements that change the game rules apply?        *
 *                                                                                             *
 *    In a network game, every player's game has to make exactly the same moves, so all of     *
 *    them would need the same enhancements. Until the host can send its choices to the others, *
 *    those enhancements apply only to campaign and skirmish games.                             *
 *=============================================================================================*/
bool EnhancementsClass::Rules_Apply(void) const
{
	return(Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH);
}


/***********************************************************************************************
 * Enhancements_Dialog -- Turn the enhancements on and off.                                     *
 *                                                                                             *
 *    Reached from the in-game Options: Game Controls, Enhancements. A list with a checkbox    *
 *    for each enhancement, and a description of the one that is highlighted.                 *
 *=============================================================================================*/
void Enhancements_Dialog(void)
{
	struct EnhancementInfo {
		char const * Name;
		char const * About;
		bool * Setting;
	};
	static EnhancementInfo const _list[] = {
		{"Veterancy: units earn ranks from their kills",
			"Units become veterans after 3 kills and elite after 7. They deal more damage, take less and fire "
			"faster, and elite units repair themselves. Selected units show their rank as chevrons.",
			&Enhance.Veterancy},
		{"Build queue: line up units at each factory",
			"Click a unit while its factory is busy to queue it, up to 9 for each factory; the sidebar shows how "
			"many more are waiting. Right-click takes one off the queue (and then, as before, holds or cancels "
			"the one being built). Works in network games too.",
			&Enhance.BuildQueue},
		{"Shared tech: both sides can build everything (skirmish)",
			"In skirmish games, every side can build the units and buildings of both the Allies and the "
			"Soviets: Tesla coils next to pillboxes, Mammoth tanks next to cruisers. The computer players "
			"can too.",
			&Enhance.SharedTech},
		{"Support powers: artillery, naval bombardment, Chinook",
			"New special weapons in the sidebar. An artillery barrage needs a radar dome and a war factory; a "
			"naval bombardment from off the map, a radar dome and a naval yard or sub pen; Chinook "
			"reinforcements (five soldiers), a helipad and barracks. The computer players use them too.",
			&Enhance.SupportPowers},
		{"Attack-move: <Ctrl><Shift> click on the ground",
			"Select units and hold <Ctrl><Shift> (or <Ctrl><Alt>) while you click on the ground: they move "
			"there, taking on the enemies they meet on the way, and then guard the place.",
			&Enhance.AttackMove},
		{"Modern mouse: left-click selects, right-click orders",
			"The right mouse button gives the selected units the order the pointer shows (move, attack, "
			"enter...), and the left button only selects: on the ground it unselects. Placing buildings, "
			"repairing, selling and the special weapons work as before. Works in network games too.",
			&Enhance.ModernMouse},
		{"Mission select: start at a mission you have reached",
			"When you start a new game and choose a side, you can start at any mission of that campaign "
			"you have reached so far. (Each mission you play is remembered from now on.)",
			&Enhance.MissionSelect},
	};
	int const count = ARRAY_SIZE(_list);

	/*
	**	Dialog & button dimensions.
	*/
	int	d_dialog_w = 260 * RESFACTOR;
	int	d_dialog_h = 150 * RESFACTOR;
	int	d_dialog_x = (SeenBuff.Get_Width() - d_dialog_w) / 2;
	int	d_dialog_y = (SeenBuff.Get_Height() - d_dialog_h) / 2;
	int	d_margin = 12 * RESFACTOR;

	int	d_list_x = d_dialog_x + d_margin;
	int	d_list_y = d_dialog_y + 25 * RESFACTOR;
	int	d_list_w = d_dialog_w - 2 * d_margin;
	int	d_list_h = 56 * RESFACTOR;

	int	d_text_x = d_list_x;
	int	d_text_y = d_list_y + d_list_h + 6 * RESFACTOR;
	int	d_text_w = d_list_w;

	int	d_ok_y = d_dialog_y + d_dialog_h - (9 * RESFACTOR) - (9 * RESFACTOR);

	enum {
		BUTTON_LIST = 100,
		BUTTON_OK
	};

	RemapControlType * scheme = GadgetClass::Get_Color_Scheme();

	CheckListClass list(BUTTON_LIST, d_list_x, d_list_y, d_list_w, d_list_h, TPF_TEXT, MFCD::Retrieve("BTN-UP.SHP"), MFCD::Retrieve("BTN-DN.SHP"));
	TextButtonClass okbtn(BUTTON_OK, TXT_OK, TPF_BUTTON, 0, d_ok_y);
	okbtn.X = d_dialog_x + (d_dialog_w - okbtn.Width) / 2;

	for (int index = 0; index < count; index++) {
		list.Add_Item(_list[index].Name);
		list.Check_Item(index, *_list[index].Setting);
	}
	list.Set_Selected_Index(0);

	GadgetClass * commands = &okbtn;
	list.Add_Tail(*commands);

	Set_Logic_Page(SeenBuff);

	bool process = true;
	bool display = true;
	while (process) {

		/*
		**	The game carries on underneath in a network game.
		*/
		if (Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH) {
			Call_Back();
		} else {
			if (Main_Loop()) {
				process = false;
			}
		}

		#ifdef WIN32
		if (AllSurfaces.SurfacesRestored) {
			AllSurfaces.SurfacesRestored = FALSE;
			display = true;
		}
		#endif

		if (display) {
			Hide_Mouse();
			Map.Flag_To_Redraw(true);
			Map.Render();
			Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
			Draw_Caption("Enhancements", d_dialog_x, d_dialog_y, d_dialog_w);

			/*
			**	What the highlighted one does, and when they apply.
			*/
			int current = list.Current_Index();
			char text[512];
			snprintf(text, sizeof(text), "%s\r\r%s", _list[(current >= 0 && current < count) ? current : 0].About,
				Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH ?
					"Those that change the game rules apply to campaign and skirmish games." :
					"Network games keep the original rules: those that change them apply to campaign and skirmish games.");
			int width, height;
			Format_Window_String(text, d_text_w, width, height);
			Fancy_Text_Print(text, d_text_x, d_text_y, scheme, TBLACK, TPF_TEXT);

			commands->Flag_List_To_Redraw();
			commands->Draw_All();
			Show_Mouse();
			display = false;
		}

		KeyNumType input = commands->Input();
		switch (input) {
			case (BUTTON_LIST | KN_BUTTON):
				display = true;		// Show the description of the item clicked.
				break;

			case (BUTTON_OK | KN_BUTTON):
			case KN_ESC:
			case KN_RETURN:
				process = false;
				break;

			default:
				break;
		}
	}

	/*
	**	Keep the choices.
	*/
	bool changed = false;
	for (int index = 0; index < count; index++) {
		if (*_list[index].Setting != list.Is_Checked(index)) {
			*_list[index].Setting = list.Is_Checked(index);
			changed = true;
		}
	}
	if (changed) {
		Options.Save_Settings();
	}
}


/***********************************************************************************************
 * Enhancements_Start_Scenario -- A scenario has started, or a saved game was loaded.          *
 *=============================================================================================*/
static void Support_Powers_Clear(void);
static void Attack_Move_Clear(void);

void Enhancements_Start_Scenario(void)
{
	Mission_Reached();
	Build_Queue_Clear();
	Support_Powers_Clear();
	Attack_Move_Clear();
}


/***********************************************************************************************
 * Modern_Mouse_Order -- Is the action an order to the selected units?                         *
 *                                                                                             *
 *    With the modern mouse controls, these are given with the right button; the left one     *
 *    selects instead.                                                                        *
 *=============================================================================================*/
bool Modern_Mouse_Order(ActionType action)
{
	switch (action) {
		case ACTION_MOVE:
		case ACTION_NOMOVE:
		case ACTION_ENTER:
		case ACTION_SELF:
		case ACTION_ATTACK:
		case ACTION_HARVEST:
		case ACTION_CAPTURE:
		case ACTION_SABOTAGE:
		case ACTION_GUARD_AREA:
		case ACTION_HEAL:
		case ACTION_DAMAGE:
		case ACTION_GREPAIR:
		case ACTION_NO_DEPLOY:
		case ACTION_NO_ENTER:
		case ACTION_NO_GREPAIR:
			return(true);

		default:
			return(false);
	}
}


/***********************************************************************************************
 * Attack-move -- the units moving to guard an area, taking on what they meet on the way.      *
 *                                                                                             *
 *    The order is the original guard area mission (EventClass MEGAMISSION) with the place as  *
 *    both the target and the destination. The units it was given to are listed here, with the *
 *    place: an entry counts while the unit is still guarding that place. The list is changed  *
 *    only as orders are carried out, the same way on every computer in a game; it isn't kept  *
 *    in saved games.                                                                          *
 *=============================================================================================*/
struct AttackMoverType {
	TARGET Unit;
	TARGET Place;
};

enum {MAX_ATTACK_MOVERS = 256};
static AttackMoverType AttackMovers[MAX_ATTACK_MOVERS];
static int AttackMoverCount = 0;


static void Attack_Move_Clear(void)
{
	AttackMoverCount = 0;
}


void Attack_Move_Order(FootClass * unit, bool attack_move)
{
	TARGET who = unit->As_Target();
	int index;
	for (index = 0; index < AttackMoverCount; index++) {
		if (AttackMovers[index].Unit == who) break;
	}
	if (attack_move && Enhance.Is_Attack_Move()) {
		if (index == AttackMoverCount) {
			if (AttackMoverCount == MAX_ATTACK_MOVERS) return;
			AttackMoverCount++;
		}
		AttackMovers[index].Unit = who;
		AttackMovers[index].Place = unit->ArchiveTarget;
	} else if (index < AttackMoverCount) {
		AttackMovers[index] = AttackMovers[--AttackMoverCount];
	}
}


bool Is_Attack_Moving(FootClass const * unit)
{
	if (AttackMoverCount == 0 || !Enhance.Is_Attack_Move() || unit->Get_Mission() != MISSION_GUARD_AREA) return(false);

	TARGET who = unit->As_Target();
	for (int index = 0; index < AttackMoverCount; index++) {
		if (AttackMovers[index].Unit == who) {
			return(AttackMovers[index].Place == unit->ArchiveTarget);
		}
	}
	return(false);
}


/***********************************************************************************************
 * Support powers -- an artillery barrage, a naval bombardment, Chinook reinforcements.        *
 *                                                                                             *
 *    They work like the original special weapons (HouseClass::SuperWeapon): a building gives  *
 *    the power, which charges up in the sidebar; the player aims it at the map, which sends a  *
 *    SPECIAL_PLACE event (DISPLAY.CPP), and HouseClass::Place_Special_Blast fires it here.     *
 *    The shells of barrages and bombardments land over a few seconds (Support_Powers_Impacts, *
 *    every game frame); they aren't kept in saved games.                                      *
 *=============================================================================================*/
void Support_Powers_Init(HouseClass * house)
{
	new (&house->SuperWeapon[SPC_ARTILLERY]) SuperClass(TICKS_PER_MINUTE * Enhance.ArtilleryTime, true, VOX_NONE, VOX_NONE, VOX_NONE, VOX_INSUFFICIENT_POWER);
	new (&house->SuperWeapon[SPC_BOMBARDMENT]) SuperClass(TICKS_PER_MINUTE * Enhance.BombardmentTime, true, VOX_NONE, VOX_NONE, VOX_NONE, VOX_INSUFFICIENT_POWER);
	new (&house->SuperWeapon[SPC_AIRDROP]) SuperClass(TICKS_PER_MINUTE * Enhance.AirdropTime, false, VOX_NONE, VOX_NONE, VOX_NONE, VOX_NONE);
}


/*
**	Give or take away one power, as the house's buildings allow; the computer fires it when ready.
*/
static void Support_Power(HouseClass * house, SpecialWeaponType power, bool allowed)
{
	SuperClass & super = house->SuperWeapon[power];
	bool player = (house == PlayerPtr);

	if (super.Is_Present()) {
		if (!allowed) {
			if (super.Remove()) {
				if (player) {
					if (Map.IsTargettingMode == power) Map.IsTargettingMode = SPC_NONE;
					Map.Column[1].Flag_To_Redraw();
				}
				house->IsRecalcNeeded = true;
			}
		} else if (super.Is_Ready() && !house->IsHuman) {
			house->Special_Weapon_AI(power);
		}
	} else if (allowed) {
		super.Enable(false, player, super.Is_Powered() && house->Power_Fraction() < 1);
		if (player) {
			Map.Add(RTTI_SPECIAL, power);
			Map.Column[1].Flag_To_Redraw();
		}
	}
}


void Support_Powers_AI(HouseClass * house)
{
	bool on = Enhance.Is_Support_Powers() && !house->IsDefeated && (house->IsHuman || house->IQ >= Rule.IQSuperWeapons);
	long have = house->ActiveBScan;

	Support_Power(house, SPC_ARTILLERY, on && (have & STRUCTF_RADAR) && (have & STRUCTF_WEAP));
	Support_Power(house, SPC_BOMBARDMENT, on && (have & STRUCTF_RADAR) && (have & (STRUCTF_SHIP_YARD|STRUCTF_SUB_PEN)));
	Support_Power(house, SPC_AIRDROP, on && (have & STRUCTF_HELIPAD) && (have & (STRUCTF_TENT|STRUCTF_BARRACKS)));
}


/*
**	The shells on their way.
*/
struct ShellType {
	int Frame;					// The game frame it lands in.
	COORDINATE Coord;
	int Damage;
	AnimType Anim;
};

enum {MAX_IMPACTS = 64};
static ShellType Impacts[MAX_IMPACTS];
static int ImpactCount = 0;


static void Barrage(CELL cell, int shells, int spread, int damage, AnimType anim, int delay, int interval)
{
	for (int shell = 0; shell < shells && ImpactCount < MAX_IMPACTS; shell++) {
		ShellType & impact = Impacts[ImpactCount++];
		impact.Frame = Frame + delay + shell * interval + Random_Pick(0, interval);
		impact.Coord = Coord_Scatter(Cell_Coord(cell), Random_Pick(0, spread * CELL_LEPTON_W));
		impact.Damage = damage;
		impact.Anim = anim;
	}
}


static void Support_Powers_Clear(void)
{
	ImpactCount = 0;
}


void Support_Powers_Impacts(void)
{
	for (int index = 0; index < ImpactCount; ) {
		if (Frame >= Impacts[index].Frame) {
			ShellType impact = Impacts[index];
			Impacts[index] = Impacts[--ImpactCount];

			new AnimClass(impact.Anim, impact.Coord);
			Explosion_Damage(impact.Coord, impact.Damage, NULL, WARHEAD_HE);
		} else {
			index++;
		}
	}
}


/*
**	A Chinook flies in from the house's side of the map with soldiers, lands near the target,
**	lets them out and flies off again.
*/
static void Chinook_Reinforcements(HouseClass * house, CELL cell)
{
	AircraftTypeClass const & type = AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT);

	ScenarioInit++;
	AircraftClass * chinook = (AircraftClass *)type.Create_One_Of(house);
	ScenarioInit--;
	if (chinook == NULL) return;
	chinook->IsALoaner = true;

	int count = min(Enhance.AirdropInfantry, type.Max_Passengers());
	for (int index = 0; index < count; index++) {
		ScenarioInit++;
		InfantryClass * soldier = (InfantryClass *)InfantryTypeClass::As_Reference((index % 3 == 2) ? INFANTRY_E3 : INFANTRY_E1).Create_One_Of(house);
		ScenarioInit--;
		if (soldier != NULL) {
			chinook->Attach(soldier);
		}
	}

	SourceType source = house->Control.Edge;
	if (source != SOURCE_NORTH && source != SOURCE_EAST && source != SOURCE_SOUTH && source != SOURCE_WEST) {
		source = SOURCE_NORTH;
	}
	CELL edge = Map.Calculated_Cell(source, -1, -1, SPEED_WINGED);

	ScenarioInit++;
	bool placed = chinook->Unlimbo(Cell_Coord(edge), DIR_N);
	ScenarioInit--;
	if (!placed) {
		delete chinook;
		return;
	}
	chinook->Assign_Destination(::As_Target(Map.Nearby_Location(cell, SPEED_FOOT)));
	chinook->Assign_Mission(MISSION_UNLOAD);
	if (house == PlayerPtr) {
		Speak(VOX_REINFORCEMENTS);
	}
}


void Support_Power_Fire(HouseClass * house, SpecialWeaponType power, CELL cell)
{
	switch (power) {
		case SPC_ARTILLERY:
			Barrage(cell, Enhance.ArtilleryShells, Enhance.ArtillerySpread, Enhance.ArtilleryDamage, ANIM_ART_EXP1, TICKS_PER_SECOND, 3);
			break;

		case SPC_BOMBARDMENT:
			Barrage(cell, Enhance.BombardmentShells, Enhance.BombardmentSpread, Enhance.BombardmentDamage, ANIM_FBALL1, TICKS_PER_SECOND * 2, 6);
			break;

		case SPC_AIRDROP:
			Chinook_Reinforcements(house, cell);
			break;

		default:
			break;
	}
}


/***********************************************************************************************
 * Build queue -- units waiting for their factory.                                             *
 *                                                                                             *
 *    One queue each for infantry, vehicles, ships and aircraft. (Buildings are placed one at  *
 *    a time, so they aren't queued.) When the factory is free, Build_Queue_AI starts the next *
 *    one, as if the player had clicked on it: the queue isn't part of the game itself, which  *
 *    is why it works in network games too. It isn't kept in saved games.                       *
 *=============================================================================================*/
enum {BUILD_QUEUE_MAX = 9};

struct BuildQueueType {
	RTTIType Type;
	int Count;
	int ID[BUILD_QUEUE_MAX];
	bool IsStarting;		// Started the first one: waiting for its production to begin,
	int StartFrame;		// since this frame.
};

static BuildQueueType BuildQueues[] = {
	{RTTI_INFANTRYTYPE, 0, {0}, false, 0},
	{RTTI_UNITTYPE, 0, {0}, false, 0},
	{RTTI_VESSELTYPE, 0, {0}, false, 0},
	{RTTI_AIRCRAFTTYPE, 0, {0}, false, 0}
};


static BuildQueueType * Build_Queue(RTTIType type)
{
	for (int index = 0; index < ARRAY_SIZE(BuildQueues); index++) {
		if (BuildQueues[index].Type == type) return(&BuildQueues[index]);
	}
	return(NULL);
}


static void Build_Queue_Redraw(void)
{
	Map.Column[0].Flag_To_Redraw();
	Map.Column[1].Flag_To_Redraw();
}


static void Build_Queue_Take(BuildQueueType * queue, int index)
{
	memmove(&queue->ID[index], &queue->ID[index+1], (queue->Count - index - 1) * sizeof(queue->ID[0]));
	queue->Count--;
	Build_Queue_Redraw();
}


/*
**	Queue one more (the sidebar, a left click on a unit whose factory is busy).
*/
bool Build_Queue_Add(RTTIType type, int id)
{
	BuildQueueType * queue = Enhance.BuildQueue ? Build_Queue(type) : NULL;
	if (queue == NULL || queue->Count >= BUILD_QUEUE_MAX) return(false);

	queue->ID[queue->Count++] = id;
	Speak((type == RTTI_INFANTRYTYPE) ? VOX_TRAINING : VOX_BUILDING);
	Build_Queue_Redraw();
	return(true);
}


/*
**	Take the last queued one of these off the queue (the sidebar, a right click).
*/
bool Build_Queue_Remove(RTTIType type, int id)
{
	BuildQueueType * queue = Build_Queue(type);
	if (queue == NULL) return(false);

	for (int index = queue->Count-1; index >= 0; index--) {
		if (queue->ID[index] == id) {
			Build_Queue_Take(queue, index);
			return(true);
		}
	}
	return(false);
}


int Build_Queue_Count(RTTIType type, int id)
{
	BuildQueueType const * queue = Build_Queue(type);
	int count = 0;
	if (queue != NULL) {
		for (int index = 0; index < queue->Count; index++) {
			if (queue->ID[index] == id) count++;
		}
	}
	return(count);
}


void Build_Queue_Clear(void)
{
	for (int index = 0; index < ARRAY_SIZE(BuildQueues); index++) {
		BuildQueues[index].Count = 0;
		BuildQueues[index].IsStarting = false;
	}
}


/*
**	Called by the sidebar every time round: start the next one when a factory is free.
*/
void Build_Queue_AI(void)
{
	if (PlayerPtr == NULL) return;

	for (int index = 0; index < ARRAY_SIZE(BuildQueues); index++) {
		BuildQueueType * queue = &BuildQueues[index];

		if (!Enhance.BuildQueue && queue->Count > 0) {
			queue->Count = 0;
			Build_Queue_Redraw();
		}

		FactoryClass * factory = PlayerPtr->Fetch_Factory(queue->Type);

		/*
		**	After starting one, wait for its production to begin (in a network game, that's a
		**	few frames later), so that the next one isn't started too.
		*/
		if (queue->IsStarting) {
			if (factory != NULL || Frame - queue->StartFrame > 60 || Frame < queue->StartFrame) {
				queue->IsStarting = false;
			}
			continue;
		}
		if (factory != NULL || queue->Count == 0) continue;

		/*
		**	The factory is free: start the next one, if it can still be built (a factory or
		**	other prerequisite may have been lost since).
		*/
		int id = queue->ID[0];
		Build_Queue_Take(queue, 0);

		TechnoTypeClass const * techno = Fetch_Techno_Type(queue->Type, id);
		if (techno == NULL || !PlayerPtr->Can_Build(techno, PlayerPtr->ActLike) || PlayerPtr->Is_Hack_Prevented(queue->Type, id)) {
			continue;
		}
		OutList.Add(EventClass(EventClass::PRODUCE, queue->Type, id));
		queue->IsStarting = true;
		queue->StartFrame = Frame;
	}
}


/***********************************************************************************************
 * Mod folders                                                                                 *
 *                                                                                             *
 *    A mod is a folder in mods/ (in the data directory). Every file the game opens is looked  *
 *    for there first (CDFileClass::Set_Name), so loose files there replace the game's: a      *
 *    RULES.INI, scenarios, shapes... (An .INI there is used whole, as with loose files in the *
 *    data directory.) The .MIX archives there are added before the game's own, so files in   *
 *    them come first too (they are kept in memory); one named like a game archive replaces   *
 *    it. The settings and saved                                                               *
 *    games stay in the data directory. Every player of a network game needs the same mod.     *
 *=============================================================================================*/
char const * Mod_Path = NULL;


void Mod_Select(int argc, char * argv[])
{
	char const * name = NULL;
	for (int index = 1; index + 1 < argc; index++) {
		if (stricmp(argv[index], "-mod") == 0 || stricmp(argv[index], "--mod") == 0) {
			name = argv[index + 1];
		}
	}
	if (name == NULL) {
		name = getenv("RA_MOD");
	}

	static char _ininame[64];
	if (name == NULL) {
		RawFileClass file("REDALERT.INI");
		INIClass ini;
		if (file.Is_Available() && ini.Load(file)) {
			ini.Get_String(ENHANCEMENTS, "Mod", "", _ininame, sizeof(_ininame));
			if (_ininame[0] != '\0') name = _ininame;
		}
	}
	if (name == NULL || *name == '\0') return;

	static char _path[_MAX_PATH];
	snprintf(_path, sizeof(_path), "mods/%s", name);
	DWORD attributes = GetFileAttributes(_path);
	if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
		fprintf(stderr, "Red Alert: the mod folder %s was not found; playing without a mod.\n", _path);
		return;
	}
	strcat(_path, "/");
	Mod_Path = _path;
	fprintf(stderr, "Red Alert: using the mod in %s\n", _path);
}


/*
**	The file in the mod folder that is to be used instead of the game's, if a mod is in use. The
**	game's own settings and saved games aren't looked for there.
*/
bool Mod_File_Name(char const * filename, char * path, int size)
{
	if (Mod_Path == NULL || filename == NULL || *filename == '\0') return(false);
	if (strchr(filename, '/') != NULL || strchr(filename, '\\') != NULL || strchr(filename, ':') != NULL) return(false);
	if (stricmp(filename, "REDALERT.INI") == 0 || strnicmp(filename, "SAVEGAME.", 9) == 0 || stricmp(filename, "HALLFAME.DAT") == 0) return(false);

	snprintf(path, size, "%s%s", Mod_Path, filename);
	return(true);
}


/*
**	The mod's own archives (those not named like one of the game's, which the game opens itself:
**	from the mod folder, then).
*/
void Mod_Register_Mixfiles(void)
{
	if (Mod_Path == NULL) return;

	static char const * const _game[] = {
		"REDALERT.MIX", "LOCAL.MIX", "HIRES.MIX", "NCHIRES.MIX", "LORES.MIX", "HIRES1.MIX", "LORES1.MIX",
		"MAIN.MIX", "CONQUER.MIX", "GENERAL.MIX", "SOUNDS.MIX", "RUSSIAN.MIX", "ALLIES.MIX", "SCORES.MIX",
		"SNOW.MIX", "TEMPERAT.MIX", "INTERIOR.MIX", "SPEECH.MIX", "MOVIES1.MIX", "MOVIES2.MIX",
		"EXPAND.MIX", "EXPAND2.MIX", "WOLAPI.MIX", "TRANSIT.MIX"
	};

	char pattern[_MAX_PATH];
	snprintf(pattern, sizeof(pattern), "%s*.MIX", Mod_Path);
	WIN32_FIND_DATA data;
	HANDLE handle = FindFirstFile(pattern, &data);
	if (handle == INVALID_HANDLE_VALUE) return;
	do {
		bool known = false;
		for (int index = 0; index < ARRAY_SIZE(_game); index++) {
			if (stricmp(data.cFileName, _game[index]) == 0) known = true;
		}
		if (!known) {
			/*
			**	Kept in memory, as the game's small archives are: the game takes some files
			**	(palettes, shapes, texts) straight from memory, which needs the archive to be.
			*/
			char const * name = strdup(data.cFileName);
			fprintf(stderr, "Red Alert: the mod's archive %s\n", name);
			new MFCD(name, &FastKey);
			MFCD::Cache(name);
		}
	} while (FindNextFile(handle, &data));
	FindClose(handle);
}


/***********************************************************************************************
 * Mission_Reached -- Remember the furthest campaign mission reached.                          *
 *                                                                                             *
 *    Called when a scenario starts, or a saved game is loaded.                                *
 *=============================================================================================*/
void Mission_Reached(void)
{
	if (Session.Type != GAME_NORMAL) return;
	if (toupper(Scen.ScenarioName[0]) != 'S' || toupper(Scen.ScenarioName[1]) != 'C') return;

	int * reached = NULL;
	switch (toupper(Scen.ScenarioName[2])) {
		case 'G':	reached = &Enhance.AlliedMission;	break;
		case 'U':	reached = &Enhance.SovietMission;	break;
		default:		break;
	}
	if (reached != NULL && Scen.Scenario > *reached) {
		*reached = Scen.Scenario;
		Options.Save_Settings();
	}
}


/***********************************************************************************************
 * Mission_Select_Dialog -- Choose the mission to start a new campaign at.                     *
 *                                                                                             *
 *    Called once the player has chosen a side (Scen.ScenarioName is that campaign's first    *
 *    mission). Lists every version of each mission reached so far, by name.                   *
 *                                                                                             *
 * OUTPUT:  false if the player cancelled.                                                     *
 *=============================================================================================*/
bool Mission_Select_Dialog(void)
{
	if (!Enhance.MissionSelect) return(true);

	char side = (char)toupper(Scen.ScenarioName[2]);
	if (side != 'G' && side != 'U') return(true);
	int reached = Enhance.AllMissions ? 99 : (side == 'G') ? Enhance.AlliedMission : Enhance.SovietMission;
	if (reached < 2) return(true);

	/*
	**	The missions. A mission may come in more than one version (the map selection screen
	**	offers a choice), or in parts (Allied mission 10).
	*/
	enum {MAX_ENTRIES = 64};
	static char _files[MAX_ENTRIES][16];
	static char _texts[MAX_ENTRIES][80];
	int count = 0;
	int furthest = 0;		// The main version of the furthest mission.
	for (int number = 1; number <= reached && number <= 99 && count < MAX_ENTRIES; number++) {
		bool found = false;
		char main_name[48] = "";
		for (char const * version = "EAEBECEDWAWBWCWD"; *version != '\0' && count < MAX_ENTRIES; version += 2) {
			char file[16];
			snprintf(file, sizeof(file), "SC%c%02d%c%c.INI", side, number, version[0], version[1]);
			CCFileClass scenario(file);
			if (!scenario.Is_Available()) continue;

			/*
			**	Other versions are often unnamed: they take the main version's name.
			*/
			char name[48] = "";
			CCINIClass ini;
			if (ini.Load(scenario, false)) {
				ini.Get_String("Basic", "Name", "", name, sizeof(name));
			}
			if (name[0] == '\0' || stricmp(name, "<none>") == 0) {
				strcpy(name, main_name);
			}

			strcpy(_files[count], file);
			char label[24];
			if (!found) {
				strcpy(main_name, name);
				furthest = count;
				snprintf(label, sizeof(label), "Mission %d", number);
			} else {
				snprintf(label, sizeof(label), "Mission %d (version %s%c)", number, (version[0] == 'W') ? "W" : "", version[1]);
			}
			if (name[0] != '\0') {
				snprintf(_texts[count], sizeof(_texts[count]), "%s: %s", label, name);
			} else {
				snprintf(_texts[count], sizeof(_texts[count]), "%s", label);
			}
			found = true;
			count++;
		}
		if (!found && number > 1) break;		// The end of the campaign.
	}
	if (count < 2) return(true);

	/*
	**	Dialog & button dimensions.
	*/
	int	d_dialog_w = 220 * RESFACTOR;
	int	d_dialog_h = 150 * RESFACTOR;
	int	d_dialog_x = (SeenBuff.Get_Width() - d_dialog_w) / 2;
	int	d_dialog_y = (SeenBuff.Get_Height() - d_dialog_h) / 2;
	int	d_margin = 12 * RESFACTOR;

	int	d_list_x = d_dialog_x + d_margin;
	int	d_list_y = d_dialog_y + 25 * RESFACTOR;
	int	d_list_w = d_dialog_w - 2 * d_margin;
	int	d_list_h = d_dialog_h - (25 + 30) * RESFACTOR;

	int	d_button_y = d_dialog_y + d_dialog_h - (9 * RESFACTOR) - (10 * RESFACTOR);

	enum {
		BUTTON_LIST = 100,
		BUTTON_OK,
		BUTTON_CANCEL
	};

	ListClass list(BUTTON_LIST, d_list_x, d_list_y, d_list_w, d_list_h, TPF_TEXT, MFCD::Retrieve("BTN-UP.SHP"), MFCD::Retrieve("BTN-DN.SHP"));
	TextButtonClass okbtn(BUTTON_OK, TXT_OK, TPF_BUTTON, 0, d_button_y);
	TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, TPF_BUTTON, 0, d_button_y);
	okbtn.X = d_dialog_x + d_dialog_w / 3 - okbtn.Width / 2;
	cancelbtn.X = d_dialog_x + (2 * d_dialog_w) / 3 - cancelbtn.Width / 2;

	for (int index = 0; index < count; index++) {
		list.Add_Item(_texts[index]);
	}
	list.Set_Selected_Index(furthest);

	GadgetClass * commands = &okbtn;
	cancelbtn.Add_Tail(*commands);
	list.Add_Tail(*commands);

	Set_Logic_Page(SeenBuff);

	bool process = true;
	bool display = true;
	bool start = false;
	while (process) {
		Call_Back();

		#ifdef WIN32
		if (AllSurfaces.SurfacesRestored) {
			AllSurfaces.SurfacesRestored = FALSE;
			display = true;
		}
		#endif

		if (display) {
			Hide_Mouse();
			Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
			Draw_Caption((side == 'G') ? "Allied campaign: choose a mission" : "Soviet campaign: choose a mission", d_dialog_x, d_dialog_y, d_dialog_w);
			commands->Flag_List_To_Redraw();
			commands->Draw_All();
			Show_Mouse();
			display = false;
		}

		KeyNumType input = commands->Input();
		switch (input) {
			case (BUTTON_OK | KN_BUTTON):
			case KN_RETURN:
				start = true;
				process = false;
				break;

			case (BUTTON_CANCEL | KN_BUTTON):
			case KN_ESC:
				process = false;
				break;

			default:
				break;
		}
	}

	if (start) {
		int index = list.Current_Index();
		if (index >= 0 && index < count) {
			Scen.Set_Scenario_Name(_files[index]);
		}
	}
	return(start);
}


/***********************************************************************************************
 * Veterancy_Rank -- The rank of a unit.                                                       *
 *                                                                                             *
 *    Only units (infantry, vehicles, ships and aircraft) earn ranks; buildings don't.         *
 *=============================================================================================*/
VeterancyType Veterancy_Rank(TechnoClass const * techno)
{
	if (techno == NULL || !Enhance.Is_Veterancy() || !techno->Is_Foot()) return(VETERANCY_ROOKIE);

	int kills = techno->Crew.Kills;
	if (Enhance.EliteKills > 0 && kills >= Enhance.EliteKills) return(VETERANCY_ELITE);
	if (Enhance.VeteranKills > 0 && kills >= Enhance.VeteranKills) return(VETERANCY_VETERAN);
	return(VETERANCY_ROOKIE);
}


/***********************************************************************************************
 * Veterancy_Credit_Kill -- Count a kill for the unit that made it.                            *
 *                                                                                             *
 *    The original counted the kill for the victim's crew, which was about to die anyway, so   *
 *    no unit ever built up kills (TechnoClass::Record_The_Kill). Friendly fire, civilians,    *
 *    barrels and mines don't count.                                                           *
 *=============================================================================================*/
void Veterancy_Credit_Kill(TechnoClass * killer, TechnoClass const * victim)
{
	if (killer == NULL || victim == NULL || !killer->IsActive || !killer->Is_Foot()) return;
	if (killer->House->Is_Ally(victim) || victim->Owner() == HOUSE_NEUTRAL) return;
	if (victim->What_Am_I() == RTTI_BUILDING) {
		StructType type = ((BuildingClass const *)victim)->Class->Type;
		if (type == STRUCT_BARREL || type == STRUCT_BARREL3 || type == STRUCT_APMINE || type == STRUCT_AVMINE) return;
	}

	VeterancyType before = Veterancy_Rank(killer);
	killer->Crew.Made_A_Kill();
	VeterancyType after = Veterancy_Rank(killer);

	if (after != before) {
		killer->Mark(MARK_CHANGE);
		if (killer->IsOwnedByPlayer) {
			Speak((after == VETERANCY_ELITE) ? VOX_UPGRADE_ARMOR : VOX_UPGRADE_FIREPOWER);
		}
	}
}


/***********************************************************************************************
 * Veterancy_Firepower, _Armor, _ROF -- What a unit's rank multiplies.                         *
 *=============================================================================================*/
fixed Veterancy_Firepower(TechnoClass const * techno)
{
	switch (Veterancy_Rank(techno)) {
		case VETERANCY_VETERAN:	return(Enhance.VeteranFirepower);
		case VETERANCY_ELITE:	return(Enhance.EliteFirepower);
		default:						return(fixed(1));
	}
}


fixed Veterancy_Armor(TechnoClass const * techno)
{
	switch (Veterancy_Rank(techno)) {
		case VETERANCY_VETERAN:	return(Enhance.VeteranArmor);
		case VETERANCY_ELITE:	return(Enhance.EliteArmor);
		default:						return(fixed(1));
	}
}


fixed Veterancy_ROF(TechnoClass const * techno)
{
	switch (Veterancy_Rank(techno)) {
		case VETERANCY_VETERAN:	return(Enhance.VeteranROF);
		case VETERANCY_ELITE:	return(Enhance.EliteROF);
		default:						return(fixed(1));
	}
}


/***********************************************************************************************
 * Veterancy_AI -- Elite units repair themselves, a step at a time.                            *
 *=============================================================================================*/
void Veterancy_AI(TechnoClass * techno)
{
	if (Enhance.EliteRepairDelay <= 0 || (Frame % Enhance.EliteRepairDelay) != 0) return;
	if (Veterancy_Rank(techno) != VETERANCY_ELITE) return;

	int full = techno->Techno_Type_Class()->MaxStrength;
	if (techno->Strength <= 0 || techno->Strength >= full) return;

	int step = full * Enhance.EliteRepairStep;
	if (step < 1) step = 1;
	techno->Strength = (short)min(full, techno->Strength + step);
	techno->Mark(MARK_CHANGE);
}


/***********************************************************************************************
 * Veterancy_Draw -- Draw a selected unit's rank: one chevron for a veteran, two for elite.    *
 *                                                                                             *
 * INPUT:   x,y   -- The middle of the bottom of the chevrons, which stack upwards from there   *
 *                   (just above the unit's health bar).                                       *
 *=============================================================================================*/
void Veterancy_Draw(TechnoClass const * techno, GraphicViewPortClass & window, int x, int y)
{
	VeterancyType rank = Veterancy_Rank(techno);
	int count = (rank == VETERANCY_ELITE) ? 2 : (rank == VETERANCY_VETERAN) ? 1 : 0;

	for (int index = 0; index < count; index++) {
		int top = y - 4 - index * 4;

		/*
		**	A dark edge under each one, so that they show on snow as well as on dark ground.
		*/
		window.Draw_Line(x-3, top+4, x, top+1, BLACK);
		window.Draw_Line(x, top+1, x+3, top+4, BLACK);
		window.Draw_Line(x-3, top+3, x, top, YELLOW);
		window.Draw_Line(x, top, x+3, top+3, YELLOW);
	}
}

#endif
