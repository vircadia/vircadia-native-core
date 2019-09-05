#import <Foundation/Foundation.h>

@interface DownloadLauncher :  NSObject<NSURLSessionDataDelegate, NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLDownloadDelegate> {
}

- (void) downloadLauncher:(NSString*) launcherUrl;

@end
