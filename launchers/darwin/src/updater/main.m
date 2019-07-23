#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

@interface UpdaterHelper : NSObject
+(NSURL*) NSStringToNSURL: (NSString*) path;
@end

@implementation UpdaterHelper
+(NSURL*) NSStringToNSURL: (NSString*) path
{
    return [NSURL URLWithString: [path stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLFragmentAllowedCharacterSet]] relativeToURL: [NSURL URLWithString:@"file://"]];
}
@end

int main(int argc, char const* argv[])
{
    if (argc < 3)
    {
        NSLog(@"Error: wrong number of arguments");
        return 0;
    }

    for (int index = 0; index < argc; index++) {
        NSLog(@"argv at index %d = %s", index, argv[index]);
    }
    NSString* oldLauncher = [NSString stringWithUTF8String:argv[1]];
    NSString* newLauncher = [NSString stringWithUTF8String:argv[2]];
    NSFileManager* fileManager = [NSFileManager defaultManager];
    [fileManager removeItemAtURL:[UpdaterHelper NSStringToNSURL:oldLauncher] error:nil];
    [fileManager moveItemAtURL: [UpdaterHelper NSStringToNSURL: newLauncher] toURL: [UpdaterHelper NSStringToNSURL:oldLauncher] error:nil];
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    NSURL* applicationURL = [UpdaterHelper NSStringToNSURL: [oldLauncher stringByAppendingString: @"/Contents/MacOS/HQ Launcher"]];
    NSArray* arguments =@[];
    NSLog(@"Launcher agruments: %@", arguments);
    [workspace launchApplicationAtURL:applicationURL options:NSWorkspaceLaunchNewInstance configuration:[NSDictionary dictionaryWithObject:arguments forKey:NSWorkspaceLaunchConfigurationArguments] error:nil];
    return 0;
}
