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

int main(int argc, char const* argv[]) {
    redirectLogToDocuments();
    if (argc < 3) {
        NSLog(@"Error: wrong number of arguments");
        return 0;
    }

    for (int index = 0; index < argc; index++) {
        NSLog(@"argv at index %d = %s", index, argv[index]);
    }

    NSError *error = nil;

    NSString* oldLauncher = [NSString stringWithUTF8String:argv[1]];
    NSString* newLauncher = [NSString stringWithUTF8String:argv[2]];
    NSURL* destinationUrl = [UpdaterHelper NSStringToNSURL:newLauncher];
    NSFileManager* fileManager = [NSFileManager defaultManager];
    [fileManager replaceItemAtURL:[UpdaterHelper NSStringToNSURL:oldLauncher]
                    withItemAtURL:[UpdaterHelper NSStringToNSURL:newLauncher]
                   backupItemName:nil
                          options:NSFileManagerItemReplacementUsingNewMetadataOnly
                 resultingItemURL:&destinationUrl
                            error:&error];
    if (error != nil) {
        NSLog(@"couldn't update launcher: %@", error);
        return 1;
    }

    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    NSURL* applicationURL = [UpdaterHelper NSStringToNSURL: [oldLauncher stringByAppendingString: @"/Contents/MacOS/HQ Launcher"]];
    NSArray* arguments =@[];
    NSLog(@"Launcher agruments: %@", arguments);

    NSDictionary *configuration = [NSDictionary dictionaryWithObject:arguments
                                                              forKey:NSWorkspaceLaunchConfigurationArguments];
    [workspace launchApplicationAtURL:applicationURL
                              options:NSWorkspaceLaunchNewInstance
                        configuration:configuration
                                error:&error];

    if (error != nil) {
        NSLog(@"couldn't start launcher: %@", error);
        return 1;
    }

    return 0;
}
