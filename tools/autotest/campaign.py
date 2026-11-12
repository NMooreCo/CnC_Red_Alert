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
"""Play through a campaign, winning each mission after a few seconds.

usage: campaign.py OUTDIR allies|soviet [--missions N] [--seconds S] [--start N]

Starts a new campaign, lets each mission run for S seconds (default 10), takes a
screenshot, makes the player win (gdb: PlayerWins = 1) and then works through everything
up to the next mission: movie, score screen, map selection, briefing. Which screen the
game shows is told from the main thread's call stack.

--missions N   stop after N missions (default: the whole campaign, and its ending)
--start N      begin at mission N: mission 1 is played, then the game is made to believe
               it was mission N-1, so winning it leads to mission N

Results go to OUTDIR/results.txt, with screenshots of each mission (mNN_*.png) and of the
screens between missions (wNN_*.png). Missions that are lost are replayed and won (Soviet
mission 7, for one, opens with a scripted trap that a player escapes by moving).
"""
import argparse
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ragame import Game, sanitizer_report

# Map selection spots (CODE/MAPSEL.CPP MapCoords, first choice), 320x200 coordinates.
SPOTS = [
    [(185, 123), (173, 112), (196, 100), (175, 113), (187, 91), (207, 161), (172, 92), (132, 119),
     (199, 73), (236, 114), (219, 64), (256, 69), (262, 77), (249, 97)],
    [(178, 105), (163, 101), (160, 89), (142, 101), (212, 163), (155, 133), (216, 103), (132, 145),
     (122, 117), (117, 130), (99, 107), (134, 125), (32, 156), (108, 97)],
]
CAMPAIGN_LENGTH = {'allies': 15, 'soviet': 14}      # Allied mission 10 has two parts.


class Campaign:
    def __init__(self, game, side, results):
        self.game = game
        self.side = side
        self.house = 1 if side == 'soviet' else 0
        self.results = results
        self.mission = 0

    def log(self, *words):
        line = time.strftime('%H:%M:%S ') + ' '.join(str(w) for w in words)
        print(line, flush=True)
        self.results.write(line + '\n')
        self.results.flush()

    def state(self):
        """(frame, scenario name, scenario number, screen) or None."""
        values, frames = self.game.gdb('Frame', 'Scen.ScenarioName', 'Scen.Scenario', backtrace=True)
        if len(values) < 3:
            return None
        frame = int(values[0]) if values[0].lstrip('-').isdigit() else -1
        name = re.match(r'"([^"]*)"', values[1])
        return frame, (name.group(1) if name else values[1]), int(values[2]), self.screen(frames)

    @staticmethod
    def screen(frames):
        f = set(frames)
        if 'VQA_Play' in f or 'Play_Movie' in f:
            return 'movie'
        if 'Map_Selection' in f:
            return 'mapsel'
        if 'ScoreClass::Presentation' in f:
            return 'score'
        if any(x.startswith('WWMessageBox') or x.endswith('Dialog') or x in ('Restate_Mission', 'BGMessageBox') for x in f):
            return 'dialog'
        if 'Start_Scenario' in f or 'Read_Scenario' in f:
            return 'loading'
        if 'Show_Who_Was_Responsible' in f:
            return 'credits'
        if 'Do_Win' in f or 'Do_Lose' in f or 'Do_Restart' in f:
            return 'ending'
        if 'Main_Loop' in f:
            return 'game'
        if 'Main_Menu' in f or 'Select_Game' in f:
            return 'menu'
        return 'other'

    def win(self):
        # A mission timer that was never started would be inherited as expired by the next
        # mission (Allied 10A -> 10B). In real play it runs by the time you can win.
        self.game.gdb('Scen.MissionTimer.DelayTime = Scen.MissionTimer.DelayTime ? Scen.MissionTimer.DelayTime : 18000',
                      'Scen.MissionTimer.Started = (Scen.MissionTimer.Started == 4294967295U) ? Frame : Scen.MissionTimer.Started',
                      'PlayerWins = 1')

    def advance(self, previous, timeout=420):
        """Work through the screens after a win until a new mission runs (returns its name)
        or the main menu comes back (returns 'menu')."""
        start = time.time()
        last = None
        polls = 0
        where = '?'
        while time.time() - start < timeout:
            if not self.game.running():
                self.log('GAME EXITED', self.game.proc.returncode)
                return None
            s = self.state()
            if s is None:
                time.sleep(2)
                continue
            frame, name, number, where = s
            polls += 1
            if polls % 3 == 1:
                self.game.shot('w%02d_%03d_%s' % (self.mission, polls, where))
            if where == 'game' and name != previous:
                if last is not None and last[1] == name and frame > last[0]:
                    return name
                last = (frame, name)
                time.sleep(2.5)
                continue
            last = None
            if where == 'menu' and previous is not None:
                return 'menu'
            if where == 'movie':
                self.game.key('Escape')
            elif where == 'score':
                self.game.key('t')
                self.game.key('Return')
                self.game.click(320, 380)
            elif where == 'mapsel':
                x, y = SPOTS[self.house][min(number, 13)]
                self.game.click(x * 2 + 12, y * 2 + 10)
            elif where == 'dialog':
                self.game.key('Return')
            elif where == 'game' and name == previous:
                self.win()      # Not processed yet, or the mission was lost and replayed.
            time.sleep(2)
        self.log('TIMEOUT after', previous, 'on screen', where)
        return None

    def run(self, missions, seconds, start_at):
        if self.game.find_window() is None:
            self.log('no game window')
            return False
        time.sleep(30)
        self.game.click(320, 182)                                   # Start New Game
        time.sleep(4)
        self.game.click(499, 252)                                   # difficulty: OK
        time.sleep(4)
        self.game.click(319 if self.side == 'soviet' else 259, 227)  # side
        time.sleep(3)
        previous = None
        while self.mission < missions:
            self.mission += 1
            t0 = time.time()
            name = self.advance(previous)
            if name is None:
                return False
            if name == 'menu':
                self.log('campaign finished: back at the main menu')
                return True
            self.log('mission %d: %s running after %.0f s' % (self.mission, name, time.time() - t0))
            time.sleep(seconds)
            self.game.shot('m%02d_%s' % (self.mission, name.split('.')[0]))
            if self.mission == 1 and start_at > 1:
                letter = 'U' if self.side == 'soviet' else 'G'
                self.game.gdb('Scen.Scenario = %d' % (start_at - 1),
                              '(void)strcpy(Scen.ScenarioName, "SC%s%02dEA.INI")' % (letter, start_at - 1))
                self.mission = start_at - 1
            self.win()
            previous = name
        if missions >= CAMPAIGN_LENGTH[self.side]:
            # After the last mission: the ending, the credits and the main menu.
            if self.advance(previous, timeout=900) == 'menu':
                self.log('campaign finished: back at the main menu')
                return True
            return False
        return True


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('outdir')
    parser.add_argument('side', choices=['allies', 'soviet'])
    parser.add_argument('--missions', type=int, default=0)
    parser.add_argument('--seconds', type=float, default=10)
    parser.add_argument('--start', type=int, default=1)
    a = parser.parse_args()
    missions = a.missions or CAMPAIGN_LENGTH[a.side]

    game = Game(a.outdir)
    results = open(os.path.join(a.outdir, 'results.txt'), 'w')
    campaign = Campaign(game, a.side, results)
    ok = campaign.run(missions, a.seconds, a.start)
    campaign.log('game exit code', game.stop(30))
    asan, ubsan = sanitizer_report(os.path.join(a.outdir, 'game.log'))
    campaign.log('sanitizer: %d AddressSanitizer error(s), %d distinct UBSan report(s)' % (asan, len(ubsan)))
    for line, count in ubsan.items():
        campaign.log('   %dx %s' % (count, line))
    sys.exit(0 if ok and asan == 0 and not ubsan else 1)


if __name__ == '__main__':
    main()
