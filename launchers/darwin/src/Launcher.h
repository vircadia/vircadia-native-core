#import <Cocoa/Cocoa.h>
#import "DownloadInterface.h"
#import "CredentialsRequest.h"
#import "DownloadDomainContent.h"
#import "LatestBuildRequest.h"
#import "OrganizationRequest.h"
#import "DownloadScripts.h"

typedef enum processStateTypes
{
    DOWNLOADING_INTERFACE = 0,
    RUNNING_INTERFACE_AFTER_DOWNLOAD,
    CHECKING_UPDATE,
    RUNNING_INTERFACE_AFTER_UPDATE
} ProcessState;

typedef enum LoginErrorTypes
{
    NONE = 0,
    ORGANIZATION,
    CREDENTIALS
} LoginError;

@interface Launcher : NSObject <NSApplicationDelegate, NSWindowDelegate, NSURLDownloadDelegate> {
}
@property (nonatomic, retain) NSString* password;
@property (nonatomic, retain) NSString* username;
@property (nonatomic, retain) NSString* organization;
@property (nonatomic, retain) NSString* userToken;
@property (nonatomic, retain) NSString* displayName;
@property (nonatomic, retain) NSString* filename;
@property (nonatomic, retain) NSString* scriptsFilename;
@property (nonatomic, retain) NSString* contentFilename;
@property (nonatomic, retain) NSString* domainURL;
@property (nonatomic, retain) NSString* domainContentUrl;
@property (nonatomic, retain) NSString* domainScriptsUrl;
@property (nonatomic, retain) DownloadInterface* downloadInterface;
@property (nonatomic, retain) CredentialsRequest* credentialsRequest;
@property (nonatomic, retain) DownloadDomainContent* downloadDomainContent;
@property (nonatomic, retain) DownloadScripts* downloadScripts;
@property (nonatomic, retain) LatestBuildRequest* latestBuildRequest;
@property (nonatomic, retain) OrganizationRequest* organizationRequest;
@property (nonatomic) BOOL credentialsAccepted;
@property (nonatomic) BOOL waitingForCredentialReponse;
@property (nonatomic) BOOL gotCredentialResponse;
@property (nonatomic) BOOL waitingForInterfaceToTerminate;
@property (nonatomic, assign, readwrite) ProcessState processState;
@property (nonatomic, assign, readwrite) LoginError loginError;

- (void) displayNameEntered:(NSString*)aDisplayName;
- (void) credentialsEntered:(NSString*)aOrginization :(NSString*)aUsername :(NSString*)aPassword;
- (void) credentialsAccepted:(BOOL) aCredentialsAccepted;
- (void) domainContentDownloadFinished;
- (void) domainScriptsDownloadFinished;
- (void) setDomainURLInfo:(NSString*) aDomainURL :(NSString*) aDomainContentUrl :(NSString*) aDomainScriptsUrl;
- (void) organizationRequestFinished:(BOOL) aOriginzationAccepted;
- (BOOL) loginShouldSetErrorState;
- (void) displayErrorPage;
- (void) showLoginScreen;
- (NSString*) getLauncherPath;
- (ProcessState) currentProccessState;
- (void) setCurrentProcessState:(ProcessState) aProcessState;
- (void) setLoginErrorState:(LoginError) aLoginError;
- (LoginError) getLoginErrorState;
- (void) shouldDownloadLatestBuild:(BOOL) shouldDownload :(NSString*) downloadUrl;
- (void) interfaceFinishedDownloading;
- (NSString*) getDownloadPathForContentAndScripts;
- (void) launchInterface;
- (BOOL) extractZipFileAtDestination:(NSString*) destination :(NSString*) file;
- (BOOL) isWaitingForInterfaceToTerminate;
- (void) setDownloadFilename:(NSString*) aFilename;
- (void) setDownloadContextFilename:(NSString*) aFilename;
- (void) setDownloadScriptsFilename:(NSString*) aFilename;
- (void) setTokenString:(NSString*) aToken;
- (NSString*) getTokenString;
- (NSString*) getDownloadContentFilename;
- (NSString*) getDownloadScriptsFilename;
- (NSString*) getDownloadFilename;
- (BOOL) isLoadedIn;
- (NSString*) getAppPath;

+ (id) sharedLauncher;
@end

