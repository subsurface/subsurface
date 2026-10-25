// SPDX-License-Identifier: GPL-2.0

// Java classes for handling Intents on Android (which in this case means
// Subsurface-mobile receiving notifaction that an FTDI dive computer was
// plugged in

// Created while looking at https://github.com/ekke/ekkesSHAREexample

package org.subsurfacedivelog.mobile;

import org.qtproject.qt.android.bindings.QtActivity;
import android.os.*;
import android.content.*;
import android.app.*;

import java.lang.String;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.util.Log;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import android.net.Uri;
import android.provider.DocumentsContract;
import androidx.core.content.FileProvider;
import androidx.core.app.ShareCompat;
import android.content.pm.PackageManager;
import java.util.List;
import android.content.pm.ResolveInfo;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

// this is the main class that will be run at start
public class SubsurfaceMobileActivity extends QtActivity
{
	// FileProvider declares an 'authority' in AndroidManifest.xml
	private static String fileProviderAuthority="org.subsurfacedivelog.mobile.fileprovider";

	// you can share one or two files
	public boolean shareViaEmail(String subject, String recipient, String body, String path1, String path2) {
		Log.d(TAG + " shareFile - trying to share: ", path1 + " and " + path2 + " to " + recipient);

		// Can't get this to work building my own intent, so let's use the IntentBuilder
		Intent shareFileIntent = new ShareCompat.IntentBuilder(this).getIntent();
		shareFileIntent.setAction(Intent.ACTION_SEND_MULTIPLE);
		// recipients are always an array, even if there's only one
		shareFileIntent.putExtra(Intent.EXTRA_EMAIL, new String[] { recipient });
		shareFileIntent.putExtra(Intent.EXTRA_SUBJECT, subject);
		shareFileIntent.putExtra(Intent.EXTRA_TEXT, body);

		// now figure out the URI we need to share the first file
		File fileToShare = new File(path1);
		Uri uri;
		try {
			uri = FileProvider.getUriForFile(this, fileProviderAuthority, fileToShare);
		} catch (IllegalArgumentException e) {
			Log.d(TAG + " shareFile - cannot get URI for ", path1);
			return false;
		}
		Log.d(TAG + " shareFile - URI for file: ", uri.toString());

		// because we allow up to two attachments, we use ACTION_SEND_MULTIPLE and so the attachments
		// need to be an ArrayList - even if we only add one attachment this still works
		ArrayList<Uri> attachments = new ArrayList<Uri>();
		attachments.add(uri);

		// if there is a second file name (that's for support emails) add it and set this up as support email as well
		if (!path2.isEmpty()) {
			fileToShare = new File(path2);
			try {
				uri = FileProvider.getUriForFile(this, fileProviderAuthority, fileToShare);
			} catch (IllegalArgumentException e) {
				Log.d(TAG + " shareFile - cannot get URI for ", path2);
				return false;
			}
			Log.d(TAG + " shareFile - URI for file: ", uri.toString());
			attachments.add(uri);
		}
		shareFileIntent.setType("text/plain");
		shareFileIntent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, attachments);
		shareFileIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
		shareFileIntent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
		Log.d(TAG + " sendFile ", " create activity for the Intent");
		// this actually allows sharing with any app that will take "text/plain" files, including Dropbox, etc
		// in order for the recipient / subject to work, the user needs to be clever enough to share with an email app
		this.startActivity(shareFileIntent);
		return true;
	}

	// AI-generated (Claude)
	// Lets the user save a copy of the file wherever they choose, through the
	// system's save dialog (Storage Access Framework). That needs no storage
	// permission: the app only writes to the document the user created.
	//
	// Each save works on its own snapshot of the export: writeExportFile()
	// always writes to the same path, and another export started while a slow
	// provider is still copying must not change (or de-anonymise) this one.
	// The snapshot is the export file itself, renamed out of the way.
	private static final int SAVE_FILE_REQUEST = 4711;
	private File pendingSnapshot;

	// reports a failed save to the user (core/android.cpp)
	private static native void saveFileFailed();

	private static void copyStream(InputStream in, OutputStream out) throws IOException {
		if (out == null)
			throw new IOException("no output stream");
		byte[] buffer = new byte[65536];
		int n;
		while ((n = in.read(buffer)) > 0)
			out.write(buffer, 0, n);
	}

	private synchronized File takePendingSnapshot(File replacement) {
		File previous = pendingSnapshot;
		pendingSnapshot = replacement;
		return previous;
	}

	public boolean saveFile(String path) {
		Log.d(TAG + " saveFile - offering to save: ", path);
		final File export = new File(path);
		final File snapshot;
		try {
			// in the export's own folder, so that it can be renamed into place
			snapshot = File.createTempFile("export-", null, export.getParentFile());
		} catch (IOException e) {
			Log.e(TAG + " saveFile", "cannot create a snapshot for " + path, e);
			saveFileFailed();
			return false;
		}
		// Moving the fresh export aside is instant, however large the dive log,
		// and the next export simply writes a new file at the shared path.
		// A rename within one folder of the app's storage is not expected to
		// fail; if it does, report it rather than copy from a file that the
		// next export may rewrite meanwhile.
		if (!export.renameTo(snapshot)) {
			Log.e(TAG + " saveFile", "cannot move " + path + " to " + snapshot.getPath());
			snapshot.delete();
			saveFileFailed();
			return false;
		}
		offerToSave(snapshot, export.getName());
		return true;
	}

	private void offerToSave(File snapshot, String fileName) {
		// a save dialog that never returned leaves its snapshot behind
		File stale = takePendingSnapshot(snapshot);
		if (stale != null)
			stale.delete();

		// octet-stream, so that no provider appends an extension of its own to the name
		Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
		intent.addCategory(Intent.CATEGORY_OPENABLE);
		intent.setType("application/octet-stream");
		intent.putExtra(Intent.EXTRA_TITLE, fileName);
		runOnUiThread(() -> startActivityForResult(intent, SAVE_FILE_REQUEST));
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		if (requestCode != SAVE_FILE_REQUEST)
			return;
		final File snapshot = takePendingSnapshot(null);
		if (resultCode != RESULT_OK || data == null || data.getData() == null) {
			Log.d(TAG + " saveFile", "cancelled");
			if (snapshot != null)
				snapshot.delete();
			return;
		}
		final Uri target = data.getData();
		if (snapshot == null || !snapshot.exists()) {
			// the snapshot got lost (the activity was recreated and the cache cleared)
			Log.e(TAG + " saveFile", "no snapshot to save to " + target.toString());
			deleteIncompleteDocument(target);
			saveFileFailed();
			return;
		}
		// a dive log can be tens of MB: copy it off the UI thread
		new Thread(() -> {
			boolean saved = false;
			try {
				try (InputStream in = new FileInputStream(snapshot);
				     OutputStream out = getContentResolver().openOutputStream(target, "wt")) {
					copyStream(in, out);
				}
				// only now: closing the output stream can fail, too
				saved = true;
			} catch (Exception e) {
				Log.e(TAG + " saveFile", "failed to save to " + target.toString(), e);
			}
			snapshot.delete();
			if (saved) {
				Log.d(TAG + " saveFile", "saved to " + target.toString());
				return;
			}
			deleteIncompleteDocument(target);
			saveFileFailed();
		}).start();
	}

	// don't leave a truncated file behind that looks like a good export
	private void deleteIncompleteDocument(Uri target) {
		try {
			DocumentsContract.deleteDocument(getContentResolver(), target);
		} catch (Exception e) {
			Log.e(TAG + " saveFile", "cannot remove the incomplete " + target.toString(), e);
		}
	}

	// the save dialog may outlive this activity instance: keep the snapshot's path
	private static final String SAVE_SNAPSHOT_KEY = "pendingSaveSnapshot";

	@Override
	protected void onSaveInstanceState(Bundle outState) {
		super.onSaveInstanceState(outState);
		synchronized (this) {
			if (pendingSnapshot != null)
				outState.putString(SAVE_SNAPSHOT_KEY, pendingSnapshot.getPath());
		}
	}

	public boolean supportEmail(String path1, String path2) {
		return shareViaEmail("Subsurface-mobile support request",
				"in-app-support@subsurface-divelog.org",
				"Please describe your issue here and keep the attached logs.\n\n\n\n",
				path1, path2);
	}

	public static boolean isIntentPending;
	public static boolean isInitialized;
	private static final String TAG = "subsurfacedivelog.mobile";
	public static native void setUsbDevice(UsbDevice usbDevice);
	public static native void restartDownload(UsbDevice usbDevice);
	private static Context appContext;

	// AI-generated (Claude)
	// On API 36 the system forces edge-to-edge, so the status/navigation bar
	// backgrounds are always transparent and our app's own content shows
	// through them. That means the OS can no longer pick readable icon
	// colors for us (it falls back to the system light/dark setting, which
	// can clash with our own app theme), so we set icon appearance
	// explicitly to match whatever color QMLManager::setStatusbarColor()
	// is painting behind the bars.
	public void setStatusBarIconAppearance(boolean darkIcons)
	{
		// Calls into this method come from JNI (QMLManager::setStatusbarColor);
		// an uncaught exception there gets silently swallowed on the C++
		// side, so log defensively until this is confirmed working across
		// devices.
		//
		// The very first call happens during startup, before the decor view
		// has gone through its first layout pass; WindowInsetsController
		// calls made that early can be silently overridden by Android's own
		// default appearance assignment right after. Posting to the decor
		// view defers the call until after that pass, so our value always
		// wins instead of racing it.
		final android.view.View decorView = getWindow().getDecorView();
		decorView.post(() -> {
			try {
				Log.d(TAG + " setStatusBarIconAppearance", "darkIcons=" + darkIcons);
				androidx.core.view.WindowInsetsControllerCompat controller =
					androidx.core.view.WindowCompat.getInsetsController(getWindow(), decorView);
				controller.setAppearanceLightStatusBars(darkIcons);
				controller.setAppearanceLightNavigationBars(darkIcons);
			} catch (Exception e) {
				Log.e(TAG + " setStatusBarIconAppearance", "failed", e);
			}
		});
	}

	// we need to provide two endpoints:
	// onNewIntent if we receive an Intent while running
	// onCreate    if we were started by an Intent
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		Log.i(TAG + " onCreate", "onCreate SubsurfaceMobileActivity");
		super.onCreate(savedInstanceState);

		// AI-generated (Claude): a save dialog still open from before a recreation
		if (savedInstanceState != null && savedInstanceState.getString(SAVE_SNAPSHOT_KEY) != null)
			takePendingSnapshot(new File(savedInstanceState.getString(SAVE_SNAPSHOT_KEY)));

		// AI-generated (Claude)
		// Let app content draw under the system bars from API 35 (Android 15) onward.
		// Opt in to edge-to-edge starting at 35, and rely on the safe-area strips painted
		// in main.qml (plus setStatusBarIconAppearance() below) to keep content readable
		// behind the bars. Pre-35 devices keep the normal, non-edge-to-edge layout.
		androidx.core.view.WindowCompat.setDecorFitsSystemWindows(getWindow(), Build.VERSION.SDK_INT < 35);

		// AI-generated (Claude)
		// On API 29+ Android enforces contrast by drawing a scrim over the
		// navigation bar (most visible with three-button navigation), which
		// would otherwise hide the color QMLManager::setStatusbarColor()
		// paints behind it. Disable that so our own color actually shows.
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
			getWindow().setNavigationBarContrastEnforced(false);

		appContext = getApplicationContext();

		// now we're checking if the App was started from another Android App via Intent
		Intent theIntent = getIntent();
		if (theIntent != null) {
			String theAction = theIntent.getAction();
			if (theAction != null) {
				Log.i(TAG + " onCreate", theAction);
				isIntentPending = true;
			}
		}

		// Register the usb permission intent filter.
		IntentFilter filter = new IntentFilter("org.subsurfacedivelog.mobile.USB_PERMISSION");
		registerReceiver(usbReceiver, filter, Context.RECEIVER_NOT_EXPORTED);

	} // onCreate

	// if we are opened from other apps:
	@Override
	public void onNewIntent(Intent intent)
	{
		Log.i(TAG + " onNewIntent", intent.getAction());
		UsbDevice device = getUsbDevice(intent);
		if (device == null) {
			Log.i(TAG + " onNewIntent", "null device");
			return;
		}
		Log.i(TAG + " onNewIntent toString", device.toString());
		super.onNewIntent(intent);
		setIntent(intent);
		// Intent will be processed, if all is initialized and Qt / QML can handle the event
		if (isInitialized) {
			processIntent();
		} else {
			isIntentPending = true;
		}
	} // onNewIntent

	public void checkPendingIntents()
	{
		isInitialized = true;
		if (isIntentPending) {
			isIntentPending = false;
			Log.i(TAG + " checkPendingIntents", "checkPendingIntents: true");
			processIntent();
		} else {
			Log.i(TAG + " checkPendingIntents", "nothingPending");
		}
	} // checkPendingIntents


	private void processIntent()
	{
		Intent intent = getIntent();
		if (intent == null) {
			Log.i(TAG + " processIntent", "intent is null");
			return;
		}
		Log.i(TAG + " processIntent", intent.getAction());
		UsbDevice device = getUsbDevice(intent);
		if (device == null) {
			Log.i(TAG + " processIntent", "null device");
			return;
		}
		Log.i(TAG + " processIntent device name", device.getDeviceName());
		try {
			setUsbDevice(device);
		} catch (LinkageError e) {
			// The native library may not be fully loaded yet.
			// checkPendingIntents() is only called once
			// during startup, so schedule a one-shot retry
			Log.w(TAG + " processIntent", "native not ready, scheduling retry", e);
			isIntentPending = true;
			new Handler(Looper.getMainLooper()).postDelayed(() -> {
				if (!isIntentPending)
					return;
				isIntentPending = false;
				UsbDevice retryDevice = getUsbDevice(getIntent());
				if (retryDevice == null)
					return;
				try {
					setUsbDevice(retryDevice);
				} catch (LinkageError retryError) {
					Log.e(TAG + " processIntent", "native still not ready after retry, giving up", retryError);
				}
			}, 500);
		}
	} // processIntent


	private final BroadcastReceiver usbReceiver = new BroadcastReceiver()
	{
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			if ("org.subsurfacedivelog.mobile.USB_PERMISSION".equals(action)) {
				synchronized (this) {
					if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
						UsbDevice device = getUsbDevice(intent);
						if (device == null) {
							Log.i(TAG, " permission granted but null device");
							return;
						}
						Log.d(TAG, "USB device permission granted for " + device.getDeviceName());
						restartDownload(device);
					} else {
						Log.d(TAG, "USB device permission denied");
					}
				}
			}
		}
	};

	public static Context getAppContext()
	{
		return appContext;
	}

	// getParcelableExtra(String) was deprecated in API 33; the typed overload
	// requires API 33, so guard by version.
	@SuppressWarnings("deprecation")
	private static UsbDevice getUsbDevice(Intent intent) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
			return intent.getParcelableExtra(UsbManager.EXTRA_DEVICE, UsbDevice.class);
		return intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
	}
}
