# Command & Conquer Red Alert — modern port

This is the Command & Conquer Red Alert source code released by Electronic Arts,
ported so that it builds with a modern compiler (GCC or Clang, 64-bit) and runs on
current systems, Linux and Windows, through [SDL2](https://www.libsdl.org/). It is meant
to be a base for modding and extending the game.

The game logic is the original code, with the Windows 95 version's code path
(640x400 hi-res graphics). What changed is what sits around it:

- the Watcom C++ / TASM build became a CMake build, and every assembly routine was
  rewritten in C++;
- DirectDraw, DirectSound and the Windows message loop are replaced by a small SDL2
  platform layer;
- the 32-bit assumptions of the code were fixed so that it runs as a 64-bit program.

## Status

Working:

- intro and briefing movies (VQA), with sound;
- main menu, dialogs, sidebar, mouse cursor;
- both campaigns, Allied and Soviet: every mission loads and runs, and so does
  everything between missions (closing movie, score screen, map selection, briefing),
  losing and replaying a mission, and the ending with the credits;
- skirmish against the computer, including the end-of-game score table;
- saving and loading games;
- LAN games ("Network" in the multiplayer menu), over UDP: tested with two players;
- sound effects, speech and music;
- window or fullscreen, scaled up, optionally stretched to 4:3 as on a CRT monitor.

Not available:

- Modem / serial games (the serial library was not released), and Westwood Online
  internet games (the service no longer exists).
- The DVD edition's MPEG movies (the original VQA movies are played instead).

Not tested:

- The Counterstrike and Aftermath expansions (they are not part of EA's freeware
  release).
- Playing missions through by hand: the campaigns were tested by an automated player
  that wins each mission after a few seconds, under AddressSanitizer and UBSan.

Tested on Linux (x86-64); CI builds it with GCC and Clang. The Windows version (64-bit,
built with MinGW-w64) was tested under Wine 9: menus, movies with sound, a mission,
saving and loading, a LAN game against the Linux version, the installer and uninstaller.
It has also been installed and played on a Windows PC. The code is written to be portable
to other SDL2 platforms (macOS...), but nothing else has been tried.

## Building

You need CMake 3.16 or later, GCC or Clang (tested with GCC 13 and Clang 18) and the
SDL2 development files.

On Debian or Ubuntu:

```sh
sudo apt install build-essential cmake ninja-build libsdl2-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

This produces `build/redalert`. To build with Clang, set `CC=clang CXX=clang++` when
running the first `cmake` command.

Build options (`-D<option>=ON` or `OFF`):

| Option | Effect |
| --- | --- |
| `RA_ENHANCEMENTS` | On by default: build in the optional [enhancements](#enhancements). Each is still off until the player turns it on. |
| `RA_DEVELOPER` | On by default: build in Westwood's scenario editor and developer keys (`-editor`, `-cheats`; see [Modding](#modding)). |
| `RA_LANGUAGE_GERMAN`, `RA_LANGUAGE_FRENCH` | Build a German or French version (the data must match). |
| `RA_WARNINGS` | Show compiler warnings (off by default: the 1996 code produces thousands). |
| `RA_SANITIZE` | Build with AddressSanitizer and UndefinedBehaviorSanitizer. With `ASAN_OPTIONS=halt_on_error=0` a run reports every memory error instead of stopping at the first. |

### Windows

The Windows version is cross-compiled with MinGW-w64, from Linux (or WSL on Windows).
You also need SDL2's MinGW development package, `SDL2-devel-2.30.x-mingw.tar.gz` from
[SDL's releases](https://github.com/libsdl-org/SDL/releases), unpacked somewhere:

```sh
sudo apt install cmake ninja-build mingw-w64
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake \
    -DCMAKE_PREFIX_PATH=/path/to/SDL2-2.30.9/x86_64-w64-mingw32
cmake --build build-win
```

This produces `build-win/redalert.exe`, which needs `SDL2.dll` (in the package's
`x86_64-w64-mingw32/bin`) next to it. The C and C++ runtimes are linked in.

CI builds the Windows version on every push: the program and an installer are under the
run's artifacts in the repository's Actions tab. The latest build of the `main` branch
(and of the branch this port was developed on) is also on the Releases page, as a
pre-release with the installer and the program; neither contains the game data.

### Windows installer

`tools/windows/make-installer.sh` makes an installer from a Windows build. It needs NSIS
(`sudo apt install nsis`) and, so that the installer can download the game data, 7-Zip's
64-bit Windows files `7z.exe`, `7z.dll` and `License.txt`. Unpack those from 7-Zip's
installer, from [7-Zip's releases](https://github.com/ip7z/7zip/releases)
(`7z x 7z2501-x64.exe 7z.exe 7z.dll License.txt`):

```sh
tools/windows/make-installer.sh --7zip path/to/7zip build-win
```

The installer installs the game for the current user (in
`%LOCALAPPDATA%\Programs\Red Alert`; the game writes its settings and saved games next
to its data, so it needs a folder of its own), with Start menu and desktop shortcuts and
an uninstaller. The game data is EA's freeware release of the two discs, which the
installer either:

- downloads from the Internet Archive (see Game data), checks against the files'
  SHA-256 digests and unpacks with 7-Zip, which the installer contains (a download that
  fails is resumed when the installer runs again); or
- copies from the discs: real discs, disc images (`.iso`; Windows opens them as a drive
  when you double-click them) or folders holding a disc's files.

Without `--7zip` the installer only copies from discs.

For an unattended install: `redalert-setup.exe /S /DATA=download`, or
`redalert-setup.exe /S /ALLIED=E:\ /SOVIET=F:\` for discs. `/D=C:\Games\Red Alert`
chooses the folder (it must come last, without quotes), and `/MIRROR=URL` downloads the
archives from another place (the files must be the same).

Alternatively, the installer can contain the game data itself, from a data directory
like the one described under Game data, and EA's license agreement for it (the file
`EA Product EULA RedAlert1.rtf` in EA's freeware download):

```sh
tools/windows/make-installer.sh --data ~/redalert-data \
    --eula "EA Product EULA RedAlert1.rtf" build-win
```

Such an installer is for your own computers only: EA's license lets you use the game,
but not distribute it, so don't publish an installer that contains the data.

## Game data

The game data is not part of this repository, nor of its releases: EA released Red Alert
as freeware in 2008, but its license lets you use the game, not distribute it.

EA's download page is gone; the Internet Archive keeps a copy of the freeware release,
[Command & Conquer Classics Original Freeware Releases](https://archive.org/details/official-cn-ctiberian-sun_202510):

| Download | Size | SHA-1 |
| --- | --- | --- |
| [`RedAlert1_AlliedDisc.rar`](https://archive.org/download/official-cn-ctiberian-sun_202510/RedAlert1_AlliedDisc.rar) | 524 MB | `30f84e4314365a9f1706cfd7359d638de43689ba` |
| [`RedAlert1_SovietDisc.rar`](https://archive.org/download/official-cn-ctiberian-sun_202510/RedAlert1_SovietDisc.rar) | 526 MB | `3df92a8abfd7c5a24d9c8f4c6fd1300da47105f9` |

They hold the disc images (`CD1_ALLIED_DISC.ISO`, `CD2_SOVIET_DISC.ISO`) and EA's license
agreement. Unpack them with 7-Zip on Windows, or `unar` or `unrar`. `bsdtar` (libarchive)
reports a CRC error on the disc images, so Windows' own "Extract all", which uses
libarchive too, may fail as well; and Linux builds of 7-Zip without the RAR codec can't
read them. The data also comes with the C&C Ultimate Collection.

On Windows, the installer downloads the data, or copies it from the discs (see Windows
installer above). Otherwise you need:

| File | Where it comes from |
| --- | --- |
| `REDALERT.MIX` | `INSTALL/REDALERT.MIX` on either disc |
| `CD1/MAIN.MIX` | `MAIN.MIX` of the Allied disc |
| `CD2/MAIN.MIX` | `MAIN.MIX` of the Soviet disc (for the Soviet campaign) |

Put them in a directory of your choice:

```
redalert-data/
    REDALERT.MIX
    CD1/MAIN.MIX
    CD2/MAIN.MIX
```

`tools/install-data.sh` does this for you from the CD images (`.iso`, read with 7-Zip)
or from directories holding the discs' files:

```sh
tools/install-data.sh ~/redalert-data CD1_ALLIED_DISC.ISO CD2_SOVIET_DISC.ISO
```

A disc is a directory named after it (`CD1` Allied, `CD2` Soviet, `CD3`
Counterstrike, `CD4` Aftermath). Files in the data directory itself are always found
first, so with only one disc you can also just put its `MAIN.MIX` next to
`REDALERT.MIX`.

The game writes its settings (`REDALERT.INI`) and saved games into the data
directory, so it needs to be writable.

## Running

Run the game from the data directory, or point it there:

```sh
RA_DATA_DIR=~/redalert-data ./build/redalert
```

If `RA_DATA_DIR` isn't set, the game looks for `REDALERT.MIX` in the current
directory and then next to the executable. On Windows the simplest is to put the data
next to `redalert.exe`, as the installer does.

Alt+Enter switches between window and fullscreen. Environment variables (on Windows,
set them in a command prompt with `set NAME=value` before starting `redalert.exe` from
it; the game's messages then appear in that window):

| Variable | Effect |
| --- | --- |
| `RA_DATA_DIR` | Directory holding the game data. |
| `RA_FULLSCREEN=1` | Start fullscreen (`0`: in a window, whatever `REDALERT.INI` says). |
| `RA_RESOLUTION=WxH` | The battle screen's size, such as `1280x720`, or `auto` (see "Bigger battle screen"). |
| `RA_SCALE=n` | Window size as a multiple of 640x400, or of the battle screen's size (default 2; less if that wouldn't fit). |
| `RA_ASPECT=1` | Show each pixel 1.2 times taller, as a CRT monitor did (640x400 as 4:3). |
| `RA_NO_SOUND=1` | Disable sound. |
| `RA_NO_MOUSE_GRAB=1` | Don't keep the mouse in the game's window while the game is active (it does by default, as the original did, so that the map scrolls when the mouse reaches the window's edges). |
| `RA_LAN_BROADCAST=a.b.c.d` | Broadcast address(es) for LAN games, comma separated (default 255.255.255.255). An address can also be a name, and have a port (`host:port`). |
| `RA_LAN_PORT=n` | The UDP port for network games (default 1234, or `Port=` in `REDALERT.INI`'s `[Network]` section). |
| `RA_DEBUG=1` | Print the game's debug messages to the console. |
| `RA_NO_MESSAGEBOX=1` | Don't show system message boxes (start-up errors and warnings); take their default answer. For automated runs. |
| `RA_SCREENSHOT=file.bmp` | Save one screenshot, `RA_SCREENSHOT_DELAY` milliseconds after start. |
| `RA_EXTRACT=NAME,...` | Copy the named files out of the MIX archives into the data directory (or `RA_EXTRACT_DIR`), then exit. See Modding. |

## Bigger battle screen

The original showed everything at 640x400. The battle screen can be bigger, so that more
of the battlefield is in view: in `REDALERT.INI`, add

```ini
[Video]
Resolution=1280x720
```

(or set `RA_RESOLUTION=1280x720`). Any size from 640x400 up works. The sidebar, radar and
power bar move to the right edge, the map fills the rest, and on a taller screen the
sidebar's build strips show more items (ten each at 720 lines) before they need
scrolling. A map smaller than the screen leaves a black margin. The menus, movies, score
screens and the map of the campaign stay 640x400 and fill the window as before.

`Resolution=auto` picks the size of the desktop divided by the biggest whole number that
leaves at least 400 lines (960x540 on a 1920x1080 desktop, shown at double size), and
starts fullscreen. `Fullscreen=yes` (or `no`) in the same section chooses whether the game
starts fullscreen; Alt+Enter switches at any time. Saved games load on any screen size.
Network players can each use a different size.

## LAN and internet games

Novell IPX, which the original used for network games, no longer exists, so "Network"
games use UDP broadcasts on port 1234 instead (the original code had this as an unused
option). Players on the same local network see each other's games in the lobby.

Over the internet, the host lets UDP port 1234 through to their computer (a port forward
on their router). The others type `/connect` and the host's address in the message line at
the bottom of the lobby, then <kbd>Enter</kbd> (`/connect 203.0.113.7`, or
`/connect host.example.net:1234`): the host's game appears in the list, to join as usual.
The address is remembered for the next time (`REDALERT.INI`, `[Network]` section,
`Connect=`); `Port=` there (or `RA_LAN_PORT`) changes the port. Everyone needs the same
version of the game: earlier versions can't join this one.

## Enhancements

Optional additions that the original game didn't have. Each one is off until you turn it
on, so by default the game plays as it always did: in a mission, open **Options**, **Game
Controls**, **Enhancements** (or set them in `REDALERT.INI`, section `[Enhancements]`).
In network games, every player's game must make exactly the same moves, so the host's
choice of those that change the rules (veterancy, support powers, attack-move, smarter
computer players, shared tech) applies to everyone: the host sends it with the game's
options.

Conveniences that don't change the game are always on: the mouse wheel scrolls the
sidebar column under the pointer, and lists in dialogs.

**Survival** is an option of skirmish and network games (in the list with Bases,
Crates...): the computer players get no base. Instead, every two minutes, each sends a wave
of forces, bigger each time (every fifth one bigger still), from one of the start positions
no player took, to hunt yours down. The mission timer at the top counts down to the next
wave. Hold out against 20 waves and destroy the last, and you win. More computer players
make more waves. In a network game, the players are allies against the waves.

**Co-op** is an option of network games: the players start allied with each other, against
the computer players (allied with each other too).

**Random maps**: at the end of the skirmish and network games' scenario lists, "Random Map:
Small (2-4)", "Medium (4-6)" and "Large (6-8)" make a new map each time you choose one
(choose it again for another). It's saved as `RANDOMS.MPR`, `RANDOMM.MPR` or
`RANDOML.MPR` in the game's folder, like the maps players made with the editor, so in a
network game the others get it from the host as the game starts. Red Alert's terrain is
made of pieces of art that only fit next to some others, so the shorelines, cliffs and
rivers come from the maps you have: every group of terrain pieces surrounded by clear
ground in a map of the same theater (a lake, a ridge, a river with its bridges, a rock) can
go anywhere on clear ground, and one that ran off its map's edge (a coast) can go at the
same edge of the new map. Each start position gets room for a base and its own ore field,
with more ore between them and gems in the middle; the terrain is placed so that every
start can be reached from every other by land; then come clumps of trees.

| Enhancement | `REDALERT.INI` | What it does |
| --- | --- | --- |
| Veterancy | `Veterancy=yes` | Units earn ranks from their kills: veteran after 3, elite after 7. A veteran deals 20% more damage, takes 20% less and fires 10% faster; an elite unit deals 40% more, takes 35% less, fires 25% faster and repairs itself. Selected units show their rank as chevrons above the health bar. Westwood started on this: every unit has a kill counter ("When it reaches a certain point, the unit improves", `CODE/CREW.H`), but the original game credited each kill to the victim and never used the count. |
| Build queue | `BuildQueue=yes` | Click a unit in the sidebar while its factory is busy to queue it, up to 9 for each kind of factory (infantry, vehicles, ships, aircraft); the cameo shows how many more are waiting ("+2"). Each one starts when the factory is free, just as if you clicked on it then, so this works in network games too. Right-click takes one off the queue; with none queued, it holds or cancels the one being built, as before. The queue isn't kept in saved games. |
| Shared tech | `SharedTech=yes` | In skirmish games, every side can build the units and buildings of both the Allies and the Soviets, and the computer players use them too. (The units that only appear in missions, such as Einstein or the hospital, can't be made buildable: the game has no sidebar pictures for them.) |
| Smarter computer players | `SmartAI=yes` | In skirmish games, the computer players gather their forces and attack together, in waves, when they outweigh the enemy's forces and the defenses around the target (by as much as their personality dares): at the building most worth destroying and least defended (refineries, power plants, factories), with attack-move, on to the next one, and back home to join the next wave when beaten. They raid harvesters out in the field with fast vehicles; build a mix of units against what the enemy has (anti-tank, anti-infantry, anti-aircraft, artillery against defenses); build more harvesters, up to four refineries, a radar dome early (the original only built one against aircraft, and the technology centers, helipads and airfields need it), a service depot (where they send their damaged vehicles), a navy when the enemy can be reached by sea, and the super weapons, using the iron curtain and the chronosphere on their tanks; and they rebuild a lost construction yard. Each has a personality: balanced, rusher (attacks early and often, more infantry), turtle (more defenses, bigger attacks later) or armored (more tanks). |
| Support powers | `SupportPowers=yes` | Three more special weapons in the sidebar, charged and aimed like the original ones: an artillery barrage (8 shells around the target; it needs a radar dome and a war factory, and recharges in 5 minutes), a naval bombardment from off the map (6 heavy shells; a radar dome and a naval yard or sub pen; 8 minutes) and Chinook reinforcements (a Chinook flies in, lands near the target and lets out five soldiers; a helipad and barracks; 6 minutes). The computer players use them too. |
| Attack-move | `AttackMove=yes` | Select units and hold <kbd>Ctrl</kbd>+<kbd>Shift</kbd> (as in later games) or <kbd>Ctrl</kbd>+<kbd>Alt</kbd> while you click on the ground: they move there, taking on the enemies they meet on the way, and then guard the place. (The original showed the guard area pointer for <kbd>Ctrl</kbd>+<kbd>Alt</kbd> over the ground, but nothing happened when you clicked.) |
| Modern mouse | `ModernMouse=yes` | The right mouse button gives the selected units the order that the pointer shows (move, attack, enter, deploy...), as in later games, and the left button only selects; on the ground it unselects. Placing buildings, repairing, selling and aiming special weapons work as before. As it only changes how orders are given, it works in network games too. |
| Mission select | `MissionSelect=yes` | When you start a new game and choose a side, you can start at any mission of that campaign that you have reached, or another version of it (several missions come in two or three versions, which the map screen chooses between). The furthest mission reached on each side is kept in `REDALERT.INI` (`AlliedMission`, `SovietMission`), from the first mission played with this version on. `AllMissions=yes` lists every mission. |

Mods and missions can tune them in `RULES.INI` (a scenario's own sections override
these). The defaults:

```ini
[Veterancy]
VeteranKills=3
EliteKills=7
VeteranFirepower=1.2    ; damage dealt, times this
VeteranArmor=0.8        ; damage taken, times this
VeteranROF=0.9          ; time between shots, times this
EliteFirepower=1.4
EliteArmor=0.65
EliteROF=0.75
EliteRepairDelay=60     ; game frames between repair steps of elite units
EliteRepairStep=0.02    ; part of full strength repaired in each step

[SupportPowers]
ArtilleryTime=5         ; minutes to recharge
ArtilleryShells=8
ArtilleryDamage=150     ; each shell
ArtillerySpread=2       ; cells from the target
BombardmentTime=8
BombardmentShells=6
BombardmentDamage=400
BombardmentSpread=3
AirdropTime=6
AirdropInfantry=5       ; soldiers in the Chinook (every third one with a rocket launcher)

[Survival]
FirstWave=3             ; minutes before the first wave
WaveInterval=2          ; minutes between waves
Waves=20                ; waves to survive (0: no end)
WaveWorth=1000          ; worth (cost) of the first wave's forces
WaveGrowth=600          ; and more for each wave after it (every fifth one half as much again)
```

Saved games keep the enhancements' state with the rest of the game. Most of it is saved
after the original's data, so that games saved by an earlier version still load; a change
to the original's data can't be made that way, and the support powers made one, so games
saved before them can't be loaded (as with the original game's patches). With mission
select, you can start again at the mission you were playing.

The code is in `CODE/ENHANCE.CPP` (`CODE/SMARTAI.CPP` for the computer players,
`CODE/SURVIVAL.CPP` for survival), and the original code that calls into it is marked
`RA_ENHANCE`. Build with `-DRA_ENHANCEMENTS=OFF` to leave all of it out.

## Modding

The quickest changes need no compiling at all: the game reads loose files from the
data directory before looking inside the MIX archives. The game can extract files from
its archives itself:

```sh
RA_DATA_DIR=~/redalert-data RA_EXTRACT=RULES.INI,SCG01EA.INI ./build/redalert
```

This writes the files into the data directory and exits. Edit `RULES.INI` there to
change units, weapons, costs, and so on; the edited copy is used from then on. The same
goes for scenarios (`SCG01EA.INI` is the first Allied mission, `SCU01EA.INI` the first
Soviet one), shapes and other files. Delete the loose copy to go back to the original.

Scenario files carry a message digest. If you edit one of the original scenarios, delete
its `[Digest]` section, or the game rejects it as corrupt (as the original did).

### Mod folders

To keep a mod apart from the game (and from other mods), put its files in a folder in
`mods` in the data directory, for example `mods/mymod/`, and start the game with
`-mod mymod` (on Windows, add it to the shortcut's target: `redalert.exe -mod mymod`), or
set `RA_MOD=mymod`, or put `Mod=mymod` in the `[Enhancements]` section of `REDALERT.INI`.
The game then looks in the mod's folder first for every file it opens: a `RULES.INI`
there, scenarios, shapes and so on replace the game's. `.MIX` archives in the folder are
added before the game's own (and kept in memory), so the files in them come first too; an
archive named like one of the game's (`CONQUER.MIX`, say) replaces it. The settings and
saved games stay in the data directory. In a network game, every player needs the same
mod.

### Westwood's scenario editor and developer keys

Westwood's own internal builds had a scenario editor and developer keys, which the
released game left out. This port builds them in (CMake option `RA_DEVELOPER`, on by
default); they stay off unless you ask for them on the command line:

- `-editor` starts in the scenario editor, on the first Allied mission. Right-click for
  its menu: new, load and save scenario, map size, add game objects, scenario and AI
  options, and play the scenario. <kbd>Insert</kbd> places objects, <kbd>Delete</kbd>
  removes the selected one, <kbd>F6</kbd> shows which cells are passable, <kbd>Esc</kbd>
  ends placing. In a game started with `-editor` or `-cheats`, <kbd>F2</kbd> switches
  between the editor and the game. Saved scenarios go to the data directory (put them in
  a mod's folder to use them in a mod).
- `-cheats` turns on the developer keys in the game: <kbd>Alt</kbd>+<kbd>W</kbd> wins the
  mission and <kbd>Alt</kbd>+<kbd>L</kbd> loses it, <kbd>P</kbd> charges every special
  weapon, <kbd>C</kbd> lets you build anything, <kbd>Insert</kbd> copies the selected
  object and <kbd>Delete</kbd> removes it, <kbd>`</kbd> sets off an explosion under the
  mouse, <kbd>O</kbd> and <kbd>B</kbd> bring a Hind and a Longbow, <kbd>F3</kbd> shows
  the map's tile numbers and <kbd>F7</kbd> the selected unit's ranges.

On Windows, add them to the shortcut's target (`redalert.exe -editor`).

For anything beyond that, the code is yours to change. Where things are:

| Directory | Contents |
| --- | --- |
| `CODE/` | The game: units (`UNIT.CPP`, `INFANTRY.CPP`, `AIRCRAFT.CPP`, `VESSEL.CPP`), buildings (`BUILDING.CPP`), the type data and rules (`*DATA.CPP`, `RULES.CPP`), AI (`HOUSE.CPP`, `TEAM.CPP`), triggers (`TRIGGER.CPP`, `TACTION.CPP`, `TEVENT.CPP`), menus and dialogs, the main loop (`CONQUER.CPP`), start-up (`STARTUP.CPP`, `INIT.CPP`). |
| `WIN32LIB/` | Westwood's engine library: graphic buffers and blitters, shapes, fonts, palettes, files, sound. |
| `WINVQ/` | The VQA movie player. |
| `WIN32LIB/SDL/` | The SDL2 platform layer: window, palette, mouse and keyboard (`SDLVIDEO.CPP`) and DirectSound emulation (`SDLSOUND.CPP`). |
| `compat/` | Stand-ins for the Windows/DOS API and C runtime the game used, and `portable.h`, which every file includes first. |

Changes made for the port are marked with `RA_PORT` (as `#ifdef RA_PORT` blocks or in
comments), so the original code stays recognizable next to them.

## Automated tests

`tools/autotest/` has scripts that play the game with no one at the keyboard, on a
virtual display: one plays through a campaign (winning each mission and going through
every screen in between), one runs a long skirmish between computer players. Run them
after a change, ideally with a `RA_SANITIZE` build; see
[tools/autotest/README.md](tools/autotest/README.md).

## Porting notes

Things to know when you change the code, because the original compiler behaved
differently from today's:

- **Structures are byte packed.** The game was compiled with Watcom's `/zp1`, and file
  formats and data overlays depend on it. `portable.h` applies `#pragma pack(1)` to
  all game code; the stand-in Windows headers use the natural layout. A system header
  whose structures the game hands to the C library must be included by `portable.h`,
  before the packing starts (`<sys/timeb.h>` wasn't: `ftime()` wrote two bytes past the
  game's 14-byte `struct timeb`).
- **Enums are as small as their values allow** (`-fshort-enums`), as with Watcom.
  For example, `TemplateType` is 2 bytes, which the scenario map data relies on.
- **Bit-fields are laid out the same everywhere**: the Windows build uses
  `-mno-ms-bitfields`, so that network packets and saved games (raw copies of the game
  objects, full of bit-field flags) are the same on Windows and Linux, and players on
  both can play together.
- **`long` was replaced by `int`** where the code meant 32 bits. Use `intptr_t` or
  `uintptr_t` to keep a pointer in an integer.
- **`bool` was an `int` typedef** in the original (Watcom 10.6 had no `bool`), and a few
  places kept other values in it (a message box's button number, the shutdown state).
  Those now use `int`; watch for the same when you see `bool` holding more than
  true/false.
- **String literals are writable**, as they were with Watcom; a few places modify
  them. The program's read-only data is made writable at start-up, on ELF systems and
  on Windows (`compat/compat.cpp`).
- **The order in which global objects are constructed differs between systems** (GCC on
  Linux follows the link order, MinGW the reverse). A global object that the
  constructors of other files' global objects use must be constant-initialized, or be
  marked `RA_CONSTRUCT_EARLY`. AddressSanitizer finds such uses: run a `RA_SANITIZE`
  build with `ASAN_OPTIONS=check_initialization_order=1:strict_init_order=1`. (It still
  reports `DisplayClass::TacButton` and `ScrollClass::Counter`, which are marked.)
- On Windows the program is linked with the real system libraries too, so the Win32
  functions the port emulates must not clash with them: the ones exported under the
  same name get other link names (see the top of `compat/include/windows.h`). Network
  games use the real Winsock there.
- **`operator new` may return NULL** for the pooled object classes (units, bullets,
  animations...) when their pool is full; the code checks, and `-fcheck-new` makes the
  compiler do so too.
- **The original ran its interface as fast as the computer could redraw the screen.**
  While it waits for the next game frame, the game loop takes in input and redraws, over
  and over, and some things move on a step each time round: edge scrolling, the radar's
  animations, the sidebar strips sliding. Today's computers would go round hundreds of
  times a second, so the port goes round at most once per tick of the game's 60 Hz clock
  (`Sync_Delay` in `CODE/CONQUER.CPP`), and the map scrolls at most one step per tick
  (`CODE/SCROLL.CPP`). Pace anything new that animates "each time round" with a timer.
- The game logic runs on the main thread, which also pumps the (emulated) Windows
  message queue. As on Windows, the game's timer callbacks (tick counters, sound
  buffer maintenance) run on another thread: SDL's timer thread. Sound is mixed on
  SDL's audio thread (`WIN32LIB/SDL/SDLSOUND.CPP`).

## License

This repository and its contents are licensed under the GPL v3 license, with
additional terms applied. Please see [LICENSE.md](LICENSE.md) for details. The game
data is not covered by it.

The original source release by Electronic Arts is preserved in the history of this
repository; to build it with the original tools you would need Watcom C/C++ 10.6,
Borland TASM 4.0, the DirectX 5 and DirectX Media 5.1 SDKs, the Greenleaf
Communications Library and the HMI Sound Operating System.
