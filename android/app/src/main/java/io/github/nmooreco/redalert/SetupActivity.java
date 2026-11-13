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

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Color;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/*
**	The app's start. The game needs its data, which can't come with the app (EA's freeware
**	release is for the player's own use), in the app's own folder (Android/data/<app>/files).
**	If it isn't there yet, the player chooses the folder that holds it on the phone, and its
**	data files are copied in, once. Then the game starts (RedAlertActivity).
*/
public class SetupActivity extends Activity {
	private static final int CHOOSE_FOLDER = 1;

	private TextView status;
	private ProgressBar progress;
	private Button choose;

	/*
	**	A file to copy: where it is (a document of the chosen folder) and where it goes.
	*/
	private static class Item {
		String document;
		String path;			// In the app's folder: "REDALERT.MIX", "CD1/MAIN.MIX"...
		long size;

		Item(String document, String path, long size) {
			this.document = document;
			this.path = path;
			this.size = size;
		}
	}

	@Override
	protected void onCreate(Bundle state) {
		super.onCreate(state);
		if (hasData()) {
			startGame();
			return;
		}
		showSetup();
	}

	private File dataFolder() {
		return getExternalFilesDir(null);
	}

	/*
	**	The data is there: REDALERT.MIX, and a disc's MAIN.MIX (next to it, or in a disc's
	**	folder: CD1 Allied, CD2 Soviet, CD3 and CD4 the expansions).
	*/
	private boolean hasData() {
		File folder = dataFolder();
		if (folder == null || !new File(folder, "REDALERT.MIX").isFile()) return false;
		String[] discs = { "MAIN.MIX", "CD1/MAIN.MIX", "CD2/MAIN.MIX", "CD3/MAIN.MIX", "CD4/MAIN.MIX" };
		for (String disc : discs) {
			if (new File(folder, disc).isFile()) return true;
		}
		return false;
	}

	private void startGame() {
		startActivity(new Intent(this, RedAlertActivity.class));
		finish();
	}

	private TextView text(String text, float size, int color) {
		TextView view = new TextView(this);
		view.setText(text);
		view.setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
		view.setTextColor(color);
		view.setPadding(0, 0, 0, pixels(12));
		return view;
	}

	private int pixels(float dp) {
		return (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, getResources().getDisplayMetrics());
	}

	private void showSetup() {
		LinearLayout layout = new LinearLayout(this);
		layout.setOrientation(LinearLayout.VERTICAL);
		layout.setPadding(pixels(28), pixels(20), pixels(28), pixels(20));

		TextView title = text("Command & Conquer Red Alert", 24, Color.rgb(230, 50, 40));
		title.setTypeface(Typeface.DEFAULT_BOLD);
		layout.addView(title);
		layout.addView(text(
			"The game needs its data files, which can't come with this app: EA released Red Alert " +
			"as freeware, but only for you to use. Copy the folder that holds them onto this phone " +
			"(into Download/RedAlert, for instance): REDALERT.MIX, with the discs' MAIN.MIX in CD1 " +
			"(Allied) and CD2 (Soviet), or one disc's MAIN.MIX next to REDALERT.MIX. Then choose " +
			"that folder here. Its files are copied into the app, once.", 16, Color.rgb(220, 215, 205)));

		choose = new Button(this);
		choose.setText("Choose the data folder");
		choose.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {
				startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE), CHOOSE_FOLDER);
			}
		});
		layout.addView(choose);

		progress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
		progress.setMax(1000);
		progress.setVisibility(View.GONE);
		layout.addView(progress);

		status = text("", 16, Color.rgb(255, 200, 90));
		status.setPadding(0, pixels(12), 0, pixels(12));
		layout.addView(status);

		layout.addView(text(
			"Playing: tap to click; drag a finger to draw a box around units; drag two fingers to " +
			"scroll the map; tap with two fingers to right-click, with three for the keyboard. " +
			"Back is Escape. A game controller works too.", 14, Color.rgb(150, 145, 135)));

		ScrollView scroll = new ScrollView(this);
		scroll.setBackgroundColor(Color.BLACK);
		scroll.addView(layout);
		setContentView(scroll);
	}

	@Override
	protected void onActivityResult(int request, int result, Intent data) {
		super.onActivityResult(request, result, data);
		if (request != CHOOSE_FOLDER || result != RESULT_OK || data == null || data.getData() == null) return;
		final Uri tree = data.getData();
		choose.setEnabled(false);
		status.setText("Looking at the folder...");
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		new Thread(new Runnable() {
			@Override
			public void run() {
				copyFolder(tree);
			}
		}).start();
	}

	/*
	**	A file of the data: the game's own (.MIX), and from an installed copy its settings,
	**	maps and saved games. Upper case, as the game asks for them.
	*/
	private static boolean wanted(String name) {
		String upper = name.toUpperCase(Locale.ROOT);
		return upper.endsWith(".MIX") || upper.endsWith(".INI") || upper.endsWith(".MPR") || upper.matches("SAVEGAME\\.[0-9][0-9][0-9]");
	}

	private void findFiles(Uri tree, String document, String prefix, List<Item> items) {
		Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, document);
		String[] columns = {
			DocumentsContract.Document.COLUMN_DOCUMENT_ID,
			DocumentsContract.Document.COLUMN_DISPLAY_NAME,
			DocumentsContract.Document.COLUMN_MIME_TYPE,
			DocumentsContract.Document.COLUMN_SIZE
		};
		Cursor cursor = getContentResolver().query(children, columns, null, null, null);
		if (cursor == null) return;
		try {
			while (cursor.moveToNext()) {
				String id = cursor.getString(0);
				String name = cursor.getString(1);
				String type = cursor.getString(2);
				long size = cursor.isNull(3) ? 0 : cursor.getLong(3);
				if (name == null) continue;
				if (DocumentsContract.Document.MIME_TYPE_DIR.equals(type)) {
					/*
					**	The discs' folders (CD1 ... CD4), next to REDALERT.MIX.
					*/
					if (prefix.isEmpty() && name.toUpperCase(Locale.ROOT).matches("CD[1-4]")) {
						findFiles(tree, id, name.toUpperCase(Locale.ROOT) + "/", items);
					}
				} else if (wanted(name)) {
					items.add(new Item(id, prefix + name.toUpperCase(Locale.ROOT), size));
				}
			}
		} finally {
			cursor.close();
		}
	}

	private void show(final String text, final int done) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				status.setText(text);
				if (done >= 0) {
					progress.setVisibility(View.VISIBLE);
					progress.setProgress(done);
				}
			}
		});
	}

	private void failed(final String text) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				status.setText(text);
				progress.setVisibility(View.GONE);
				choose.setEnabled(true);
				getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
			}
		});
	}

	private void copyFolder(Uri tree) {
		try {
			List<Item> items = new ArrayList<>();
			findFiles(tree, DocumentsContract.getTreeDocumentId(tree), "", items);

			boolean found = false;
			long total = 0;
			for (Item item : items) {
				if (item.path.equals("REDALERT.MIX")) found = true;
				total += item.size;
			}
			if (!found) {
				failed("REDALERT.MIX isn't in that folder. Choose the folder that holds it.");
				return;
			}

			/*
			**	Each file goes in under another name first, so that one cut short (the app
			**	closed while copying) isn't taken for the real thing.
			*/
			File folder = dataFolder();
			long copied = 0;
			byte[] buffer = new byte[1 << 20];
			for (Item item : items) {
				File target = new File(folder, item.path);
				File partial = new File(folder, item.path + ".part");
				if (target.getParentFile() != null) target.getParentFile().mkdirs();
				Uri source = DocumentsContract.buildDocumentUriUsingTree(tree, item.document);
				try (InputStream input = getContentResolver().openInputStream(source);
						OutputStream output = new FileOutputStream(partial)) {
					if (input == null) throw new java.io.IOException("can't read " + item.path);
					int count;
					long shown = 0;
					while ((count = input.read(buffer)) > 0) {
						output.write(buffer, 0, count);
						copied += count;
						if (copied - shown > (4 << 20)) {
							shown = copied;
							show(String.format(Locale.ROOT, "Copying %s (%d of %d MB)...", item.path, copied >> 20, total >> 20),
								total > 0 ? (int)(copied * 1000 / total) : 0);
						}
					}
				}
				if (target.exists()) target.delete();
				if (!partial.renameTo(target)) throw new java.io.IOException("can't write " + item.path);
			}

			if (!hasData()) {
				failed("There's no MAIN.MIX in that folder (or in its CD1 or CD2). Choose the folder that holds the data.");
				return;
			}
			runOnUiThread(new Runnable() {
				@Override
				public void run() {
					startGame();
				}
			});
		} catch (Exception error) {
			failed("Copying the data failed: " + error.getMessage());
		}
	}
}
