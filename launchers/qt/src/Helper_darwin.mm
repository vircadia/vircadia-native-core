#include "Helper.h"

#import "NSTask+NSTaskExecveAdditions.h"

#import <Cocoa/Cocoa.h>
#include <QString>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>

void launchClient(const QString& clientPath, const QString& homePath, const QString& defaultScriptOverride,
                  const QString& displayName, const QString& contentCachePath, QString loginTokenResponse) {

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
                             @"--no-launcher",
                             @"--suppress-settings-reset", nil];
    } else {
        arguments = [NSArray arrayWithObjects:
                                 @"--url" , homePath.toNSString(),
                             @"--cache", contentCachePath.toNSString(),
                             @"--defaultScriptsOverride", defaultScriptOverride.toNSString(),
                             @"--setBookmark", homeBookmark,
                             @"--no-updater",
                             @"--no-launcher",
                             @"--suppress-settings-reset", nil];
    }

    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSURL *url = [NSURL fileURLWithPath:[workspace fullPathForApplication:clientPath.toNSString()]];
    NSTask *task = [[NSTask alloc] init];
    task.launchPath = [url path];
    task.arguments = arguments;
    [task replaceThisProcess];

}

QString getBundlePath() {
    return QString::fromNSString([[NSBundle mainBundle] bundlePath]);
}


void launchAutoUpdater(const QString& autoUpdaterPath) {
    NSException *exception;
    bool launched = false;
    // Older versions of Launcher put updater in `/Contents/Resources/updater`.
    NSString* newLauncher = autoUpdaterPath.toNSString();
    for (NSString *bundlePath in @[@"/Contents/MacOS/updater",
                                   @"/Contents/Resources/updater",
                                   ]) {
        NSTask* task = [[NSTask alloc] init];
        task.launchPath = [newLauncher stringByAppendingString: bundlePath];
        task.arguments = @[[[NSBundle mainBundle] bundlePath], newLauncher];

        qDebug() << "launching updater: " << task.launchPath << task.arguments;

        @try {
            [task launch];
        }
        @catch (NSException *e) {
            qDebug() << "couldn't launch updater: " << QString::fromNSString(e.name) << QString::fromNSString(e.reason);
            exception = e;
            continue;
        }

        launched = true;
        break;
    }

    if (!launched) {
        @throw exception;
    }

    QCoreApplication::instance()->quit();
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
    NSError *error = nil;
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSURL* destinationUrl = [UpdaterHelper NSStringToNSURL:newDirectory.toNSString()];
    bool success = (bool) [fileManager replaceItemAtURL:[UpdaterHelper NSStringToNSURL:orginalDirectory.toNSString()] withItemAtURL:[UpdaterHelper NSStringToNSURL:newDirectory.toNSString()]
                                 backupItemName:nil options:NSFileManagerItemReplacementUsingNewMetadataOnly resultingItemURL:&destinationUrl error:&error];

    if (error != nil) {
        qDebug() << "NSFileManager::replaceItemAtURL -> error: " << error;
    }

    return success;
}


void waitForInterfaceToClose() {
    bool interfaceRunning = true;

    while (interfaceRunning) {
        interfaceRunning = false;
        NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
        NSArray* apps = [workspace runningApplications];
        for (NSRunningApplication* app in apps) {
            if ([[app bundleIdentifier] isEqualToString:@"com.highfidelity.interface"] ||
                [[app bundleIdentifier] isEqualToString:@"com.highfidelity.interface-pr"]) {
                interfaceRunning = true;
                break;
            }
        }
    }
}

bool isLauncherAlreadyRunning() {
    NSArray* apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.highfidelity.launcher"];
    if ([apps count] > 1) {
        qDebug() << "launcher is already running";
        return true;
    }

    return false;
}

void closeInterfaceIfRunning() {
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    NSArray* apps = [workspace runningApplications];
    for (NSRunningApplication* app in apps) {
        if ([[app bundleIdentifier] isEqualToString:@"com.highfidelity.interface"] ||
            [[app bundleIdentifier] isEqualToString:@"com.highfidelity.interface-pr"]) {
            [app terminate];
        }
    }
}
