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

// this is the main class that will be run at start
public class SubsurfaceMobileActivity extends QtActivity
{
	public static boolean isIntentPending;
	public static boolean isInitialized;
	private static final String TAG = "subsurfacedivelog.mobile";

	// we need to provide two endpoints:
	// onNewIntent if we receive an Intent while running
	// onCreate    if we were started by an Intent
	@Override
	public void onCreate(Bundle savedInstanceState) {
		Log.i(TAG + " onCreate", "onCreate SubsurfaceMobileActivity");
		super.onCreate(savedInstanceState);

		// now we're checking if the App was started from another Android App via Intent
		Intent theIntent = getIntent();
		if (theIntent != null) {
			String theAction = theIntent.getAction();
			if (theAction != null) {
				Log.i(TAG + " onCreate", theAction);
				isIntentPending = true;
			}
		}
	} // onCreate

	// if we are opened from other apps:
	@Override
	public void onNewIntent(Intent intent) {
		Log.i(TAG + " onNewIntent", intent.getAction());
		UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
		Log.i(TAG + " onNewIntent device name", device.getDeviceName());
	//	if (Build.VERSION.SDK_INT > 20) {
	//		Log.i(TAG + " onNewIntent manufacturer name", device.getManufacturerName());
	//		Log.i(TAG + " onNewIntent product name", device.getProductName());
	//	}
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

	public void checkPendingIntents(String workingDir) {
		isInitialized = true;
		Log.i(TAG + " checkPendingIntents", workingDir);
		if (isIntentPending) {
			isIntentPending = false;
			Log.i(TAG + " checkPendingIntents", "checkPendingIntents: true");
			processIntent();
		} else {
			Log.i(TAG + " checkPendingIntents", "nothingPending");
		}
	} // checkPendingIntents


	private void processIntent() {
		Intent intent = getIntent();
		Log.i(TAG + " processIntent", intent.getAction());
		UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
		Log.i(TAG + " processIntent device name", device.getDeviceName());
	//	if (Build.VERSION.SDK_INT > 20) {
	//		Log.i(TAG + " processIntent manufacturer name", device.getManufacturerName());
	//		Log.i(TAG + " processIntent product name", device.getProductName());
	//	}
		Log.i(TAG + " processIntent toString", device.toString());
	} // processIntent
}
