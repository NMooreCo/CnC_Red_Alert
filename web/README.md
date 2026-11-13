# Red Alert in a web browser

The same game as the desktop versions, compiled to WebAssembly with
[Emscripten](https://emscripten.org). `index.html` is the page that starts it; the build
adds `redalert.js` and `redalert.wasm` next to it.

## Playing

Open the page (from a web server: see Hosting), choose the folder that holds the game data
and press Play. The folder is laid out as for the desktop versions (see "Game data" in the
main README): `REDALERT.MIX`, with the discs' `MAIN.MIX` in `CD1` (Allied) and `CD2`
(Soviet), or one disc's `MAIN.MIX` next to `REDALERT.MIX`. An installed copy's folder will
do: its settings (`REDALERT.INI`) and saved games come along.

The data is copied into the browser, so it is chosen only once: the `.MIX` files go into the
browser's cache storage (they are too big for IndexedDB), the settings, saved games and
random maps into IndexedDB, saved again every 20 seconds and when the tab is hidden.
"Forget the data" removes it all. Cache storage exists only on pages served over https or
from localhost; elsewhere the data has to be chosen each time.

Everything plays as on the desktop, with the mouse, the keyboard and the sound, except
network games: a web page can't send UDP packets, so the multiplayer menu has no "Network"
(Skirmish is there). The game fills the browser's window; for full screen, use the
browser's own (F11), or Alt+Enter, which asks the browser for it.

The data is held in memory while the game runs: about 500 MB with one disc, 1 GB with both.
A desktop browser (Chrome, Edge, Firefox or Safari) is needed.

## Building

With Emscripten's `emcmake`:

```sh
emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
```

The page is then `build-web/index.html`, with `redalert.js` and `redalert.wasm`. Emscripten
builds its own copy of SDL2 (its `-sUSE_SDL=2` port) the first time, downloading SDL's
source from GitHub.

CI builds it with Ubuntu 24.04's Emscripten (3.1.6; the `redalert-web` artifact). That
package's cache of prebuilt libraries is read-only ("frozen"), so it can't build the SDL2
port into it: point Emscripten at a writable copy first.

```sh
sudo apt-get install emscripten ninja-build
cp -R --preserve=mode,timestamps /usr/share/emscripten/cache ~/em-cache
{ sed 's/^FROZEN_CACHE = True.*/FROZEN_CACHE = False/' /usr/share/emscripten/.emscripten
  echo; echo "CACHE = '$HOME/em-cache'"; } > ~/em-config
export EM_CONFIG=~/em-config
```

Without access to GitHub, give it SDL's source yourself (the version the port expects,
2.0.20 for Emscripten 3.1.6): `EMCC_LOCAL_PORTS=sdl2=/path/to/SDL2-2.0.20`.

## Hosting

Any static web server will do, as long as the page is served over http(s) (not opened
as a file) and `.wasm` files as `application/wasm` (most servers do). To try it locally:

```sh
cd build-web && python3 -m http.server 8000
```

and open <http://localhost:8000/>. Never put the game data on a public server: EA's
license lets you use the game, not distribute it.

For testing, the page also takes:

- `?data=URL`: fetch the data from a web server (`URL/REDALERT.MIX`, `URL/CD1/MAIN.MIX`...)
  instead of asking for it, without keeping it;
- `?env=NAME=VALUE` (several times if needed): the game's environment variables, as in
  the main README's Running section (`?env=RA_RESOLUTION=1280x720`, `?env=RA_DEBUG=1`,
  whose messages go to the browser's console).

`Module._RA_Web_Game_Frame()`, in the browser's console, gives the game's logic frame: it
advances 20 times a second at the default game speed.

## How it works

The game is the desktop version's code; the browser needs a few things done differently
(each marked `__EMSCRIPTEN__` in the code):

- The game never returns to the browser: its loops (menus, dialogs, the game itself,
  movies) run until they're done, and a web page has to let the browser run to draw the
  page, take input and play sound. With Asyncify (`-sASYNCIFY`), `emscripten_sleep`
  unwinds the game's stack, lets the browser run and comes back. The game yields there
  about every 10 milliseconds, from `Win32_Web_Poll` (`compat/win32.cpp`), which is called
  wherever it waits or reads the time: the event pump, `Sleep`, the timers' tick counts,
  the sound's play position (the movies keep time by it).
- There are no threads for the timers (the 60 Hz system timer, the sound and mouse
  upkeep): `Win32_Web_Poll` calls them, once for each period that has passed.
- The sound mixer runs only while the browser does, so its buffer is bigger (4096 samples).
- Winsock doesn't start (`compat/winsock.cpp`): the game then offers no network games.
- `main`: the compiler renames a `main(argc, argv)`, and Emscripten 3.1.6 starts only a
  `main()` itself, so one starts the game's (`compat/compat.cpp`).
