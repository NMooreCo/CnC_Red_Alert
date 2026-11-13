# Red Alert on Android

The game as an Android app: the same code as the desktop versions, built for 64-bit ARM
phones and tablets running Android 11 or later. SDL2 provides the window, sound and input
on Android as it does elsewhere.

**Status:** CI builds the app (the `android-BRANCH` pre-release, and the `redalert-android`
artifact). It hasn't been run on a phone yet: there is no Android device or emulator where
it was made. The touch controls were tested in the web version, on an emulated phone in
Chromium. Reports are welcome.

## Installing

1. On the phone, download `redalert-android.apk` from the Releases page (the `android-...`
   pre-release) and open it. Android asks to allow your browser (or file manager) to
   install apps: allow it, then install. It isn't on Google Play.
2. Newer builds install over older ones (they are all signed with the same key), keeping
   the data, settings and saved games.

## The game data

The data isn't part of the app: EA released Red Alert as freeware, but its license lets you
use the game, not distribute it. You need the same files as on the desktop (see "Game data"
in the main README): `REDALERT.MIX`, with the discs' `MAIN.MIX` in `CD1` (Allied) and `CD2`
(Soviet), or one disc's `MAIN.MIX` next to `REDALERT.MIX`. If you've installed the game on
Windows with this port's installer, its folder is laid out like that already.

1. Copy that folder onto the phone, inside a folder of its own: for instance, connect the
   phone to the computer by USB and copy it to `Download/RedAlert`. (Android's folder picker
   won't offer the top of `Download` or of the phone's storage themselves.)
2. Open the app, tap **Choose the data folder** and choose it. Its data files are copied
   into the app (about 500 MB with one disc, 1 GB with both), once; then the game starts.

You can also copy the files with a computer straight into the app's folder,
`Android/data/io.github.nmooreco.redalert/files`, where the app keeps them. The game's
settings (`REDALERT.INI`) and saved games are there too. Uninstalling the app removes them
all; to choose the data again, clear the app's storage in Android's settings.

## Playing

| On the screen | Does |
| --- | --- |
| tap | a click (select, give an order, press a button) |
| one finger dragged | draws a box around units (the left button held) |
| two fingers dragged | scroll the map |
| two fingers tapped | a right click (deselect; with modern controls, an order) |
| three fingers tapped | the on-screen keyboard, on and off (names, chat) |
| Back | Escape (the options menu in a mission) |

A game controller works as on the desktop (see "Game controllers" in the main README), and
so do a mouse and keyboard. The radar map can be tapped to jump there.

The battle screen uses the whole width of the phone (`[Video] Resolution=auto` in
`REDALERT.INI`); the menus are 640x400, as always. `Resolution=640x400` there gives the
original's bigger buttons and pictures.

LAN games over Wi-Fi should work with other copies of the game (phones or computers): the
app keeps Wi-Fi's broadcast packets coming.

## Building

You need Java 17 or later and the Android SDK, with platform 35, NDK 27.2.12479018 and
CMake 3.22.1 (`sdkmanager "platforms;android-35" "ndk;27.2.12479018" "cmake;3.22.1"`; the
build tools come with Gradle's Android plugin).

```sh
cd android
ANDROID_HOME=/path/to/android-sdk ./gradlew assembleRelease
```

This produces `app/build/outputs/apk/release/app-release.apk`. The build downloads SDL2's
source the first time (2.32.10, checked against its SHA-256), or uses yours with
`-Psdl2Dir=/path/to/SDL2-2.32.10`. It builds the game with the repository's
`CMakeLists.txt`, as `libredalert.so`, next to `libSDL2.so`.

`debug.keystore` signs every build, so that newer ones install over older ones; it isn't a
secret (its password is `android`). Use your own key for anything you publish.

## How it works

- SDL's Java side (`SDLActivity`, from SDL's source) runs the game's library: it calls
  `SDL_main` (`CODE/STARTUP.CPP`). `SetupActivity` comes first and puts the data in place.
- Android 11 is needed for the game's writable string literals (`compat/compat.cpp`): the
  NDK links the read only data separately from the code only from Android 11 (API 30) on,
  so that it can be made writable without touching the code.
- The libraries are aligned for phones with 16 KB memory pages
  (`ANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES`).
- Android's C library lacks `ftime()` (`compat/android/sys/timeb.h`), and its `<stdlib.h>`
  declares `malloc()` through its own `<malloc.h>` (`compat/posix/malloc.h`).
- The game data is found in the app's folder (`Platform_Get_Base_Path`,
  `WIN32LIB/SDL/SDLVIDEO.CPP`), as on the desktop next to the program. The touch controls
  are in the same file, for every platform (the web version on a phone too).
