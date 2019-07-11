#import <Cocoa/Cocoa.h>

@interface DownloadInterface : NSObject<NSURLSessionDataDelegate, NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLDownloadDelegate> {
}
@property (nonatomic, assign) NSString* finalFilePath;
@property (nonatomic, assign) double progressPercentage;
@property (nonatomic, assign) double taskProgressPercentage;

- (void) downloadInterface:(NSString*) downloadUrl;

- (double) getProgressPercentage; 
@end
