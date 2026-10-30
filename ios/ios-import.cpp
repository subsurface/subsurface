// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
//
// Callback bridge from the ObjC++ document-picker delegate in ios-share.mm
// back into Qt/C++ for "Load from file" - the iOS equivalent of the JNI
// free-function bridge in core/android.cpp (loadFileReady()/loadFileFailed()).
// This is a plain C++ (not ObjC++) translation unit, kept separate from
// ios-share.mm, so that only this file needs to include QMLManager;
// ios-share.mm stays focused on UIKit/Foundation interop and never needs to
// know about Qt's object model. This is also why IosShare itself doesn't
// need to become a QObject just for this one callback.
#include "ios-share.h"
#include "mobile-widgets/qmlmanager.h"
#include "core/errorhelper.h"
#include "core/gettext.h"

// Called by the document-picker delegate once the file picked for "Load
// from file" has been copied into the app's own sandbox (initForOpening...
// asCopy:YES). Unlike core/android.cpp's loadFileReady(), which must
// dispatch with Qt::QueuedConnection because the JNI callback can arrive on
// a worker thread, UIDocumentPickerViewControllerDelegate callbacks always
// run on the main thread, so importDivesFromFile() can be called directly -
// see that function's documentation for the full threading contract.
// removeSource is true because 'path' is a temporary sandbox copy made for
// this import, owned by us.
void iosLoadFileReady(const QString &path)
{
	QMLManager::instance()->importDivesFromFile(path, true);
}

// Called by the document-picker delegate when no usable file was picked
// (the picker returned no URL, or it could not be resolved to a local
// path). Cancelling the picker is handled separately in ios-share.mm and
// does not call this: it is silent and changes nothing, since no file was
// ever copied into the sandbox in that case.
void iosLoadFileFailed()
{
	report_error("%s", translate("gettextFromC", "Could not read the selected file"));
}
