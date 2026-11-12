The Windows version of this modern port of Command & Conquer Red Alert, built from the
`@BRANCH@` branch (commit @COMMIT@). It is rebuilt and replaced whenever the branch changes.

**The game data is not included**: EA released Red Alert as freeware in 2008, but its
license lets you use the game, not distribute it. Here is how to get it.

### Getting the game data

1. Download `RedAlert1_AlliedDisc.rar` and `RedAlert1_SovietDisc.rar` (about 500 MB each)
   from the Internet Archive's copy of EA's freeware release:
   https://archive.org/details/official-cn-ctiberian-sun_202510
2. Unpack them with [7-Zip](https://www.7-zip.org/) (Windows' own "Extract all" may fail on
   these archives). They hold the disc images `CD1_ALLIED_DISC.ISO` and
   `CD2_SOVIET_DISC.ISO`.
3. Double-click each `.ISO` file: Windows opens it as a drive (E:, F:...).

### Installing

1. Download `redalert-windows-setup.exe` and run it. It isn't code-signed, so Windows may
   say "Windows protected your PC": choose **More info**, then **Run anyway**.
2. When it asks for the discs, choose the two drives that Windows opened: the Allied disc
   (CD 1) and the Soviet disc (CD 2), one for each campaign. Real discs, or folders holding
   a disc's files, work too.
3. It installs for you only (no administrator rights needed), with Start menu and desktop
   shortcuts, and an uninstaller in Settings > Apps.

`redalert-windows-x86_64.zip` is the program without the installer: unpack it and put the
game data next to `redalert.exe` (`REDALERT.MIX`, which is in the `INSTALL` folder of
either disc, `CD1\MAIN.MIX` from the Allied disc and `CD2\MAIN.MIX` from the Soviet disc).

Needs 64-bit Windows. This build has been tested under Wine; reports from Windows
machines are welcome. See the [README](https://github.com/@REPO@/blob/@BRANCH@/README.md)
for more, including modding.
