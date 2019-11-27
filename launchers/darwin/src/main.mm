#import "Launcher.h"
#import "Settings.h"
#import "LauncherCommandlineArgs.h"

void redirectLogToDocuments()
{
    NSString* filePath = [[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) objectAtIndex:0]
                          stringByAppendingString:@"/Launcher/"];

    if (![[NSFileManager defaultManager] fileExistsAtPath:filePath]) {
        NSError * error = nil;
        [[NSFileManager defaultManager] createDirectoryAtPath:filePath withIntermediateDirectories:TRUE attributes:nil error:&error];
    }
    NSString *pathForLog = [filePath stringByAppendingPathComponent:@"log.txt"];

    freopen([pathForLog cStringUsingEncoding:NSASCIIStringEncoding],"a+",stderr);
}

int main(int argc, const char* argv[]) {
    //NSApp.appearance = [NSAppearance appearanceNamed: NSAppearanceNameAqua];
    redirectLogToDocuments();
    NSArray* apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.highfidelity.launcher"];
    if ([apps count] > 1) {
        NSLog(@"launcher is already running");
        return 0;
    }

    [NSApplication sharedApplication];
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    [Settings sharedSettings];
    [NSApp setDelegate: sharedLauncher];

    // Referenced from https://stackoverflow.com/questions/9155015/handle-cmd-q-in-cocoa-application-and-menu-item-quit-application-programmatic
    id menubar = [[NSMenu new] autorelease];
    id appMenuItem = [[NSMenuItem new] autorelease];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];
    id appMenu = [[NSMenu new] autorelease];
    id appName = [[NSProcessInfo processInfo] processName];
    id quitTitle = [@"Quit " stringByAppendingString:appName];
    id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];

    [[NSApplication sharedApplication] activateIgnoringOtherApps:TRUE];
    return NSApplicationMain(argc, argv);
}
