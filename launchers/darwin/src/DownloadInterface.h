#import <Cocoa/Cocoa.h>

@interface DownloadInterface : NSObject<NSURLSessionDataDelegate, NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLDownloadDelegate> {
}
@property (nonatomic, assign) NSString* finalFilePath;

- (void) downloadInterface:(NSString*) downloadUrl;
@end
