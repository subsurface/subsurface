// SPDX-License-Identifier: GPL-2.0

// Java classes for handling Intents on Android (which in this case means
// Subsurface-mobile receiving notifaction that an FTDI dive computer was
// plugged in

// Created while looking at https://github.com/ekke/ekkesSHAREexample

package org.subsurfacedivelog.mobile;

import org.qtproject.qt5.android.QtNative;

import org.qtproject.qt5.android.bindings.QtActivity;
import android.os.*;
import android.content.*;
import android.app.*;

import java.lang.String;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.util.Log;
import java.io.File;
import android.net.Uri;
import android.support.v4.content.FileProvider;
import android.support.v4.app.ShareCompat;
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
		// better save than sorry
		if (QtNative.activity() == null)
			return false;

		Log.d(TAG + " shareFile - trying to share: ", path1 + " and " + path2 + " to " + recipient);

		// Can't get this to work building my own intent, so let's use the IntentBuilder
		Intent shareFileIntent = ShareCompat.IntentBuilder.from(QtNative.activity()).getIntent();
		shareFileIntent.setAction(Intent.ACTION_SEND_MULTIPLE);
		// recipients are always an array, even if there's only one
		shareFileIntent.putExtra(Intent.EXTRA_EMAIL, new String[] { recipient });
		shareFileIntent.putExtra(Intent.EXTRA_SUBJECT, subject);
		shareFileIntent.putExtra(Intent.EXTRA_TEXT, body);

		// now figure out the URI we need to share the first file
		File fileToShare = new File(path1);
		Uri uri;
		try {
			uri = FileProvider.getUriForFile(QtNative.activity(), fileProviderAuthority, fileToShare);
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
				uri = FileProvider.getUriForFile(QtNative.activity(), fileProviderAuthority, fileToShare);
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
		QtNative.activity().startActivity(shareFileIntent);
		return true;
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

	// we need to provide two endpoints:
	// onNewIntent if we receive an Intent while running
	// onCreate    if we were started by an Intent
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		Log.i(TAG + " onCreate", "onCreate SubsurfaceMobileActivity");
		super.onCreate(savedInstanceState);

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
		registerReceiver(usbReceiver, filter);

	} // onCreate

	// if we are opened from other apps:
	@Override
	public void onNewIntent(Intent intent)
	{
		Log.i(TAG + " onNewIntent", intent.getAction());
		UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
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
		UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
		if (device == null) {
			Log.i(TAG + " processIntent", "null device");
			return;
		}
		Log.i(TAG + " processIntent device name", device.getDeviceName());
		setUsbDevice(device);
	} // processIntent


	private final BroadcastReceiver usbReceiver = new BroadcastReceiver()
	{
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			if ("org.subsurfacedivelog.mobile.USB_PERMISSION".equals(action)) {
				synchronized (this) {
					if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
						UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
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
}
