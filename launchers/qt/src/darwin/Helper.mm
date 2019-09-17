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
