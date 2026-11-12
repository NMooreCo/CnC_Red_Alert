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
"""Start the game and run a list of steps against it.

usage: drive.py OUTDIR STEP [STEP...]

Steps (coordinates are game coordinates, 640x400):
    wait:SECONDS          sleep
    shot:NAME             screenshot of the game window to OUTDIR/NAME.png
    move:X,Y              move the mouse
    click:X,Y[,BUTTON]    click (button 1 = left, 3 = right)
    drag:X1,Y1,X2,Y2      drag with the left button (to select units)
    key:KEYSYM            press a key (xdotool names: Return, Escape, F1, ctrl+s...)
    type:TEXT             type text
    gdb:EXPR[;EXPR...]    print expressions with gdb, e.g. gdb:PlayerWins=1 or gdb:Frame
    gdbx:FILE             attach gdb in the background with a command file (OUTDIR/gdbx.txt)

Example -- start the first Allied mission, then win it:
    drive.py out wait:30 click:320,182 wait:4 click:499,252 wait:4 click:259,227 \\
        wait:8 key:Escape wait:6 key:Escape wait:15 shot:mission gdb:PlayerWins=1 wait:10 shot:won
"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ragame import Game, sanitizer_report


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    game = Game(sys.argv[1])
    for step in sys.argv[2:]:
        if not game.running():
            print('game exited (%s) before step %s' % (game.proc.returncode, step))
            break
        kind, _, arg = step.partition(':')
        if kind == 'wait':
            time.sleep(float(arg))
        elif kind == 'shot':
            print('shot', game.shot(arg))
        elif kind == 'move':
            x, y = (int(v) for v in arg.split(','))
            game.move(x, y)
        elif kind == 'click':
            parts = [int(v) for v in arg.split(',')]
            game.click(parts[0], parts[1], parts[2] if len(parts) > 2 else 1)
        elif kind == 'drag':
            game.drag(*(int(v) for v in arg.split(',')))
        elif kind == 'key':
            game.key(arg)
        elif kind == 'type':
            game.type(arg)
        elif kind == 'gdb':
            values, _ = game.gdb(*arg.split(';'))
            print('gdb', arg, '->', ' | '.join(values))
        elif kind == 'gdbx':
            game.gdb_script(arg)
        else:
            sys.exit('unknown step: ' + step)
        sys.stdout.flush()
    print('game exit code', game.stop())
    asan, ubsan = sanitizer_report(os.path.join(game.out, 'game.log'))
    if asan or ubsan:
        print('sanitizer: %d AddressSanitizer error(s), %d distinct UBSan report(s)' % (asan, len(ubsan)))


if __name__ == '__main__':
    main()
