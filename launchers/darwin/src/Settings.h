#import <Cocoa/Cocoa.h>


@interface Settings : NSObject {
}
@property (nonatomic, assign) NSInteger build;
@property (nonatomic, assign) BOOL loggedIn;
@property (nonatomic, assign) NSString* domain;
@property (nonatomic, assign) NSString* launcher;
@property (nonatomic, assign) NSString* _organizationBuildTag;
- (NSInteger) latestBuildVersion;
- (BOOL) isLoggedIn;
- (void) login:(BOOL)aLoggedIn;
- (void) buildVersion:(NSInteger) aBuildVersion;
- (void) setLauncherPath:(NSString*) aLauncherPath;
- (NSString*) getLaucnherPath;
- (void) setDomainUrl:(NSString*) aDomainUrl;
- (NSString*) getDomainUrl;
- (void) setOrganizationBuildTag:(NSString*) aOrganizationBuildTag;
- (NSString*) organizationBuildTag;
- (void) save;
+ (id) sharedSettings;
@end
