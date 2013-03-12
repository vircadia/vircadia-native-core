#include "Oscilloscope.h"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#import "InterfaceMacOSX.h"

@class InterfaceMainMenuTarget;

static InterfaceMainMenuTarget *sharedInterfaceMainMenuTarget = nil;
static Oscilloscope *sharedAudioScope;


@interface InterfaceMainMenuTarget : NSObject

@property (strong, nonatomic) NSMenuItem *scopeAudioMenuItem;
@property (strong, nonatomic) NSMenuItem *scopeNoneMenuItem;

@end


@implementation InterfaceMainMenuTarget

- (void)scopeAudioAction {
    sharedAudioScope->setState(true);
    [self.scopeAudioMenuItem setState:NSOnState];
    [self.scopeNoneMenuItem setState:NSOffState];
}

- (void)scopeNoneAction {
    sharedAudioScope->setState(false);
    [self.scopeAudioMenuItem setState:NSOffState];
    [self.scopeNoneMenuItem setState:NSOnState];
}

@end


void initMacOSXMenu(Oscilloscope *audioScope) {
    @autoreleasepool {
        if (NSApp) {
            if (!sharedInterfaceMainMenuTarget) {
                sharedInterfaceMainMenuTarget = [[InterfaceMainMenuTarget alloc] init];
            }
            sharedAudioScope = audioScope;
            
            NSMenu *mainMenu = [NSApp mainMenu];
            
            NSMenuItem *scopeMenuItem = [mainMenu insertItemWithTitle:@"Scope"
                                                               action:nil
                                                         keyEquivalent:@""
                                                              atIndex:3];
            
            NSMenu *scopeMenu = [[[NSMenu alloc] init] initWithTitle:@"Scope"];
            [scopeMenuItem setSubmenu:scopeMenu];
            sharedInterfaceMainMenuTarget.scopeAudioMenuItem = [scopeMenu addItemWithTitle:@"Audio"
                                                                                    action:@selector(scopeAudioAction)
                                                                             keyEquivalent:@""];
            [sharedInterfaceMainMenuTarget.scopeAudioMenuItem setTarget:sharedInterfaceMainMenuTarget];
            [sharedInterfaceMainMenuTarget.scopeAudioMenuItem setState:NSOnState];
            
            sharedInterfaceMainMenuTarget.scopeNoneMenuItem = [scopeMenu addItemWithTitle:@"None"
                                                                                   action:@selector(scopeNoneAction)
                                                                            keyEquivalent:@""];
            [sharedInterfaceMainMenuTarget.scopeNoneMenuItem setTarget:sharedInterfaceMainMenuTarget];
        }
    }
}
