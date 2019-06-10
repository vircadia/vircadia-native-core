#import <Cocoa/Cocoa.h>

@interface CredentialsRequest : NSObject <NSURLConnectionDelegate> {
}

@property (nonatomic, retain) NSMutableData* webData;
@property (nonatomic, retain) NSString* jsonString;

- (void) confirmCredentials:(NSString*)username :(NSString*)password;
@end
