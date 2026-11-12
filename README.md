# Command & Conquer Red Alert — modern port

This is the Command & Conquer Red Alert source code released by Electronic Arts,
ported so that it builds with a modern compiler (GCC or Clang, 64-bit) and runs on
current systems through [SDL2](https://www.libsdl.org/). It is meant to be a base for
modding and extending the game.

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
- campaign missions (tested: first Allied mission), skirmish against the computer;
- saving and loading games;
- LAN games ("Network" in the multiplayer menu), over UDP: tested with two players;
- sound effects, speech and music;
- window or fullscreen, scaled up, optionally stretched to 4:3 as on a CRT monitor.

Not available:

- Modem / serial games (the serial library was not released), and Westwood Online
  internet games (the service no longer exists).
- The DVD edition's MPEG movies (the original VQA movies are played instead).

Tested on Linux (x86-64); CI builds it with GCC and Clang. The code is written to be
portable to other SDL2 platforms, but nothing else has been tried yet.

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

Build options (`-D<option>=ON`):

| Option | Effect |
| --- | --- |
| `RA_LANGUAGE_GERMAN`, `RA_LANGUAGE_FRENCH` | Build a German or French version (the data must match). |
| `RA_WARNINGS` | Show compiler warnings (off by default: the 1996 code produces thousands). |
| `RA_SANITIZE` | Build with AddressSanitizer and UndefinedBehaviorSanitizer. |

## Game data

The game data is not part of this repository. EA released Red Alert as freeware in
2008; its two CD images (Allied and Soviet discs) are still around, and the data also
comes with the C&C Ultimate Collection. You need:

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

EA's freeware downloads (`RedAlert1_AlliedDisc.rar`, `RedAlert1_SovietDisc.rar`) are RAR
archives holding these images. Unpack them with `unar` or `unrar`; some 7-Zip builds
can't decompress them.

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
directory and then next to the executable.

Alt+Enter switches between window and fullscreen. Environment variables:

| Variable | Effect |
| --- | --- |
| `RA_DATA_DIR` | Directory holding the game data. |
| `RA_FULLSCREEN=1` | Start fullscreen. |
| `RA_SCALE=n` | Window size as a multiple of 640x400 (default 2). |
| `RA_ASPECT=1` | Stretch 640x400 to 4:3, as a CRT monitor showed it. |
| `RA_NO_SOUND=1` | Disable sound. |
| `RA_LAN_BROADCAST=a.b.c.d` | Broadcast address(es) for LAN games, comma separated (default 255.255.255.255). |
| `RA_DEBUG=1` | Print the game's debug messages to the console. |
| `RA_NO_MESSAGEBOX=1` | Don't show system message boxes (start-up errors and warnings); take their default answer. For automated runs. |
| `RA_SCREENSHOT=file.bmp` | Save one screenshot, `RA_SCREENSHOT_DELAY` milliseconds after start. |

## LAN games

Novell IPX, which the original used for network games, no longer exists, so "Network"
games use UDP broadcasts on port 1234 instead (the original code had this as an unused
option). Players on the same local network see each other's games in the lobby.

## Modding

The quickest changes need no compiling at all: the game reads loose files from the
data directory before looking inside the MIX archives. Extract `RULES.INI` from
`LOCAL.MIX` (inside `REDALERT.MIX`) with any MIX tool, edit it, and drop it next to
`REDALERT.MIX` to change units, weapons, costs, and so on. The same goes for
scenarios (`SCG01EA.INI`...), shapes and other files.

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

## Porting notes

Things to know when you change the code, because the original compiler behaved
differently from today's:

- **Structures are byte packed.** The game was compiled with Watcom's `/zp1`, and file
  formats and data overlays depend on it. `portable.h` applies `#pragma pack(1)` to
  all game code; the stand-in Windows headers use the natural layout.
- **Enums are as small as their values allow** (`-fshort-enums`), as with Watcom.
  For example, `TemplateType` is 2 bytes, which the scenario map data relies on.
- **`long` was replaced by `int`** where the code meant 32 bits. Use `intptr_t` or
  `uintptr_t` to keep a pointer in an integer.
- **`bool` was an `int` typedef** in the original (Watcom 10.6 had no `bool`), and a few
  places kept other values in it (a message box's button number, the shutdown state).
  Those now use `int`; watch for the same when you see `bool` holding more than
  true/false.
- **String literals are writable**, as they were with Watcom; a few places modify
  them. The program's read-only data is made writable at start-up on ELF systems
  (`compat/compat.cpp`).
- **`operator new` may return NULL** for the pooled object classes (units, bullets,
  animations...) when their pool is full; the code checks, and `-fcheck-new` makes the
  compiler do so too.
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
