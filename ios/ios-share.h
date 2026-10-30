// SPDX-License-Identifier: GPL-2.0
#ifndef IOSSHARE_H
#define IOSSHARE_H

// onlt Qt headers and data structures allowed here
#include <QString>

class IosShare {
public:
	IosShare();
	~IosShare();
	void supportEmail(const QString &firstPath, const QString &secondPath);
	void shareViaEmail(const QString &subject, const QString &recipient, const QString &body, const QString &firstPath, const QString &secondPath);
	// AI-generated (Claude): offers to save a copy of the file wherever the user picks
	void saveFile(const QString &path);
	// AI-generated (Claude): offers to pick a file to import dives from ("Load from file")
	void loadFile();
private:
	void *self;
};

// AI-generated (Claude)
// Callback bridge from the ObjC++ document-picker delegate in ios-share.mm
// back into Qt/C++ - the iOS equivalent of the JNI free-function bridge in
// core/android.cpp (loadFileReady()/loadFileFailed()). These are plain free
// functions, not IosShare methods, because they are called by the delegate
// *into* Qt code rather than by Qt code *into* the delegate; IosShare stays a
// plain C++ class rather than a QObject, so there is no signal/slot to use
// instead. They are implemented in ios/ios-import.cpp, a plain C++ (not
// ObjC++) translation unit that alone needs to include QMLManager, keeping
// ios-share.mm focused on UIKit/Foundation interop.
//
// iosLoadFileReady() hands the sandbox copy of the file picked for "Load
// from file" over for import; the document picker delegate always runs on
// the main thread, so (unlike the Android JNI bridge, callable from a worker
// thread) no Qt::QueuedConnection dispatch is needed here.
// iosLoadFileFailed() reports that no usable file was picked.
void iosLoadFileReady(const QString &path);
void iosLoadFileFailed();

#endif /* IOSSHARE_H */
