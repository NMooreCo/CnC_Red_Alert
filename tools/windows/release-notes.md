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
each factory), shared tech (both sides build everything, in skirmish), support
powers (artillery barrage, naval bombardment, Chinook reinforcements) and mission select (start a new campaign at any mission you have
reached). The mouse wheel scrolls the sidebar and lists.

Games saved by builds before the support powers can't be loaded by this one; with mission
select you can start again at the mission you were playing.

Needs 64-bit Windows. Tested under Wine and played on Windows; reports are welcome. See the [README](https://github.com/@REPO@/blob/@BRANCH@/README.md)
for more, including modding.
