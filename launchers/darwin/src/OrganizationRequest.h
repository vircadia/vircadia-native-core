#import <Cocoa/Cocoa.h>

@interface OrganizationRequest : NSObject <NSURLConnectionDelegate> {
}

@property (nonatomic, retain) NSMutableData* webData;
@property (nonatomic, retain) NSString* jsonString;
@property (nonatomic, retain) NSString* username;

- (void) confirmOrganization:(NSString*) aOrganization :(NSString*) aUsername;
@end
