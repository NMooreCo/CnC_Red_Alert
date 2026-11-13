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
**	RANDMAP.CPP -- random maps for skirmish and network games (RA_ENHANCE).
**
**	"Random Map: Small (2-4)", "Medium (4-6)" and "Large (6-8)" are at the end of the skirmish
**	and network games' scenario lists. Choosing one makes a new map (choosing it again makes
**	another), saved as RANDOMS.MPR, RANDOMM.MPR or RANDOML.MPR in the game's directory: a
**	multiplayer map like those players made with the editor, which the game sends to the
**	other players of a network game that don't have it.
**
**	Red Alert's terrain is built of templates, pieces of art that only fit next to some
**	others, so the generator doesn't invent shorelines or cliffs. It takes them from the maps
**	there are: in each scenario of the same theater (temperate or snow), every group of
**	terrain pieces surrounded by clear ground (a lake, a ridge of cliffs, a river with its
**	bridges, a rock) is a "feature" that can go anywhere else on clear ground, as it is. A
**	group that runs off the edge of its map (a coast, a cliff) can go at the same edge of the
**	new map, where it is cut off the same way.
**
**	A new map has start positions spread around it, each with room for a base and an ore
**	field nearby, more ore (and gems) between them, features placed at random so that every
**	start can still be reached from every other by land, and clumps of trees.
*/

#include	"FUNCTION.H"
#include	<math.h>
#include	<time.h>

#ifdef RA_ENHANCE

/*
**	The random map entries of the scenario lists.
*/
struct RandomMapType {
	char const * Filename;
	char const * Description;
	int Size;							// Width and height, in cells.
	int Starts;							// Start positions.
};
static RandomMapType const RandomMaps[] = {
	{"RANDOMS.MPR", "Random Map: Small (2-4)", 64, 4},
	{"RANDOMM.MPR", "Random Map: Medium (4-6)", 80, 6},
	{"RANDOML.MPR", "Random Map: Large (6-8)", 96, 8},
};


/*
**	The generator's own random numbers (it runs on one computer: the map is sent to the
**	others, so it doesn't use the game's synchronized ones).
*/
static unsigned RandomState = 1;

static unsigned Rnd(void)
{
	RandomState ^= RandomState << 13;
	RandomState ^= RandomState >> 17;
	RandomState ^= RandomState << 5;
	return(RandomState);
}

static int Rnd(int low, int high)
{
	if (high <= low) return(low);
	return(low + (int)(Rnd() % (unsigned)(high - low + 1)));
}


/***************************************************************************************
**	The feature library: the groups of terrain pieces of the maps of one theater.
*/
enum {
	EDGE_LEFT = 1,
	EDGE_TOP = 2,
	EDGE_RIGHT = 4,
	EDGE_BOTTOM = 8
};

enum FeatureKind {
	KIND_WATER,							// A lake or a coast.
	KIND_RIVER,							// A river (and its bridges and fords).
	KIND_CLIFF,							// Cliffs and rocks that block the way.
	KIND_DECOR,							// The rest: rough ground, patches, roads, small rocks.
	KIND_COUNT
};

struct FeatureCell {
	short X;								// Where in the feature (from its top left corner).
	short Y;
	unsigned short TType;
	unsigned char TIcon;
	unsigned char Land;					// Its LandType.
};

struct Feature {
	short Width;
	short Height;
	short Left;							// Where its map's edges were (from the feature's top left
	short Top;							// corner): beyond them, its map had no ground.
	short Right;
	short Bottom;
	int First;							// Its first cell in Library.Cells.
	int Count;
	unsigned char Edges;				// The edges of its map it reaches (EDGE_...).
	unsigned char Kind;
};

static struct {
	TheaterType Theater;
	FeatureCell * Cells;
	int CellCount;
	int CellMax;
	Feature * Features;
	int Count;
	int Max;
} Library = {THEATER_NONE, NULL, 0, 0, NULL, 0, 0};

static unsigned short SourceTType[MAP_CELL_TOTAL];
static unsigned char SourceTIcon[MAP_CELL_TOTAL];
static unsigned char Staging[0x14000];


static bool Is_Clear_Template(unsigned short ttype)
{
	return(ttype == (unsigned short)TEMPLATE_NONE || ttype == TEMPLATE_CLEAR1 || ttype == 255);
}


/*
**	Adds one group of terrain pieces (cells, the map's bounds) to the library.
*/
static void Library_Add(CELL const * cells, int count, TheaterType theater, int x0, int y0, int w, int h)
{
	if (count < 1 || count > 3000) return;

	int minx = MAP_CELL_W, miny = MAP_CELL_H, maxx = -1, maxy = -1;
	int kinds[KIND_COUNT] = {0, 0, 0, 0};
	int shore = 0;
	for (int index = 0; index < count; index++) {
		int x = Cell_X(cells[index]);
		int y = Cell_Y(cells[index]);
		minx = min(minx, x);
		miny = min(miny, y);
		maxx = max(maxx, x);
		maxy = max(maxy, y);

		/*
		**	Only pieces of this theater (with their art at hand, to tell the land).
		*/
		unsigned short ttype = SourceTType[cells[index]];
		if (ttype >= TEMPLATE_COUNT) return;
		if (ttype >= TEMPLATE_SHORE01 && ttype <= TEMPLATE_SHORECLIFF38) shore++;
		TemplateTypeClass const & tplate = TemplateTypeClass::As_Reference((TemplateType)ttype);
		if ((tplate.Theater & (1 << theater)) == 0 || tplate.Get_Image_Data() == NULL) return;
		switch (tplate.Land_Type(SourceTIcon[cells[index]])) {
			case LAND_WATER: kinds[KIND_WATER]++; break;
			case LAND_RIVER: kinds[KIND_RIVER]++; break;
			case LAND_ROCK: kinds[KIND_CLIFF]++; break;
			default: break;
		}
	}

	/*
	**	Bits of shore with no water (the edge of a lake that was in the way of something else)
	**	look odd on their own.
	*/
	if (shore > 0 && kinds[KIND_WATER] == 0) return;

	if (Library.Count == Library.Max) {
		Library.Max = Library.Max ? Library.Max * 2 : 256;
		Library.Features = (Feature *)realloc(Library.Features, Library.Max * sizeof(Feature));
	}
	if (Library.CellCount + count > Library.CellMax) {
		while (Library.CellCount + count > Library.CellMax) {
			Library.CellMax = Library.CellMax ? Library.CellMax * 2 : 8192;
		}
		Library.Cells = (FeatureCell *)realloc(Library.Cells, Library.CellMax * sizeof(FeatureCell));
	}
	if (Library.Features == NULL || Library.Cells == NULL) {
		Library.Count = Library.Max = Library.CellCount = Library.CellMax = 0;
		return;
	}

	Feature & feature = Library.Features[Library.Count++];
	feature.Width = maxx - minx + 1;
	feature.Height = maxy - miny + 1;
	feature.Left = x0 - minx;
	feature.Top = y0 - miny;
	feature.Right = x0 + w - 1 - minx;
	feature.Bottom = y0 + h - 1 - miny;
	feature.First = Library.CellCount;
	feature.Count = count;
	feature.Edges = 0;
	if (minx == x0) feature.Edges |= EDGE_LEFT;
	if (miny == y0) feature.Edges |= EDGE_TOP;
	if (maxx == x0 + w - 1) feature.Edges |= EDGE_RIGHT;
	if (maxy == y0 + h - 1) feature.Edges |= EDGE_BOTTOM;
	if (kinds[KIND_WATER]) {
		feature.Kind = KIND_WATER;
	} else if (kinds[KIND_RIVER]) {
		feature.Kind = KIND_RIVER;
	} else if (kinds[KIND_CLIFF] >= 3) {
		feature.Kind = KIND_CLIFF;
	} else {
		feature.Kind = KIND_DECOR;
	}

	for (int index = 0; index < count; index++) {
		FeatureCell & fc = Library.Cells[Library.CellCount++];
		CELL cell = cells[index];
		fc.X = Cell_X(cell) - minx;
		fc.Y = Cell_Y(cell) - miny;
		fc.TType = SourceTType[cell];
		fc.TIcon = SourceTIcon[cell];
		fc.Land = TemplateTypeClass::As_Reference((TemplateType)fc.TType).Land_Type(fc.TIcon);
	}
}


/*
**	Adds the features of one scenario, if it is of the theater.
*/
static void Library_Add_Map(char const * name, TheaterType theater)
{
	CCFileClass file(name);
	if (!file.Is_Available()) return;
	CCINIClass ini;
	if (!ini.Load(file, false)) return;
	if (ini.Get_TheaterType("Map", "Theater", THEATER_NONE) != theater) return;

	int x0 = Bound(ini.Get_Int("Map", "X", 1), 0, MAP_CELL_W - 1);
	int y0 = Bound(ini.Get_Int("Map", "Y", 1), 0, MAP_CELL_H - 1);
	int w = Bound(ini.Get_Int("Map", "Width", MAP_CELL_W - 2), 1, MAP_CELL_W - x0);
	int h = Bound(ini.Get_Int("Map", "Height", MAP_CELL_H - 2), 1, MAP_CELL_H - y0);

	int len = ini.Get_UUBlock("MapPack", Staging, sizeof(Staging));
	if (len <= 0) return;
	BufferStraw bstraw(Staging, len);
	LCWStraw decomp(LCWStraw::DECOMPRESS);
	decomp.Get_From(&bstraw);
	if (ini.Get_Int("Basic", "NewINIFormat", 0) >= 3) {
		if (decomp.Get(SourceTType, sizeof(SourceTType)) != sizeof(SourceTType)) return;
		if (decomp.Get(SourceTIcon, sizeof(SourceTIcon)) != sizeof(SourceTIcon)) return;
	} else {
		for (CELL cell = 0; cell < MAP_CELL_TOTAL; cell++) {
			if (decomp.Get(&SourceTType[cell], 2) != 2) return;
			if (decomp.Get(&SourceTIcon[cell], 1) != 1) return;
		}
	}

	/*
	**	Each group of pieces that touch (also diagonally) is a feature.
	*/
	static unsigned char seen[MAP_CELL_TOTAL];
	static CELL stack[MAP_CELL_TOTAL];
	static CELL group[MAP_CELL_TOTAL];
	memset(seen, 0, sizeof(seen));
	for (int y = y0; y < y0 + h; y++) {
		for (int x = x0; x < x0 + w; x++) {
			CELL cell = XY_Cell(x, y);
			if (seen[cell] || Is_Clear_Template(SourceTType[cell])) continue;

			int count = 0;
			int top = 0;
			stack[top++] = cell;
			seen[cell] = 1;
			while (top > 0) {
				CELL c = stack[--top];
				group[count++] = c;
				for (int dy = -1; dy <= 1; dy++) {
					for (int dx = -1; dx <= 1; dx++) {
						int nx = Cell_X(c) + dx;
						int ny = Cell_Y(c) + dy;
						if (nx < x0 || ny < y0 || nx >= x0 + w || ny >= y0 + h) continue;
						CELL n = XY_Cell(nx, ny);
						if (!seen[n] && !Is_Clear_Template(SourceTType[n])) {
							seen[n] = 1;
							stack[top++] = n;
						}
					}
				}
			}
			Library_Add(group, count, theater, x0, y0, w, h);
		}
	}
}


/*
**	The theater's art at hand (the land of each piece is in its art), and the library built
**	from the scenarios there are, once for each theater.
*/
static void Library_Build(TheaterType theater)
{
	/*
	**	The theater's data, as for a scenario of it. It changes the game's palette (not the
	**	screen's): put that back. The next scenario loads all of its theater's art again
	**	(LastTheater): only the templates' are loaded here.
	*/
	PaletteClass gamepal = GamePalette;
	PaletteClass original = OriginalPalette;
	Map.Init_Theater(theater);
	TemplateTypeClass::Init(theater);
	LastTheater = THEATER_NONE;
	GamePalette = gamepal;
	OriginalPalette = original;

	if (Library.Theater == theater && Library.Count > 0) return;
	Library.Theater = theater;
	Library.Count = 0;
	Library.CellCount = 0;

	static char const * const prefixes[] = {"SCM", "SCG", "SCU"};
	char name[_MAX_FNAME+_MAX_EXT];
	for (int p = 0; p < ARRAY_SIZE(prefixes); p++) {
		for (int number = 1; number < 50; number++) {
			for (char version = 'A'; version <= 'E'; version++) {
				sprintf(name, "%s%02dE%c.INI", prefixes[p], number, version);
				Library_Add_Map(name, theater);
			}
		}
	}
}


/***************************************************************************************
**	The map being made: its cells, from its top left corner.
*/
enum {
	F_BASE = 0x01,						// Room for a base: nothing goes here.
	F_ORE = 0x02,						// Ore or gems.
	F_FEATURE = 0x04,					// Part of a feature.
	F_TREE = 0x08,						// Under a tree.
	F_BLOCKED = 0x10,					// Ground units can't go here.
	F_RESERVED = 0x20					// Kept clear (around an ore field's center).
};

static int Size;
static unsigned short TType[96*96];
static unsigned char TIcon[96*96];
static signed char Over[96*96];
static unsigned char Flags[96*96];
static unsigned char Seen[96*96];
static short Queue[96*96];

static int StartCount;
static int StartX[8];
static int StartY[8];
static int FieldCount;
static int FieldX[32];
static int FieldY[32];

struct TreeType {
	TerrainType Type;
	short Cell;
};
static TreeType Trees[1200];
static int TreeCount;


static bool On_Map(int x, int y)
{
	return(x >= 0 && y >= 0 && x < Size && y < Size);
}


/*
**	Can ground units get from the first start position to every other, and to every ore
**	field?
*/
static bool Is_Connected(void)
{
	memset(Seen, 0, Size * Size);
	int head = 0, tail = 0;
	int first = StartY[0] * Size + StartX[0];
	if (Flags[first] & F_BLOCKED) return(false);
	Queue[tail++] = first;
	Seen[first] = 1;
	while (head < tail) {
		int c = Queue[head++];
		int x = c % Size;
		int y = c / Size;
		static int const dx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
		static int const dy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
		for (int d = 0; d < 8; d++) {
			int nx = x + dx[d];
			int ny = y + dy[d];
			if (!On_Map(nx, ny)) continue;
			int n = ny * Size + nx;
			if (Seen[n] || (Flags[n] & F_BLOCKED)) continue;

			/*
			**	Not diagonally between two blocked cells.
			*/
			if (d >= 4 && (Flags[y * Size + nx] & F_BLOCKED) && (Flags[ny * Size + x] & F_BLOCKED)) continue;
			Seen[n] = 1;
			Queue[tail++] = n;
		}
	}
	for (int index = 1; index < StartCount; index++) {
		if (!Seen[StartY[index] * Size + StartX[index]]) return(false);
	}
	for (int index = 0; index < FieldCount; index++) {
		if (!Seen[FieldY[index] * Size + FieldX[index]]) return(false);
	}
	return(true);
}


/*
**	Marks a round area.
*/
static void Mark_Area(int cx, int cy, int radius, unsigned char flag)
{
	for (int y = cy - radius; y <= cy + radius; y++) {
		for (int x = cx - radius; x <= cx + radius; x++) {
			if (On_Map(x, y) && (x-cx)*(x-cx) + (y-cy)*(y-cy) <= radius*radius + radius) {
				Flags[y * Size + x] |= flag;
			}
		}
	}
}


/*
**	An ore (or gem) field: a blob of about "cells" cells around cx,cy.
*/
static void Make_Field(int cx, int cy, int cells, bool gems)
{
	cx = Bound(cx, 3, Size - 4);
	cy = Bound(cy, 3, Size - 4);
	if (FieldCount < ARRAY_SIZE(FieldX)) {
		FieldX[FieldCount] = cx;
		FieldY[FieldCount] = cy;
		FieldCount++;
	}
	Mark_Area(cx, cy, 2, F_RESERVED);

	int placed = 0;
	for (int tries = 0; tries < cells * 20 && placed < cells; tries++) {
		int x, y;
		if (placed == 0) {
			x = cx;
			y = cy;
		} else {
			/*
			**	Next to a cell of the field, and not too far from its center.
			*/
			int reach = 2 + (int)sqrt((double)cells / 3.0);
			x = cx + Rnd(-reach, reach);
			y = cy + Rnd(-reach, reach);
			if (!On_Map(x, y)) continue;
			bool next = false;
			for (int d = 0; d < 4 && !next; d++) {
				int nx = x + (d == 0) - (d == 1);
				int ny = y + (d == 2) - (d == 3);
				if (On_Map(nx, ny) && (Flags[ny * Size + nx] & F_ORE)) next = true;
			}
			if (!next) continue;
		}
		if (x < 2 || y < 2 || x >= Size - 2 || y >= Size - 2) continue;
		int c = y * Size + x;
		if (Flags[c] & (F_ORE|F_BASE|F_FEATURE|F_BLOCKED)) continue;
		Flags[c] |= F_ORE|F_RESERVED;
		Over[c] = gems ? (signed char)Rnd(OVERLAY_GEMS1, OVERLAY_GEMS4) : (signed char)Rnd(OVERLAY_GOLD1, OVERLAY_GOLD4);
		placed++;
	}
}


/*
**	Does the feature fit with its top left corner at ox,oy? Its cells must land on clear
**	ground that is free, and the ground around them (in the map) must be clear ground in its
**	own map too: nothing of it may stick out, and no other feature may touch it. Where it
**	reached the edge of its map, it must reach the edge of this one.
*/
static unsigned char Shape[100*100];

static bool Feature_Fits(Feature const & f, int ox, int oy)
{
	if (f.Width > 100 || f.Height > 100) return(false);
	memset(Shape, 0, f.Width * f.Height);
	for (int index = 0; index < f.Count; index++) {
		FeatureCell const & fc = Library.Cells[f.First + index];
		Shape[fc.Y * f.Width + fc.X] = 1;
	}

	int inside = 0;
	for (int index = 0; index < f.Count; index++) {
		FeatureCell const & fc = Library.Cells[f.First + index];
		int x = ox + fc.X;
		int y = oy + fc.Y;
		if (!On_Map(x, y)) continue;
		inside++;
		if (Flags[y * Size + x] & (F_BASE|F_ORE|F_FEATURE|F_TREE|F_RESERVED)) return(false);

		for (int dy = -1; dy <= 1; dy++) {
			for (int dx = -1; dx <= 1; dx++) {
				if (dx == 0 && dy == 0) continue;
				int nx = x + dx;
				int ny = y + dy;
				if (!On_Map(nx, ny)) continue;
				int sx = fc.X + dx;
				int sy = fc.Y + dy;
				if (sx >= 0 && sy >= 0 && sx < f.Width && sy < f.Height && Shape[sy * f.Width + sx]) continue;
				if (sx < f.Left || sy < f.Top || sx > f.Right || sy > f.Bottom) return(false);
				if (Flags[ny * Size + nx] & (F_FEATURE|F_TREE)) return(false);
			}
		}
	}

	/*
	**	Not just a sliver of it.
	*/
	return(inside * 2 >= f.Count);
}


/*
**	Puts the feature in (or takes it out again).
*/
static void Feature_Place(Feature const & f, int ox, int oy, bool place)
{
	for (int index = 0; index < f.Count; index++) {
		FeatureCell const & fc = Library.Cells[f.First + index];
		int x = ox + fc.X;
		int y = oy + fc.Y;
		if (!On_Map(x, y)) continue;
		int c = y * Size + x;
		if (place) {
			TType[c] = fc.TType;
			TIcon[c] = fc.TIcon;
			Flags[c] |= F_FEATURE;
			if (fc.Land == LAND_WATER || fc.Land == LAND_RIVER || fc.Land == LAND_ROCK) {
				Flags[c] |= F_BLOCKED;
			}
		} else {
			TType[c] = (unsigned short)TEMPLATE_NONE;
			TIcon[c] = 0;
			Flags[c] &= ~(F_FEATURE|F_BLOCKED);
		}
	}
}


/*
**	Where a feature that reached edges of its map may go: those edges at the same edges here.
**	Returns false if it can't.
*/
static bool Feature_Position(Feature const & f, int & ox, int & oy)
{
	int margin = 2;
	if ((f.Edges & EDGE_LEFT) && (f.Edges & EDGE_RIGHT)) {
		if (f.Width < Size) return(false);
		ox = -Rnd(0, f.Width - Size);
	} else if (f.Edges & EDGE_LEFT) {
		ox = 0;
	} else if (f.Edges & EDGE_RIGHT) {
		ox = Size - f.Width;
	} else {
		if (f.Width > Size - 2*margin) return(false);
		ox = Rnd(margin, Size - margin - f.Width);
	}
	if ((f.Edges & EDGE_TOP) && (f.Edges & EDGE_BOTTOM)) {
		if (f.Height < Size) return(false);
		oy = -Rnd(0, f.Height - Size);
	} else if (f.Edges & EDGE_TOP) {
		oy = 0;
	} else if (f.Edges & EDGE_BOTTOM) {
		oy = Size - f.Height;
	} else {
		if (f.Height > Size - 2*margin) return(false);
		oy = Rnd(margin, Size - margin - f.Height);
	}
	return(true);
}


/*
**	Places features of a kind, at random, until "cells" more cells are covered (or it runs
**	out of tries). Only features of up to "most" cells.
*/
static int Feature_Weight(Feature const & f, FeatureKind kind, int most)
{
	if (f.Kind != kind || f.Count > most) return(0);
	int weight = (kind == KIND_CLIFF) ? 4 + f.Count / 4 : 4;
	if (f.Edges != 0) weight = max(1, weight / 3);
	return(weight);
}

static int Place_Features(FeatureKind kind, int cells, int most, int tries)
{
	int total = 0;
	for (int index = 0; index < Library.Count; index++) {
		total += Feature_Weight(Library.Features[index], kind, most);
	}
	if (total == 0) return(0);

	int covered = 0;
	while (tries-- > 0 && covered < cells) {
		int pick = (int)(Rnd() % (unsigned)total);
		Feature const * f = NULL;
		for (int index = 0; index < Library.Count; index++) {
			Feature const & feature = Library.Features[index];
			pick -= Feature_Weight(feature, kind, most);
			if (pick < 0) {
				f = &feature;
				break;
			}
		}
		if (f == NULL) break;

		int ox, oy;
		if (!Feature_Position(*f, ox, oy) || !Feature_Fits(*f, ox, oy)) continue;
		Feature_Place(*f, ox, oy, true);
		if (!Is_Connected()) {
			Feature_Place(*f, ox, oy, false);
			continue;
		}
		for (int index = 0; index < f->Count; index++) {
			FeatureCell const & fc = Library.Cells[f->First + index];
			if (On_Map(ox + fc.X, oy + fc.Y)) covered++;
		}
	}
	return(covered);
}


/*
**	Clumps of trees, on free ground, never in the way.
*/
static void Place_Trees(TheaterType theater, int clumps)
{
	TerrainType types[TERRAIN_COUNT];
	int typecount = 0;
	for (TerrainType t = TERRAIN_TREE1; t <= TERRAIN_CLUMP5; t++) {
		TerrainTypeClass const & ttype = TerrainTypeClass::As_Reference(t);
		if ((ttype.Theater & (1 << theater)) && !ttype.IsWaterBased) {
			types[typecount++] = t;
		}
	}
	if (typecount == 0) return;

	for (int clump = 0; clump < clumps; clump++) {
		int cx = Rnd(2, Size - 3);
		int cy = Rnd(2, Size - 3);
		int first = TreeCount;
		int count = Rnd(2, 9);
		int radius = 1 + count / 3;
		for (int index = 0; index < count * 3 && TreeCount - first < count && TreeCount < ARRAY_SIZE(Trees); index++) {
			int x = cx + Rnd(-radius, radius);
			int y = cy + Rnd(-radius, radius);
			TerrainType type = types[Rnd(0, typecount - 1)];
			if (type >= TERRAIN_CLUMP1 && Rnd(0, 2) != 0) {
				type = types[Rnd(0, typecount - 1)];
			}

			/*
			**	All the cells it stands on free, and away from bases and ore.
			*/
			short const * occupy = TerrainTypeClass::As_Reference(type).Occupy_List();
			bool ok = true;
			int cells[16];
			int n = 0;
			for (; *occupy != REFRESH_EOL && ok && n < ARRAY_SIZE(cells); occupy++) {
				int dy = (*occupy + MAP_CELL_W/2) / MAP_CELL_W;
				if (*occupy + MAP_CELL_W/2 < 0) dy--;
				int dx = *occupy - dy * MAP_CELL_W;
				int ox = x + dx;
				int oy = y + dy;
				if (ox < 1 || oy < 1 || ox >= Size - 1 || oy >= Size - 1) {
					ok = false;
					break;
				}
				int c = oy * Size + ox;
				if (Flags[c] & (F_BASE|F_ORE|F_FEATURE|F_TREE|F_BLOCKED|F_RESERVED)) ok = false;
				cells[n++] = c;
			}
			if (!ok || n == 0 || *occupy != REFRESH_EOL) continue;
			for (int k = 0; k < n; k++) {
				Flags[cells[k]] |= F_TREE|F_BLOCKED;
			}
			Trees[TreeCount].Type = type;
			Trees[TreeCount].Cell = (short)(y * Size + x);
			TreeCount++;
		}

		/*
		**	A clump that cuts a way off doesn't stay.
		*/
		if (!Is_Connected()) {
			while (TreeCount > first) {
				TreeCount--;
				int x = Trees[TreeCount].Cell % Size;
				int y = Trees[TreeCount].Cell / Size;
				short const * occupy = TerrainTypeClass::As_Reference(Trees[TreeCount].Type).Occupy_List();
				for (; *occupy != REFRESH_EOL; occupy++) {
					int dy = (*occupy + MAP_CELL_W/2) / MAP_CELL_W;
					if (*occupy + MAP_CELL_W/2 < 0) dy--;
					int dx = *occupy - dy * MAP_CELL_W;
					if (On_Map(x + dx, y + dy)) {
						Flags[(y + dy) * Size + (x + dx)] &= ~(F_TREE|F_BLOCKED);
					}
				}
			}
		}
	}
}


/*
**	The start positions: around the middle of the map, evenly apart, each with room for a
**	base and an ore field of its own, further ore between them and gems in the middle.
*/
static void Place_Starts(int starts)
{
	StartCount = starts;
	double const pi = 3.14159265358979;
	double turn = (Rnd() % 3600) * pi / 1800.0;
	double radius = Size * 0.36;
	int margin = 8;

	for (int index = 0; index < starts; index++) {
		double angle = turn + index * 2.0 * pi / starts + ((int)(Rnd() % 21) - 10) * pi / 360.0;
		int x = Size / 2 + (int)(cos(angle) * radius);
		int y = Size / 2 + (int)(sin(angle) * radius);
		StartX[index] = Bound(x, margin, Size - 1 - margin);
		StartY[index] = Bound(y, margin, Size - 1 - margin);
		Mark_Area(StartX[index], StartY[index], 6, F_BASE);
	}

	for (int index = 0; index < starts; index++) {
		/*
		**	The base's own field: off to one side of the way to the middle.
		*/
		double angle = atan2((double)(Size / 2 - StartY[index]), (double)(Size / 2 - StartX[index]));
		angle += ((Rnd() & 1) ? 1 : -1) * (40 + Rnd(0, 30)) * pi / 180.0;
		int distance = Rnd(9, 11);
		Make_Field(StartX[index] + (int)(cos(angle) * distance), StartY[index] + (int)(sin(angle) * distance), Rnd(26, 34), false);
	}

	for (int index = 0; index < starts; index++) {
		/*
		**	Between neighbors, somewhat towards the middle.
		*/
		int next = (index + 1) % starts;
		int mx = (StartX[index] + StartX[next]) / 2;
		int my = (StartY[index] + StartY[next]) / 2;
		mx += (Size / 2 - mx) / 3;
		my += (Size / 2 - my) / 3;
		Make_Field(mx, my, Rnd(18, 26), false);
	}
	Make_Field(Size / 2, Size / 2, Rnd(14, 20), true);
}


/*
**	Writes the map as a scenario.
*/
static bool Write_Map(char const * filename, char const * name, TheaterType theater)
{
	int x0 = (MAP_CELL_W - Size) / 2;
	int y0 = (MAP_CELL_H - Size) / 2;

	CCINIClass ini;
	static char const * const BASIC = "Basic";
	ini.Put_String(BASIC, "Name", name);
	ini.Put_VQType(BASIC, "Intro", VQ_NONE);
	ini.Put_VQType(BASIC, "Brief", VQ_NONE);
	ini.Put_VQType(BASIC, "Win", VQ_NONE);
	ini.Put_VQType(BASIC, "Lose", VQ_NONE);
	ini.Put_VQType(BASIC, "Action", VQ_NONE);
	ini.Put_HousesType(BASIC, "Player", HOUSE_MULTI1);
	ini.Put_ThemeType(BASIC, "Theme", THEME_NONE);
	ini.Put_Int(BASIC, "NewINIFormat", 3);
	ini.Put_Bool(BASIC, "Official", false);

	static char const * const MAP = "Map";
	ini.Put_TheaterType(MAP, "Theater", theater);
	ini.Put_Int(MAP, "X", x0);
	ini.Put_Int(MAP, "Y", y0);
	ini.Put_Int(MAP, "Width", Size);
	ini.Put_Int(MAP, "Height", Size);

	char entry[16];
	for (int index = 0; index < StartCount; index++) {
		sprintf(entry, "%d", index);
		ini.Put_Int("Waypoints", entry, XY_Cell(x0 + StartX[index], y0 + StartY[index]));
	}
	sprintf(entry, "%d", WAYPT_HOME);
	ini.Put_Int("Waypoints", entry, XY_Cell(x0 + StartX[0], y0 + StartY[0]));
	sprintf(entry, "%d", WAYPT_REINF);
	ini.Put_Int("Waypoints", entry, XY_Cell(x0 + Size / 2, y0 + Size / 2));

	/*
	**	The terrain pieces, then the overlays (ore), as the game packs them.
	*/
	static unsigned short ttype[MAP_CELL_TOTAL];
	static unsigned char ticon[MAP_CELL_TOTAL];
	static signed char over[MAP_CELL_TOTAL];
	for (CELL cell = 0; cell < MAP_CELL_TOTAL; cell++) {
		ttype[cell] = (unsigned short)TEMPLATE_NONE;
		ticon[cell] = 0;
		over[cell] = (signed char)OVERLAY_NONE;
	}
	for (int y = 0; y < Size; y++) {
		for (int x = 0; x < Size; x++) {
			CELL cell = XY_Cell(x0 + x, y0 + y);
			ttype[cell] = TType[y * Size + x];
			ticon[cell] = TIcon[y * Size + x];
			over[cell] = Over[y * Size + x];
		}
	}

	BufferPipe mappipe(Staging, sizeof(Staging));
	LCWPipe mapcomp(LCWPipe::COMPRESS);
	mapcomp.Put_To(&mappipe);
	int len = mapcomp.Put(ttype, sizeof(ttype));
	len += mapcomp.Put(ticon, sizeof(ticon));
	len += mapcomp.Flush();
	ini.Put_UUBlock("MapPack", Staging, len);

	BufferPipe overpipe(Staging, sizeof(Staging));
	LCWPipe overcomp(LCWPipe::COMPRESS);
	overcomp.Put_To(&overpipe);
	len = overcomp.Put(over, sizeof(over));
	len += overcomp.Flush();
	ini.Put_UUBlock("OverlayPack", Staging, len);

	for (int index = 0; index < TreeCount; index++) {
		int x = Trees[index].Cell % Size;
		int y = Trees[index].Cell / Size;
		sprintf(entry, "%d", XY_Cell(x0 + x, y0 + y));
		ini.Put_String("TERRAIN", entry, TerrainTypeClass::As_Reference(Trees[index].Type).Name());
	}

	RawFileClass file(filename);
	return(ini.Save(file, true) != 0);
}


/*
**	Makes a random map: the theater, start positions and ore, features, trees.
*/
static bool Make_Map(RandomMapType const & def, unsigned seed)
{
	RandomState = seed ? seed : 0x2545F491;
	for (int index = 0; index < 8; index++) Rnd();

	TheaterType theater = (Rnd() & 1) ? THEATER_SNOW : THEATER_TEMPERATE;
	Library_Build(theater);

	Size = def.Size;
	for (int index = 0; index < Size * Size; index++) {
		TType[index] = (unsigned short)TEMPLATE_NONE;
		TIcon[index] = 0;
		Over[index] = (signed char)OVERLAY_NONE;
		Flags[index] = 0;
	}
	FieldCount = 0;
	TreeCount = 0;

	Place_Starts(def.Starts);

	/*
	**	Features: some water (a lake, or a coast on a bigger map), maybe a river, cliffs,
	**	then the small things, to cover about a seventh of the map.
	*/
	int area = Size * Size;
	int covered = 0;
	if (Rnd(0, 3) != 0) {
		covered += Place_Features(KIND_WATER, area / 12, area / (Size >= 80 ? 7 : 12), 80);
	}
	if (Size >= 80 && Rnd(0, 2) == 0) {
		covered += Place_Features(KIND_RIVER, area / 14, area / 10, 60);
	}
	covered += Place_Features(KIND_CLIFF, max(area / 10, area / 7 - covered), area / 30, 400);
	covered += Place_Features(KIND_DECOR, area / 40, 60, 200);

	Place_Trees(theater, area / 200 + Rnd(0, area / 400));

	return(Write_Map(def.Filename, def.Description, theater));
}


/***********************************************************************************************
 * Random_Map_Add_Entries -- Adds the random maps to the scenario list.                        *
 *                                                                                             *
 *    At the end of the list. The digest is that of the map made last time, if there is one.   *
 *=============================================================================================*/
void Random_Map_Add_Entries(void)
{
	for (int index = 0; index < ARRAY_SIZE(RandomMaps); index++) {
		char digest[32] = "";
		CCFileClass file(RandomMaps[index].Filename);
		if (file.Is_Available()) {
			INIClass ini;
			ini.Load(file);
			ini.Get_String("Digest", "1", "", digest, sizeof(digest));
		}
		Session.Scenarios.Add(new MultiMission(RandomMaps[index].Filename, RandomMaps[index].Description, digest, false, false));
	}
}


/***********************************************************************************************
 * Random_Map_Is -- Is this the file of a random map (one of the list's)?                      *
 *=============================================================================================*/
bool Random_Map_Is(char const * filename)
{
	for (int index = 0; index < ARRAY_SIZE(RandomMaps); index++) {
		if (filename != NULL && stricmp(filename, RandomMaps[index].Filename) == 0) return(true);
	}
	return(false);
}


/***********************************************************************************************
 * Random_Map_Generate -- Makes a new random map for a scenario list entry, if it is one.      *
 *                                                                                             *
 *    INPUT:   scenario -- The index into Session.Scenarios.                                   *
 *                                                                                             *
 *             only_if_missing -- Only if there is no map yet (when the game starts).          *
 *                                                                                             *
 *    OUTPUT:  Was a map made?                                                                 *
 *=============================================================================================*/
bool Random_Map_Generate(int scenario, bool only_if_missing)
{
	if (scenario < 0 || scenario >= Session.Scenarios.Count()) return(false);
	MultiMission * mission = Session.Scenarios[scenario];
	for (int index = 0; index < ARRAY_SIZE(RandomMaps); index++) {
		if (stricmp(mission->Get_Filename(), RandomMaps[index].Filename) != 0) continue;

		if (only_if_missing && CCFileClass(RandomMaps[index].Filename).Is_Available()) return(false);

		/*
		**	RA_RANDOM_SEED makes the same map each time (for testing).
		*/
		unsigned seed = (unsigned)time(NULL) * 2654435761U ^ (unsigned)clock() ^ (unsigned)(Frame * 40503);
		if (getenv("RA_RANDOM_SEED") != NULL) {
			seed = (unsigned)strtoul(getenv("RA_RANDOM_SEED"), NULL, 0);
		}
		if (!Make_Map(RandomMaps[index], seed)) return(false);

		char digest[32] = "";
		CCFileClass file(RandomMaps[index].Filename);
		INIClass ini;
		ini.Load(file);
		ini.Get_String("Digest", "1", "", digest, sizeof(digest));
		mission->Set_Digest(digest);
		return(true);
	}
	return(false);
}

#endif
