//
//  main.m
//  NoNIBCodeOnlyGUIAppBootstrapping
//
//  Created by Hoon H. on 2014/06/13.
//  Copyright (c) 2014 Eonil. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "main.h"

@interface	BoilerplateApplicationController : NSResponder <NSApplicationDelegate>
- (void)userTapQuitMenu:(id)sender;
@end

@implementation BoilerplateApplicationController
{
	NSWindow*	_main_window;
}
- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	{
		/*!
		 But it's OK to create after launching finished.
		 */
        /*
		_main_window	=	[[NSWindow alloc] init];
		[_main_window setStyleMask:NSResizableWindowMask | NSClosableWindowMask | NSTitledWindowMask | NSMiniaturizableWindowMask];
		[_main_window setContentSize:CGSizeMake(400, 300)];
		[_main_window makeKeyAndOrderFront:self];
	    */
    }
	
	/*!
	 Make a menu to let you to quit application by @c Command+Q.
	 
	 Notice that;
	 
	 -	"Menu 1" means the main menu bar itself. So it will not be visible.
	 -	"Item 2" means the application menu item. The specified title will be ignored,
		and will be replaced by system provided application name.
	 -	"Menu 3" means a drop-down menu when you cliked the application menu.
	 -	"Item 4" means the first menu will be visible to you.
	 -	@c keyEquivalent of "Item 4" is specified as lowercase letter to avoid requiring
		shift key combination. If you put uppercase letter, then you have to press shift
		key for the shortcut.
	 
	 The selector @c userTapQuitMenu: will be passed to first-responder, and the 
	 application-delegate object in this case, which is an instance of @c TestApp1 class.
	 
	 */
	/*
	NSCAssert([[NSApplication sharedApplication] mainMenu] == nil, @"An application should have no main-menu at first.");

	SEL			s1	=	NSSelectorFromString(@"userTapQuitMenu:");		//	Made from a string to avoid warning.
	
	NSMenu*		m1	=	[[NSMenu alloc] initWithTitle:@"Menu 1"];
	NSMenuItem*	m2	=	[[NSMenuItem alloc] initWithTitle:@"Item 2" action:s1 keyEquivalent:@""];
	
	NSMenu*		m3	=	[[NSMenu alloc] initWithTitle:@"Menu 3"];
	NSMenuItem*	m4	=	[[NSMenuItem alloc] initWithTitle:@"Item 4" action:s1 keyEquivalent:@"q"];
	
	[[NSApplication sharedApplication] setMainMenu:m1];
	[m1 addItem:m2];
	[m2 setSubmenu:m3];
	[m3 addItem:m4];*/
    cardpeek_main(0,NULL);
    /* [NSApp terminate: nil]; */
    [[NSApplication sharedApplication] terminate:self];
}
- (void)userTapQuitMenu:(id)sender
{
	[[NSApplication sharedApplication] terminate:self];
}
@end


int main(int argc, const char * argv[])
{
	@autoreleasepool
	{
		BoilerplateApplicationController*		del1	=	[[BoilerplateApplicationController alloc] init];
		NSApplication*	app1	=	[NSApplication sharedApplication];		///	Let it to be created by accessing it.
		[app1 setDelegate:del1];
		[app1 run];
	}
}
