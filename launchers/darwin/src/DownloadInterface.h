#import <Cocoa/Cocoa.h>

@interface DownloadInterface : NSObject<NSURLSessionDataDelegate, NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLDownloadDelegate> {
}
@property (nonatomic, assign) NSString* finalFilePath;
@property (nonatomic, assign) double progressPercentage;

- (void) downloadInterface:(NSString*) downloadUrl;

- (double) getProgressPercentage;
@end
