// SPDX-License-Identifier: GPL-2.0
//
// this code was inspired by the discussions in
// https://forum.qt.io/topic/88297/native-objective-c-calls-from-cpp-qt-ios-email-call

// this include file only has the C++/Qt headers that can be used from C++
#include "ios-share.h"

// these are the required ObjC++ headers
#import <Foundation/Foundation.h>
#import <Foundation/NSString.h>
#import <UIKit/UIKit.h>
#import <MessageUI/MessageUI.h>

// declare an ObjC++ class that will interact with the mail controller
// that second member that is called when the mail app is finished is critical for this to work
@interface IosShareObject : UIViewController  <MFMailComposeViewControllerDelegate>
{
}
- (void)shareViaEmail:(const QString &) subject :(const QString &) recipient :(const QString &) body :(const QString &) firstPath :(const QString &) secondPath;
- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(nullable NSError *)error;
@end

@implementation IosShareObject
// first, inside the implementation of the ObjC++ class, implement the Qt class
IosShare::IosShare() : self(NULL) {
	// call init to ensure that the ObjC++ object is instantiated, which in return
	// apparently sets up the Controller
	self = [ [IosShareObject alloc] init];
}

IosShare::~IosShare() {
//  this call below apparently caused a crash at exit.
//  since at exit I really don't care much about a memory leak, this should be fine.
//	[(id)self dealloc];
}

// simplified method that fills subject, recipient, and body for support emails
void IosShare::supportEmail(const QString &firstPath, const QString &secondPath) {
	QString subject("Subsurface-mobile support request");
	QString recipient("in-app-support@subsurface-divelog.org");
	QString body("Please describe your issue here and keep the attached logs.\n\n\n\n");
	shareViaEmail(subject, recipient, body, firstPath, secondPath);
}

void IosShare::shareViaEmail(const QString &subject, const QString &recipient, const QString &body, const QString &firstPath, const QString &secondPath) {
	// ObjC++ syntax to call the shareViaEmail method of that class - so this is
	// where we transition from Qt/C++ code to ObjC++ code that can interact
	// directly with iOS
	[(id)self shareViaEmail:subject:recipient:body:firstPath:secondPath];
}

// the rest is the ObjC++ implementation
- (instancetype)init {
	// this is just boiler plate that I really don't understand
	// it appears to make sure that the ViewController infrastructure is initialized?
	return super.init;
}

- (void)shareViaEmail:(const QString &) subjectQS :(const QString &) recipientQS :(const QString &) bodyQS :(const QString &) firstPathQS :(const QString &) secondPathQS {
	// since we are mixing Qt and ObjC++ data structures, let's allocate copies
	// of our Qt strings and convert recipients into an array
	NSString *firstPath = [[NSString alloc] initWithUTF8String:firstPathQS.toUtf8().data()];
	NSString *secondPath = [[NSString alloc] initWithUTF8String:secondPathQS.toUtf8().data()];
	NSString *subject = [[NSString alloc] initWithUTF8String:subjectQS.toUtf8().data()];
	NSString *recipient = [[NSString alloc] initWithUTF8String:recipientQS.toUtf8().data()];
	NSString *body = [[NSString alloc] initWithUTF8String:bodyQS.toUtf8().data()];
	NSArray *recipents = [NSArray arrayWithObject:recipient];
	// create the mail controller and connect it with the object
	MFMailComposeViewController *mc = [[MFMailComposeViewController alloc] init];
	mc.mailComposeDelegate = self;
	[mc setSubject:subject];
	[mc setMessageBody:body isHTML:NO];
	[mc setToRecipients:recipents];
	// set up up to two attachments - only if we have a path and the file isn't empty (iOS throws up if you have an empty attachment)
	if (!firstPathQS.isEmpty()) {
		NSData *myData = [NSData dataWithContentsOfFile: firstPath];
		if (myData != nil)
			[mc addAttachmentData:myData mimeType:@"text/plain" fileName:[firstPath lastPathComponent]];
	}
	if (!secondPathQS.isEmpty()) {
		//NSString *path = [[NSBundle mainBundle] pathForResource:@"log2" ofType:@"txt"];
		NSData *myData = [NSData dataWithContentsOfFile: secondPath];
		if (myData != nil)
			[mc addAttachmentData:myData mimeType:@"text/plain" fileName:[secondPath lastPathComponent]];
	}
	// more black magic; get a view controller that is connected to our application window
	UIViewController * topController = [UIApplication sharedApplication].keyWindow.rootViewController;
	while (topController.presentedViewController){
		topController = topController.presentedViewController;
	}
	// finally, show the controller - the code returns right away, which is why we need the 'didFinishWithResult' method below
	[topController presentViewController:mc animated:YES completion:NULL];
}

// I would have kinda liked to inform the caller that sending mail failed, but I can't figure
// out how to get that information back to the Qt code calling us. Oh well. At least we log the results.
// But the critically important part is that we dismiss the view controller.
- (void) mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(nullable NSError *)error {
	switch (result) {
	case MFMailComposeResultCancelled:
		NSLog(@"Mail cancelled");
		break;
	case MFMailComposeResultSaved:
		NSLog(@"Mail saved");break;
	case MFMailComposeResultSent:
		NSLog(@"Mail sent");break;
	case MFMailComposeResultFailed:
		NSLog(@"Mail sent failure: %@", [error localizedDescription]);
		break;
	default:
		break;
	}
	[controller dismissViewControllerAnimated:YES completion:NULL];
}
@end
