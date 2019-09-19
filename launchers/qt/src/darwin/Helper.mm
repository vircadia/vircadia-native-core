#include "../Helper.h"

#import "NSTask+NSTaskExecveAdditions.h"

#import <Cocoa/Cocoa.h>
#include <QString>
#include <QDebug>

void launchClient(const QString& clientPath, const QString& homePath, const QString& defaultScriptOverride,
                  const QString& displayName, const QString& contentCachePath, const QString& loginTokenResponse) {

    NSString* homeBookmark = [[NSString stringWithFormat:@"hqhome="] stringByAppendingString:homePath.toNSString()];
    NSArray* arguments;
    if (!loginTokenResponse.isEmpty()) {
        arguments = [NSArray arrayWithObjects:
                                 @"--url" , homePath.toNSString(),
                             @"--tokens", loginTokenResponse.toNSString(),
                             @"--cache", contentCachePath.toNSString(),
                             @"--displayName", displayName.toNSString(),
                             @"--defaultScriptsOverride", defaultScriptOverride.toNSString(),
                             @"--setBookmark", homeBookmark,
                             @"--no-updater",
                             @"--no-launcher", nil];
    } else {
        arguments = [NSArray arrayWithObjects:
                                 @"--url" , homePath.toNSString(),
                             @"--cache", contentCachePath.toNSString(),
                             @"--defaultScriptsOverride", defaultScriptOverride.toNSString(),
                             @"--setBookmark", homeBookmark,
                             @"--no-updater",
                             @"--no-launcher", nil];
    }

    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSURL *url = [NSURL fileURLWithPath:[workspace fullPathForApplication:clientPath.toNSString()]];
    NSLog(@"------> path %@: ", [url path]);
    NSTask *task = [[NSTask alloc] init];
    task.launchPath = [url path];
    task.arguments = arguments;
    [task replaceThisProcess];

}


void launchAutoUpdater(const QString& autoUpdaterPath) {
    NSTask* task = [[NSTask alloc] init]; 
    task.launchPath = [autoUpdaterPath.toNSString() stringByAppendingString:@"/Contents/Resources/updater"];
    task.arguments = @[[[NSBundle mainBundle] bundlePath], autoUpdaterPath.toNSString()];
    [task launch];

    exit(0);
}


@interface UpdaterHelper : NSObject
+(NSURL*) NSStringToNSURL: (NSString*) path;
@end

@implementation UpdaterHelper
+(NSURL*) NSStringToNSURL: (NSString*) path
{
    return [NSURL URLWithString: [path stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLFragmentAllowedCharacterSet]] relativeToURL: [NSURL URLWithString:@"file://"]];
}
@end


bool replaceDirectory(const QString& orginalDirectory, const QString& newDirectory) {
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSURL* destinationUrl = [UpdaterHelper NSStringToNSURL:newDirectory.toNSString()];
    return (bool) [fileManager replaceItemAtURL:[UpdaterHelper NSStringToNSURL:orginalDirectory.toNSString()] withItemAtURL:[UpdaterHelper NSStringToNSURL:newDirectory.toNSString()]
                                 backupItemName:nil options:NSFileManagerItemReplacementUsingNewMetadataOnly resultingItemURL:&destinationUrl error:nil];
}
