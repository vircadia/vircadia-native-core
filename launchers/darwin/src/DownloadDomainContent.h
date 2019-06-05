#import <Cocoa/Cocoa.h>

@interface DownloadDomainContent : NSObject<NSURLSessionDataDelegate, NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLDownloadDelegate> {
}

- (void) downloadDomainContent:(NSString*) domainContentUrl;

@end
