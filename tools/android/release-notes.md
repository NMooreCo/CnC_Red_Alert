The Android version of this modern port of Command & Conquer Red Alert, built from the
`@BRANCH@` branch (commit @COMMIT@): an app for 64-bit ARM phones and tablets with Android 11
or later. It is rebuilt and replaced whenever the branch changes.

**New and not yet tried on a phone**: it was built and checked, but there was no Android
device to run it on where it was made. The touch controls were tested in the web version on
an emulated phone. Please report how it goes.

**The game data is not included**: EA released Red Alert as freeware in 2008, but its
license lets you use the game, not distribute it.

### Installing

1. On the phone, download `redalert-android.apk` below and open it. Android asks to allow
   your browser (or file manager) to install apps: allow it, then install. Newer builds
   install over older ones, keeping your data and saved games.
2. Copy the game data folder onto the phone, inside a folder of its own (for instance, by
   USB, into `Download/RedAlert`): `REDALERT.MIX`, with the discs' `MAIN.MIX` in `CD1`
   (Allied) and `CD2` (Soviet), or one disc's `MAIN.MIX` next to `REDALERT.MIX`. The folder
   of a Windows install from this port's installer is laid out like that already.
3. Open the app, tap **Choose the data folder** and choose it. The files are copied into
   the app, once (about 500 MB per disc); then the game starts.

### Playing

Tap to click; drag a finger to draw a box around units; drag two fingers to scroll the map;
tap with two fingers to right-click, with three for the on-screen keyboard. Back is Escape.
A game controller, a mouse and a keyboard work too. The battle screen uses the phone's
whole width.

See [android/README.md](https://github.com/@REPO@/blob/@BRANCH@/android/README.md) for more.
