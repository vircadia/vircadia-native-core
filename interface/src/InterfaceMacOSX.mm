#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

@interface InterfaceMainMenuTarget : NSObject

@property (strong, nonatomic) NSMenuItem *scopeAudioMenuItem;
@property (strong, nonatomic) NSMenuItem *scopeNoneMenuItem;

@end

@implementation InterfaceMainMenuTarget

- (void)scopeAudioAction {
    NSLog(@"scopeAudioAction");
}

- (void)scopeNoneAction {
    NSLog(@"scopeNoneAction");
}

@end

static InterfaceMainMenuTarget *sharedInterfaceMainMenuTarget = nil;

void initMacOSXMenu() {
    @autoreleasepool {
        if (NSApp) {
            if (!sharedInterfaceMainMenuTarget) {
                sharedInterfaceMainMenuTarget = [[InterfaceMainMenuTarget alloc] init];
            }
            
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
            
            [NSApp setMainMenu:mainMenu];
        }
    }
}
