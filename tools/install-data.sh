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
# Sets up a game data directory from the Red Alert CDs.
#
# usage: tools/install-data.sh DATA_DIR ALLIED_DISC [SOVIET_DISC]
#
# A disc is either a CD image (.iso, extracted with 7-Zip: "7z") or a directory holding
# the disc's files (a mounted or copied CD). The script copies REDALERT.MIX (from the
# disc's INSTALL directory) to DATA_DIR and each disc's MAIN.MIX to DATA_DIR/CD1 (Allied)
# and DATA_DIR/CD2 (Soviet).

set -e

if [ $# -lt 2 ] || [ $# -gt 3 ]; then
	echo "usage: $0 DATA_DIR ALLIED_DISC [SOVIET_DISC]" >&2
	echo "  a disc is a .iso image or a directory holding the disc's files" >&2
	exit 1
fi

DATA_DIR=$1
shift

# copy_from_disc DISC PATH_ON_DISC DESTINATION_FILE
copy_from_disc() {
	disc=$1
	name=$2
	dest=$3
	if [ -d "$disc" ]; then
		# Find the file whatever the case of its name.
		found=$(find "$disc" -ipath "$disc/$name" -type f 2>/dev/null | head -n 1)
		if [ -z "$found" ]; then
			echo "error: $name not found in $disc" >&2
			exit 1
		fi
		cp "$found" "$dest"
	else
		if ! command -v 7z >/dev/null 2>&1; then
			echo "error: 7z (7-Zip) is needed to read CD images; install it or mount the image" >&2
			exit 1
		fi
		tmp=$(mktemp -d)
		7z e -y -o"$tmp" "$disc" "$name" >/dev/null
		file=$(find "$tmp" -type f | head -n 1)
		if [ -z "$file" ]; then
			rm -rf "$tmp"
			echo "error: $name not found in $disc" >&2
			exit 1
		fi
		mv "$file" "$dest"
		rm -rf "$tmp"
	fi
	echo "  $dest"
}

mkdir -p "$DATA_DIR"
echo "Installing the game data into $DATA_DIR:"

copy_from_disc "$1" "INSTALL/REDALERT.MIX" "$DATA_DIR/REDALERT.MIX"

volume=1
for disc in "$@"; do
	mkdir -p "$DATA_DIR/CD$volume"
	copy_from_disc "$disc" "MAIN.MIX" "$DATA_DIR/CD$volume/MAIN.MIX"
	volume=$((volume + 1))
done

echo "Done. Run the game with: RA_DATA_DIR=\"$DATA_DIR\" ./build/redalert"
