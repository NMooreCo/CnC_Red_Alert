/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
package io.github.nmooreco.redalert;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Bundle;

import org.libsdl.app.SDLActivity;

/*
**	The game: SDL's activity, running the game's library (libredalert.so: SDL calls its
**	SDL_main). SetupActivity starts it once the game data is in place.
*/
public class RedAlertActivity extends SDLActivity {
	private WifiManager.MulticastLock multicast;

	@Override
	protected String[] getLibraries() {
		return new String[] { "SDL2", "redalert" };
	}

	@Override
	protected void onCreate(Bundle state) {
		super.onCreate(state);

		/*
		**	LAN games find each other by broadcast, which Wi-Fi drops unless the app asks
		**	for it.
		*/
		WifiManager wifi = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
		if (wifi != null) {
			multicast = wifi.createMulticastLock("redalert");
			multicast.setReferenceCounted(false);
			multicast.acquire();
		}
	}

	@Override
	protected void onDestroy() {
		if (multicast != null && multicast.isHeld()) {
			multicast.release();
		}
		super.onDestroy();
	}
}
