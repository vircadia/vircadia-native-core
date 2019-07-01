#import "Window.h"

@implementation Window
-(BOOL)canBecomeKeyWindow {
    return YES;
}

-(BOOL)isMovableByWindowBackground
{
    return TRUE;
}

@end
