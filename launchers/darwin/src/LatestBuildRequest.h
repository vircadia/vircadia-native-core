#import <Cocoa/Cocoa.h>

@interface LatestBuildRequest : NSObject <NSURLConnectionDelegate> {
}

@property (nonatomic, retain) NSMutableData* webData;
@property (nonatomic, retain) NSString* jsonString;

- (void) requestLatestBuildInfo;
@end
