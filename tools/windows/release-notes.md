The Windows version of this modern port of Command & Conquer Red Alert, built from the
`@BRANCH@` branch (commit @COMMIT@). It is rebuilt and replaced whenever the branch changes.

**The game data is not included.** EA released the game's two discs as freeware in 2008;
the installer copies the data from them, or from their disc images (`.iso`).

### Installing

1. Download `redalert-windows-setup.exe` and run it. It isn't code-signed, so Windows may
   say "Windows protected your PC": choose **More info**, then **Run anyway**.
2. When it asks for the discs, insert them, or double-click each `.iso` file so that
   Windows opens it as a drive, and choose that drive: the Allied disc (CD 1) and the
   Soviet disc (CD 2), one for each campaign. A folder holding a disc's files works too.
3. It installs for you only (no administrator rights needed), with Start menu and desktop
   shortcuts, and an uninstaller in Settings > Apps.

`redalert-windows-x86_64.zip` is the program without the installer: unpack it and put the
game data next to `redalert.exe` (`REDALERT.MIX`, which is in the `INSTALL` folder of
either disc, `CD1\MAIN.MIX` from the Allied disc and `CD2\MAIN.MIX` from the Soviet disc).

Needs 64-bit Windows. This build has been tested under Wine; reports from Windows
machines are welcome. See the [README](https://github.com/@REPO@/blob/@BRANCH@/README.md)
for more, including modding.
