#!/bin/sh
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
#	Builds the Windows installer (NSIS) from a Windows build of the game.
#
#	usage: make-installer.sh [options] BUILD_DIR
#
#	  --data DIR       put the game data in the installer: DIR holds REDALERT.MIX,
#	                   CD1/MAIN.MIX and CD2/MAIN.MIX (as tools/install-data.sh makes it)
#	  --eula FILE      EA's license agreement for the data ("EA Product EULA RedAlert1.rtf"
#	                   in EA's freeware download); required with --data
#	  --7zip DIR       7-Zip's 7z.exe, 7z.dll and License.txt (64-bit, unpacked from 7-Zip's
#	                   installer): the installer can then download the game data itself
#	  --sdl2 FILE      SDL2.dll (default: found through the build's CMake cache)
#	  --out FILE       the installer to write (default: in BUILD_DIR)
#	  --version TEXT   version text (default: date and commit)
#
#	Without --data, the installer gets the data from EA's freeware release: it downloads it
#	(with --7zip), or copies it from the player's discs (or disc images, or folders holding
#	the discs' files).
#
#	An installer with the data in it is for your own use: EA's license for the freeware
#	release allows you to use the data, not to distribute it.
#
set -e

die() { echo "make-installer.sh: $*" >&2; exit 1; }

data= eula= sdl2= out= version= sevenzip=
while [ $# -gt 0 ]; do
	case "$1" in
		--data) data=$2; shift 2 ;;
		--eula) eula=$2; shift 2 ;;
		--sdl2) sdl2=$2; shift 2 ;;
		--7zip) sevenzip=$2; shift 2 ;;
		--out) out=$2; shift 2 ;;
		--version) version=$2; shift 2 ;;
		-h|--help) sed -n '/^#	usage/,/^set -e/p' "$0" | sed '$d; s/^#	\{0,1\}//'; exit 0 ;;
		-*) die "unknown option $1" ;;
		*) break ;;
	esac
done
[ $# -eq 1 ] || die "usage: make-installer.sh [options] BUILD_DIR (see --help)"

command -v makensis >/dev/null || die "makensis (NSIS) is needed: apt install nsis"
source_dir=$(cd "$(dirname "$0")/../.." && pwd)
build_dir=$(cd "$1" && pwd)
[ -f "$build_dir/redalert.exe" ] || die "no redalert.exe in $build_dir (build for Windows first)"

if [ -z "$sdl2" ]; then
	sdl2_cmake=$(sed -n 's/^SDL2_DIR:PATH=//p' "$build_dir/CMakeCache.txt" 2>/dev/null)
	[ -n "$sdl2_cmake" ] && sdl2="$sdl2_cmake/../../../bin/SDL2.dll"
fi
[ -n "$sdl2" ] && [ -f "$sdl2" ] || die "SDL2.dll not found; give it with --sdl2"
sdl2=$(cd "$(dirname "$sdl2")" && pwd)/$(basename "$sdl2")
sdl2_readme=$(dirname "$sdl2")/../../README-SDL.txt

if [ -z "$version" ]; then
	version=$(git -C "$source_dir" log -1 --format=%cd-%h --date=format:%Y.%m.%d 2>/dev/null || echo dev)
fi
source_url=$(git -C "$source_dir" remote get-url origin 2>/dev/null | sed -e 's#://[^/@]*@#://#' -e 's#\.git$##')
[ -n "$source_url" ] || source_url="https://github.com/electronicarts/CnC_Red_Alert"

# The program without its debugging information (the build keeps it).
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cp "$build_dir/redalert.exe" "$work/redalert.exe"
strip=$(command -v x86_64-w64-mingw32-strip || command -v strip || true)
[ -n "$strip" ] && "$strip" "$work/redalert.exe"

set -- -V2 -DBUILD_DIR="$work" -DSDL2_DLL="$sdl2" -DSOURCE_DIR="$source_dir" \
	-DSOURCE_URL="$source_url" -DVERSION="$version"
[ -f "$sdl2_readme" ] && set -- "$@" -DSDL2_README="$(cd "$(dirname "$sdl2_readme")" && pwd)/README-SDL.txt"

if [ -n "$data" ]; then
	data=$(cd "$data" && pwd)
	for f in REDALERT.MIX CD1/MAIN.MIX CD2/MAIN.MIX; do
		[ -f "$data/$f" ] || die "$data/$f is missing"
	done
	[ -n "$eula" ] && [ -f "$eula" ] || die "--data needs --eula: EA's license agreement for the data"
	eula=$(cd "$(dirname "$eula")" && pwd)/$(basename "$eula")
	set -- "$@" -DDATA_DIR="$data" -DEULA="$eula"
	[ -n "$out" ] || out="$build_dir/redalert-$version-setup-with-data.exe"
else
	if [ -n "$sevenzip" ]; then
		# The download option: 7-Zip, and the download plugin (radl.c).
		sevenzip=$(cd "$sevenzip" && pwd)
		for f in 7z.exe 7z.dll License.txt; do
			[ -f "$sevenzip/$f" ] || die "$sevenzip/$f is missing"
		done
		cc=$(command -v x86_64-w64-mingw32-gcc-posix || command -v x86_64-w64-mingw32-gcc) ||
			die "the MinGW-w64 C compiler is needed for the download plugin"
		"$cc" -std=gnu11 -O2 -Wall -shared -s -static-libgcc -o "$work/radl.dll" \
			"$source_dir/tools/windows/radl.c" -lwinhttp -lbcrypt -luser32
		set -- "$@" -DSEVENZIP_DIR="$sevenzip" -DRADL_DIR="$work"
	fi
	[ -n "$out" ] || out="$build_dir/redalert-$version-setup.exe"
fi
case "$out" in /*) ;; *) out=$(pwd)/$out ;; esac

echo "Building $out ..."
[ -n "$data" ] && echo "(With the game data: compressing it takes several minutes.)"
makensis "$@" -DOUT_FILE="$out" "$source_dir/tools/windows/redalert.nsi"
ls -l "$out"
if [ -n "$data" ]; then
	echo
	echo "This installer contains EA's game data. EA's license lets you use the data, not"
	echo "distribute it: keep the installer for your own computers; don't publish it."
fi
