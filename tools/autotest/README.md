# Automated tests

These scripts run the game on a virtual X display and play it without anyone at the
keyboard. They are how the port was tested, and they are useful after a change to the
code or to the rules and missions:

| Script | What it does |
| --- | --- |
| `campaign.py` | Plays through a campaign: each mission runs for a few seconds and is then won, through the movie, score screen, map selection and briefing up to the next mission, and after the last one the ending and credits. |
| `skirmish.py` | Soak test: a long skirmish with the most computer players the map allows, the player's house handed to the computer too, at the fastest speed. |
| `drive.py` | Runs a list of steps you give it (clicks, keys, waits, screenshots, game variables). |

Each run writes screenshots, `results.txt` and the game's output (`game.log`) into the
directory you name.

## Requirements

Linux with `Xvfb`, `xdotool`, ImageMagick (`import`), `gdb` and Python 3. On Debian or
Ubuntu:

```sh
sudo apt install xvfb xdotool imagemagick gdb python3
```

The scripts read and set game variables (for example `PlayerWins`) by attaching `gdb`
to the game for a moment. They start the game with `RA_ALLOW_PTRACE=1`, which lets
`gdb` do that without root.

## Running

```sh
export RA_DATA_DIR=~/redalert-data       # the game data (see the main README)
export RA_BIN=build/redalert             # the game to test (this is the default)

tools/autotest/campaign.py out/allies allies
tools/autotest/campaign.py out/soviet soviet --missions 3
tools/autotest/skirmish.py out/soak --map 4 --minutes 30 --money 50000 --air
```

The game runs on display `:99` unless `RA_DISPLAY` names another; an `Xvfb` is started
there if there is none. To run tests side by side, give each its own display and its own
copy of the data directory (the game writes its settings and saves there).

For the most value, test a build made with `-DRA_SANITIZE=ON` and set
`ASAN_OPTIONS=halt_on_error=0` so that a run reports every memory error. The scripts
count the AddressSanitizer errors and UBSan reports in `game.log` at the end and exit
with status 1 if there are any. They leave out two kinds of UBSan report that come from
the original code's byte-packed data structures (misaligned accesses) and its object
pools.

`campaign.py --start N` begins at mission N (it plays mission 1, then makes the game
believe it was mission N-1). Soviet mission 7 opens with a scripted trap: a soldier
shoots the explosive barrels next to your men, and a player has to move them away.
The script does not, so it loses that mission once and replays it.
