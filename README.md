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
| `RA_FULLSCREEN=1` | Start fullscreen. |
| `RA_SCALE=n` | Window size as a multiple of 640x400 (default 2). |
| `RA_ASPECT=1` | Stretch 640x400 to 4:3, as a CRT monitor showed it. |
| `RA_NO_SOUND=1` | Disable sound. |
| `RA_NO_MOUSE_GRAB=1` | Don't keep the mouse in the game's window while the game is active (it does by default, as the original did, so that the map scrolls when the mouse reaches the window's edges). |
| `RA_LAN_BROADCAST=a.b.c.d` | Broadcast address(es) for LAN games, comma separated (default 255.255.255.255). |
| `RA_DEBUG=1` | Print the game's debug messages to the console. |
| `RA_NO_MESSAGEBOX=1` | Don't show system message boxes (start-up errors and warnings); take their default answer. For automated runs. |
| `RA_SCREENSHOT=file.bmp` | Save one screenshot, `RA_SCREENSHOT_DELAY` milliseconds after start. |
| `RA_EXTRACT=NAME,...` | Copy the named files out of the MIX archives into the data directory (or `RA_EXTRACT_DIR`), then exit. See Modding. |

## LAN games

Novell IPX, which the original used for network games, no longer exists, so "Network"
games use UDP broadcasts on port 1234 instead (the original code had this as an unused
option). Players on the same local network see each other's games in the lobby.

## Enhancements

Optional additions that the original game didn't have. Each one is off until you turn it
on, so by default the game plays as it always did: in a mission, open **Options**, **Game
Controls**, **Enhancements** (or set them in `REDALERT.INI`, section `[Enhancements]`).
Those that change the game rules apply to campaign and skirmish games; network games use
the original rules for now, as every player would need the same ones.

| Enhancement | `REDALERT.INI` | What it does |
| --- | --- | --- |
| Veterancy | `Veterancy=yes` | Units earn ranks from their kills: veteran after 3, elite after 7. A veteran deals 20% more damage, takes 20% less and fires 10% faster; an elite unit deals 40% more, takes 35% less, fires 25% faster and repairs itself. Selected units show their rank as chevrons above the health bar. Westwood started on this: every unit has a kill counter ("When it reaches a certain point, the unit improves", `CODE/CREW.H`), but the original game credited each kill to the victim and never used the count. |
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
```

The code is in `CODE/ENHANCE.CPP`, and the original code that calls into it is marked
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
