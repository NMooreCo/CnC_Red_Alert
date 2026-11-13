The Windows version of this modern port of Command & Conquer Red Alert, built from the
`@BRANCH@` branch (commit @COMMIT@). It is rebuilt and replaced whenever the branch changes.

**The game data is not included**: EA released Red Alert as freeware in 2008, but its
license lets you use the game, not distribute it. The installer downloads it for you.

### Installing

1. Download `redalert-windows-setup.exe` and run it. It isn't code-signed, so Windows may
   say "Windows protected your PC": choose **More info**, then **Run anyway**.
2. On the "Game data" page, keep **Download EA's freeware release** (1 GB, from the
   [Internet Archive](https://archive.org/details/official-cn-ctiberian-sun_202510)):
   the setup downloads the two discs, checks that they are EA's original files and
   unpacks them. You can also copy the data from the discs, from disc images (`.iso`) that
   Windows has opened as drives, or from folders holding a disc's files.
3. It installs for you only (no administrator rights needed), with Start menu and desktop
   shortcuts, and an uninstaller in Settings > Apps.

`redalert-windows-x86_64.zip` is the program without the installer: unpack it and put the
game data next to `redalert.exe` (`REDALERT.MIX`, which is in the `INSTALL` folder of
either disc, `CD1\MAIN.MIX` from the Allied disc and `CD2\MAIN.MIX` from the Soviet disc).
The discs are `RedAlert1_AlliedDisc.rar` and `RedAlert1_SovietDisc.rar` on the Internet
Archive; unpack them with [7-Zip](https://www.7-zip.org/) (Windows' own "Extract all" may
fail on them), then open the `.ISO` files inside.

**Optional enhancements** (all off until you turn them on): in a mission, open **Options**,
**Game Controls**, **Enhancements**. So far: veterancy (units earn ranks from their kills), a build queue (line up units at
each factory), attack-move (Ctrl+Shift click), modern mouse controls (right-click to give
orders), shared tech (both sides build everything, in skirmish), smarter computer players
(in skirmish: they attack together in waves, raid harvesters, build navies and super
weapons, each with a personality), support powers (artillery barrage, naval bombardment,
Chinook reinforcements) and mission select (start a new campaign at any mission you have
reached). The mouse wheel scrolls the sidebar and lists.

**Gamepads**: a controller works the pointer (left stick), the mouse buttons (A, B), map
scrolling (right stick) and more; see the README.

**Co-op campaign**: in a network game, the host can choose a campaign mission (at the end
of the map list); the players play it together, sharing one side.

**Random maps**: at the end of the skirmish and network game map lists, "Random Map: Small",
"Medium" and "Large" make a new map each time you choose one, from the terrain of the
game's own maps. Other players get it from the host.

**Bigger battle screen**: more of the battlefield in view. Open `REDALERT.INI` (in the
game's folder) and add a `[Video]` section with `Resolution=1280x720` (or any size), or
`Resolution=auto` to fill the screen (it starts fullscreen; Alt+Enter switches). The menus
stay as they were.

**Survival**, in the skirmish and network game options: hold out against 20 waves of enemy
forces. **Co-op**, in the network game options: the players together against the computer
players. In network games, the host's choice of enhancements applies to everyone.

**Internet games**: the host lets UDP port 1234 through their router; the others type
`/connect` and the host's address in the lobby's message line. Everyone needs this version
(earlier ones can't join it).

Games saved by builds before the support powers can't be loaded by this one; with mission
select you can start again at the mission you were playing.

Needs 64-bit Windows. Tested under Wine and played on Windows; reports are welcome. See the [README](https://github.com/@REPO@/blob/@BRANCH@/README.md)
for more, including modding.
