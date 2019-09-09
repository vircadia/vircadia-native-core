#import <Foundation/Foundation.h>

@interface DownloadLauncher :  NSObject<NSURLSessionDataDelegate, NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLDownloadDelegate> {
}

@property (readonly) bool didBecomeDownloadTask;

- (void) downloadLauncher:(NSString*) launcherUrl;

@end
