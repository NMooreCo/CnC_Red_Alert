#!/usr/bin/env python3
#
#	Command & Conquer Red Alert(tm)
#	Copyright 2025 Electronic Arts Inc.
#
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
"""Soak test: a long skirmish where every side, the player's included, is the computer.

usage: skirmish.py OUTDIR [--map N] [--minutes M]

Starts a skirmish with the most computer players the map allows, on hard, hands the
player's house to the computer AI as well, sets the fastest game speed and lets them
fight. Every minute it takes a screenshot and records the number of objects. Run it with a
sanitizer build (RA_SANITIZE) to look for memory errors in long games.

--map N      the Nth map in the skirmish list, from 0 (default 0, "A Path Beyond";
             9 is "Island Wars", with naval fighting)
--minutes M  how long to let it run (default 30)
--money N    every minute, give each side N credits if it has less (default 0: off).
             Without it the computer players run out of money in long games and the
             fighting dies down.
--air        give one computer player an aircraft at the start. The AI only builds a
             radar dome (which airfields and helipads need) once an enemy has aircraft,
             so without this there is no air war between computer players.
"""
import argparse
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ragame import Game, sanitizer_report


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('outdir')
    parser.add_argument('--map', type=int, default=0)
    parser.add_argument('--minutes', type=int, default=30)
    parser.add_argument('--money', type=int, default=0)
    parser.add_argument('--air', action='store_true')
    a = parser.parse_args()

    game = Game(a.outdir)
    results = open(os.path.join(a.outdir, 'results.txt'), 'w')

    def log(*words):
        line = time.strftime('%H:%M:%S ') + ' '.join(str(w) for w in words)
        print(line, flush=True)
        results.write(line + '\n')
        results.flush()

    if game.find_window() is None:
        log('no game window')
        sys.exit(1)
    time.sleep(30)
    game.click(320, 238)                # Multiplayer Game
    time.sleep(3)
    game.click(320, 252)                # Skirmish
    time.sleep(5)
    game.click(200, 73 + 13 * a.map)    # the map
    time.sleep(1)
    game.click(237, 272)                # AI players: the most
    game.click(237, 272)
    game.click(530, 322)                # hard
    game.shot('setup')
    game.click(105, 368)                # OK
    time.sleep(40)
    game.gdb('PlayerPtr->IsHuman = 0', 'PlayerPtr->IsStarted = 1', 'PlayerPtr->IQ = Rule.MaxIQ',
             'Options.GameSpeed = 0')
    if a.air:
        # A MiG for the second house (HOUSE_MULTI2). It stays off the map; its existence is
        # enough for the other houses to see an air threat.
        game.gdb('$h = HouseClass::As_Pointer(HOUSE_MULTI2)', '$t = &AircraftTypeClass::As_Reference(AIRCRAFT_MIG)',
                 '$t->Create_One_Of($h)')

    names = ['frame', 'units', 'infantry', 'buildings', 'aircraft', 'vessels', 'bullets', 'won', 'lost']
    for minute in range(1, a.minutes + 1):
        time.sleep(60)
        if not game.running():
            log('GAME EXITED', game.proc.returncode)
            break
        game.shot('s%03d' % minute)
        values, _ = game.gdb('Frame', 'Units.ActiveCount', 'Infantry.ActiveCount', 'Buildings.ActiveCount',
                             'Aircraft.ActiveCount', 'Vessels.ActiveCount', 'Bullets.ActiveCount',
                             'PlayerWins', 'PlayerLoses')
        log('minute %d:' % minute, ', '.join('%s %s' % (n, v) for n, v in zip(names, values)))
        if a.money:
            # The multiplayer houses are HOUSE_MULTI1 (12) to HOUSE_MULTI8 (19).
            game.gdb(*['(HouseClass::As_Pointer((HousesType)%d) && HouseClass::As_Pointer((HousesType)%d)->Credits < %d) ? '
                       '(HouseClass::As_Pointer((HousesType)%d)->Credits = %d) : 0' % (h, h, a.money, h, a.money)
                       for h in range(12, 20)])

    log('game exit code', game.stop(60))
    asan, ubsan = sanitizer_report(os.path.join(a.outdir, 'game.log'))
    log('sanitizer: %d AddressSanitizer error(s), %d distinct UBSan report(s)' % (asan, len(ubsan)))
    for line, count in ubsan.items():
        log('   %dx %s' % (count, line))
    sys.exit(0 if asan == 0 and not ubsan else 1)


if __name__ == '__main__':
    main()
