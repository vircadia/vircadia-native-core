#import <Cocoa/Cocoa.h>

@interface DownloadDomainContent : NSObject<NSURLDownloadDelegate> {
}

- (void) downloadDomainContent:(NSString*) domainContentUrl;

@end
