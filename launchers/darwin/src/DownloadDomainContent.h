#import <Cocoa/Cocoa.h>

@interface DownloadDomainContent : NSObject<NSURLSessionDataDelegate, NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLDownloadDelegate> {
}
@property (nonatomic, assign) double progressPercentage;
@property (nonatomic, assign) double taskProgressPercentage;
- (void) downloadDomainContent:(NSString*) domainContentUrl;

- (double) getProgressPercentage;

@end
