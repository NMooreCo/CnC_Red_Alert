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
"""Run the game on a virtual X display and drive it, for automated tests.

The game runs on its own Xvfb display. Input goes in through xdotool, screenshots come
out through ImageMagick's "import", and gdb attaches for a moment to read or set game
variables (gdb -p ... -batch -ex "print EXPR"). See tools/autotest/README.md.

Environment:
    RA_BIN        the game binary (default: build/redalert next to this repository)
    RA_DATA_DIR   the game data directory (required)
    RA_DISPLAY    X display to use (default :99); an Xvfb is started on it if needed
"""
import atexit
import os
import re
import signal
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_BIN = os.path.join(HERE, '..', '..', 'build', 'redalert')


class Game:
    def __init__(self, outdir, args=()):
        self.out = outdir
        os.makedirs(outdir, exist_ok=True)
        self.display = os.environ.get('RA_DISPLAY', ':99')
        self.env = dict(os.environ)
        self.env['DISPLAY'] = self.display
        self.env.setdefault('SDL_AUDIODRIVER', 'dummy')
        self.env.setdefault('RA_SCALE', '1')
        self.env.setdefault('RA_NO_MESSAGEBOX', '1')
        self.env['RA_ALLOW_PTRACE'] = '1'       # Let gdb attach (Linux Yama ptrace_scope).
        if not self.env.get('RA_DATA_DIR'):
            sys.exit('RA_DATA_DIR must name the game data directory')
        binary = os.path.abspath(self.env.get('RA_BIN', DEFAULT_BIN))
        self._start_x()
        self.log = open(os.path.join(outdir, 'game.log'), 'w')
        self.proc = subprocess.Popen([binary] + list(args), env=self.env, stdout=self.log,
                                     stderr=subprocess.STDOUT, cwd='/')
        self.window = None
        # The size of the screen the game is showing: 640x400, except in battle with a bigger
        # battle screen (RA_RESOLUTION); the window shows it as big as it fits, centred.
        self.logical = (640, 400)
        atexit.register(self.kill)
        signal.signal(signal.SIGTERM, lambda *a: sys.exit(1))

    # -- processes ---------------------------------------------------------------------
    def _run(self, *cmd, timeout=120):
        return subprocess.run(list(cmd), env=self.env, capture_output=True, text=True, timeout=timeout)

    def _start_x(self):
        if subprocess.run(['pgrep', '-f', 'Xvfb ' + self.display + '( |$)'],
                          capture_output=True).returncode != 0:
            subprocess.Popen(['Xvfb', self.display, '-screen', '0', '1280x1024x24', '-nolisten', 'tcp'],
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            time.sleep(2)

    def running(self):
        return self.proc.poll() is None

    def stop(self, wait=10):
        """Ask the game to quit (as when its window is closed); kill it if it won't."""
        if self.running():
            self.proc.send_signal(signal.SIGTERM)
            try:
                self.proc.wait(wait)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()
        return self.proc.returncode

    def kill(self):
        if self.running():
            self.proc.kill()

    # -- window and input ----------------------------------------------------------------
    def find_window(self, timeout=60):
        """The game's window, found by process id (other games may share the display)."""
        end = time.time() + timeout
        while time.time() < end and self.running():
            ids = self._run('xdotool', 'search', '--pid', str(self.proc.pid), '--name', 'Red Alert').stdout.split()
            if ids:
                self.window = ids[-1]
                return self.window
            time.sleep(0.2)
        return None

    def _geometry(self):
        for attempt in range(3):
            if self.window is None and self.find_window() is None:
                raise RuntimeError('no game window')
            out = self._run('xdotool', 'getwindowgeometry', '--shell', self.window).stdout
            g = dict(line.split('=', 1) for line in out.split() if '=' in line)
            if 'X' in g:
                return int(g['X']), int(g['Y']), int(g['WIDTH']), int(g['HEIGHT'])
            # The window is gone. While the game starts up, it can replace its first window
            # (when it sets the video mode), so look for the game's window again.
            self.window = None
            if not self.running():
                break
        # Say why (a negative exit code is the signal that ended the game).
        raise RuntimeError('the game window is gone (game exit code %s)' % self.proc.poll())

    def _to_screen(self, x, y):
        """Game coordinates (in self.logical, normally 640x400) to screen coordinates."""
        wx, wy, ww, wh = self._geometry()
        lw, lh = self.logical
        scale = min(ww / lw, wh / lh)
        ox = (ww - lw * scale) / 2
        oy = (wh - lh * scale) / 2
        return wx + int(ox + x * scale), wy + int(oy + y * scale)

    def move(self, x, y):
        sx, sy = self._to_screen(x, y)
        self._run('xdotool', 'mousemove', str(sx), str(sy))
        time.sleep(0.15)

    def click(self, x, y, button=1):
        self.move(x, y)
        self._run('xdotool', 'click', str(button))
        time.sleep(0.2)

    def drag(self, x1, y1, x2, y2):
        a = self._to_screen(x1, y1)
        b = self._to_screen(x2, y2)
        self._run('xdotool', 'mousemove', str(a[0]), str(a[1]))
        time.sleep(0.15)
        self._run('xdotool', 'mousedown', '1')
        for k in range(1, 9):
            self._run('xdotool', 'mousemove', str(a[0] + (b[0] - a[0]) * k // 8), str(a[1] + (b[1] - a[1]) * k // 8))
            time.sleep(0.05)
        self._run('xdotool', 'mouseup', '1')
        time.sleep(0.2)

    def key(self, keysym):
        if self.window is None:
            self.find_window()
        self._run('xdotool', 'key', '--window', self.window, keysym)
        time.sleep(0.2)

    def type(self, text):
        self._run('xdotool', 'type', '--delay', '80', text)
        time.sleep(0.2)

    def shot(self, name):
        """Screenshot of the game window to OUTDIR/NAME.png."""
        wx, wy, ww, wh = self._geometry()
        path = os.path.join(self.out, name + '.png')
        self._run('import', '-window', 'root', '-crop', '%dx%d+%d+%d' % (ww, wh, wx, wy), '+repage', path)
        return path

    # -- gdb --------------------------------------------------------------------------------
    def gdb(self, *exprs, backtrace=False):
        """Attach, print each expression (assignments work: "PlayerWins = 1"), detach.

        Returns (values, frames): the printed values as strings, and the function names on
        the main thread's stack if backtrace is set."""
        args = ['gdb', '-p', str(self.proc.pid), '-batch', '-ex', 'set pagination off']
        for e in exprs:
            args += ['-ex', 'print ' + e]
        if backtrace:
            args += ['-ex', 'thread 1', '-ex', 'bt 60']
        r = self._run(*args)
        values = re.findall(r'^\$\d+ = (.*)$', r.stdout, re.M)
        frames = re.findall(r'^#\d+\s+(?:0x[0-9a-f]+ in )?([A-Za-z_][\w:~<>]*)', r.stdout, re.M)
        return values, frames

    def gdb_script(self, path):
        """Attach gdb in the background running a command file (breakpoints with commands
        that "continue"); its output goes to OUTDIR/gdbx.txt. While it is attached, gdb()
        cannot attach."""
        out = open(os.path.join(self.out, 'gdbx.txt'), 'a')
        subprocess.Popen(['gdb', '-p', str(self.proc.pid), '-batch', '-x', path], stdout=out,
                         stderr=subprocess.STDOUT, env=self.env)
        time.sleep(3)


def sanitizer_report(logfile):
    """Count AddressSanitizer errors and list the distinct UBSan reports in a game log,
    leaving out the ones that come from the original code's packed data layout."""
    text = open(logfile, errors='replace').read()
    asan = len(re.findall(r'ERROR: AddressSanitizer', text))
    ubsan = {}
    for line in re.findall(r'^.*runtime error.*$', text, re.M):
        if 'misaligned' in line or 'does not point to an object' in line:
            continue
        line = re.sub(r'0x[0-9a-f]+', 'ADDR', line)
        line = re.sub(r'^.*?(CODE|WIN32LIB|WINVQ|compat)/', r'\1/', line)
        ubsan[line] = ubsan.get(line, 0) + 1
    return asan, ubsan
