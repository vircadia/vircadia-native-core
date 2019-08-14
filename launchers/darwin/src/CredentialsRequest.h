#import <Cocoa/Cocoa.h>

@interface CredentialsRequest : NSObject <NSURLSessionDelegate, NSURLSessionTaskDelegate> {
}

- (void) confirmCredentials:(NSString*)username :(NSString*)password;
@end
