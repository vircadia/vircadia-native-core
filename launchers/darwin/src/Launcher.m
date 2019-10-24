#import "Launcher.h"
#import "Window.h"
#import "SplashScreen.h"
#import "LoginScreen.h"
#import "DisplayNameScreen.h"
#import "LauncherCommandlineArgs.h"
#import "ProcessScreen.h"
#import "ErrorViewController.h"
#import "Settings.h"
#import "NSTask+NSTaskExecveAdditions.h"
#import "Interface.h"
#import "HQDefaults.h"

@interface Launcher ()

@property (strong) IBOutlet Window* window;
@property (nonatomic, assign) NSString* finalFilePath;
@property (nonatomic, assign) NSButton* button;
@end


static BOOL const DELETE_ZIP_FILES = TRUE;
@implementation Launcher

+ (id) sharedLauncher {
    static Launcher* sharedLauncher = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedLauncher = [[self alloc]init];
    });
    return sharedLauncher;
}

-(id)init {
    if (self = [super init]) {
        self.username = [[NSString alloc] initWithString:@"Default Property Value"];
        self.downloadInterface = [DownloadInterface alloc];
        self.downloadDomainContent = [DownloadDomainContent alloc];
        self.downloadLauncher = [DownloadLauncher alloc];
        self.credentialsRequest = [CredentialsRequest alloc];
        self.latestBuildRequest = [LatestBuildRequest alloc];
        self.organizationRequest = [OrganizationRequest alloc];
        self.downloadScripts = [DownloadScripts alloc];
        struct LatestBuildInfo latestBuildInfo;
        latestBuildInfo.downloadURL = nil;
        latestBuildInfo.shouldDownload = FALSE;
        latestBuildInfo.requestBuildFinished = FALSE;
        self.buildInfo = latestBuildInfo;
        self.credentialsAccepted = TRUE;
        self.gotCredentialResponse = FALSE;
        self.waitingForCredentialReponse = FALSE;
        self.waitingForInterfaceToTerminate = FALSE;
        self.latestBuildRequestFinished = FALSE;
        self.userToken = nil;
        self.progressIndicator = nil;
        self.processState = DOWNLOADING_INTERFACE;
    }
    return self;
}

-(void)awakeFromNib {
    [[NSApplication sharedApplication] activateIgnoringOtherApps:TRUE];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                           selector:@selector(didTerminateApp:)
                                                               name:NSWorkspaceDidTerminateApplicationNotification
                                                             object:nil];

    SplashScreen* splashScreen = [[SplashScreen alloc] initWithNibName:@"SplashScreen" bundle:nil];
    [self.window setContentViewController: splashScreen];
    [self closeInterfaceIfRunning];

    if (!self.waitingForInterfaceToTerminate) {
        [self checkLoginStatus];
    }
}

- (NSString*) getDownloadPathForContentAndScripts
{
    NSString* filePath = [[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) objectAtIndex:0]
                          stringByAppendingString:@"/Launcher/"];

    if (![[NSFileManager defaultManager] fileExistsAtPath:filePath]) {
        NSError * error = nil;
        [[NSFileManager defaultManager] createDirectoryAtPath:filePath withIntermediateDirectories:TRUE attributes:nil error:&error];
    }

    return filePath;
}

- (NSString*) getLauncherPath
{
    return [[[NSBundle mainBundle] bundlePath] stringByAppendingString:@"/Contents/MacOS/"];
}

- (void) updateProgressIndicator
{
    double contentPercentage = [self.downloadDomainContent getProgressPercentage];
    double interfacePercentage = [self.downloadInterface getProgressPercentage];
    double currentTotalPercentage = self.progressTarget;
    if (self.processState == DOWNLOADING_INTERFACE) {
        if (self.shouldDownloadInterface) {
            currentTotalPercentage = (contentPercentage * 0.5) + (interfacePercentage * 0.5);
        } else {
            currentTotalPercentage = contentPercentage;
        }
    } else {
        currentTotalPercentage = interfacePercentage;
    }
    self.progressTarget = currentTotalPercentage;
}

- (double) lerp:(double) pointA :(double) pointB :(double) interp
{
    double lerpValue = pointA + interp * (pointB - pointA);
    return lerpValue;
}

- (BOOL) extractZipFileAtDestination:(NSString *)destination :(NSString*)file
{
    NSTask* task = [[NSTask alloc] init];
    task.launchPath = @"/usr/bin/unzip";
    task.arguments = @[@"-o", @"-d", destination, file];

    [task launch];
    [task waitUntilExit];

    if (DELETE_ZIP_FILES) {
        NSFileManager* fileManager = [NSFileManager defaultManager];
        [fileManager removeItemAtPath:file error:NULL];
    }

    if ([task terminationStatus] != 0) {
        NSLog(@"Extracting file failed -> termination status: %d", [task terminationStatus]);
        return FALSE;
    }

    return TRUE;
}

-(void) setProgressView:(NSProgressIndicator*) aProgressIndicator
{
    self.progressIndicator = aProgressIndicator;
}

-(NSProgressIndicator*) getProgressView
{
    return self.progressIndicator;
}

- (void) restart
{
    SplashScreen* splashScreen = [[SplashScreen alloc] initWithNibName:@"SplashScreen" bundle:nil];
    [[[[NSApplication sharedApplication] windows] objectAtIndex:0] setContentViewController: splashScreen];

    [self checkLoginStatus];
}

- (void) displayErrorPage
{
    ErrorViewController* errorPage = [[ErrorViewController alloc] initWithNibName:@"ErrorScreen" bundle:nil];
    [[[[NSApplication sharedApplication] windows] objectAtIndex:0] setContentViewController: errorPage];
}

- (void) checkLoginStatus
{
    [NSTimer scheduledTimerWithTimeInterval:1.0
                                     target:self
                                   selector:@selector(onSplashScreenTimerFinished:)
                                   userInfo:nil
                                    repeats:NO];
    [[NSApplication sharedApplication] activateIgnoringOtherApps:TRUE];
}

- (void) setDownloadContextFilename:(NSString *)aFilename
{
    self.contentFilename = aFilename;
}

- (void) setDownloadScriptsFilename:(NSString*)aFilename
{
    self.scriptsFilename = aFilename;
}

- (NSString*) getDownloadContentFilename
{
    return self.contentFilename;
}

- (NSString*) getDownloadScriptsFilename
{
    return self.scriptsFilename;
}

- (void) startUpdateProgressIndicatorTimer
{
    self.progressTarget = 0.0;
    self.updateProgressIndicatorTimer = [NSTimer scheduledTimerWithTimeInterval: 0.0016
                                                                        target: self
                                                                      selector: @selector(updateIndicator:)
                                                                      userInfo:nil
                                                                       repeats: YES];

    [[NSRunLoop mainRunLoop] addTimer:self.updateProgressIndicatorTimer forMode:NSRunLoopCommonModes];
}

- (void) endUpdateProgressIndicatorTimer
{
    [self.updateProgressIndicatorTimer invalidate];
    self.updateProgressIndicatorTimer = nil;
}

- (void) updateIndicator:(NSTimer*) timer
{
    NSProgressIndicator* progressIndicator = [self getProgressView];
    double oldValue = progressIndicator.doubleValue;
    progressIndicator.doubleValue = [self lerp:oldValue :self.progressTarget :0.3];
}

- (void)didTerminateApp:(NSNotification *)notification {
    if (self.waitingForInterfaceToTerminate) {
        NSString* appName = [notification.userInfo valueForKey:@"NSApplicationName"];
        if ([appName isEqualToString:@"interface"]) {
            self.waitingForInterfaceToTerminate = FALSE;
            [self checkLoginStatus];
        }
    }
}

- (void) closeInterfaceIfRunning
{
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    NSArray* apps = [workspace runningApplications];
    for (NSRunningApplication* app in apps) {
        if ([[app bundleIdentifier] isEqualToString:@"com.highfidelity.interface"] ||
            [[app bundleIdentifier] isEqualToString:@"com.highfidelity.interface-pr"]) {
            [app terminate];
            self.waitingForInterfaceToTerminate = true;
        }
    }
}

- (BOOL) isWaitingForInterfaceToTerminate {
    return self.waitingForInterfaceToTerminate;
}

- (BOOL) isLoggedIn
{
    return [[Settings sharedSettings] isLoggedIn];
}

- (void) setDomainURLInfo:(NSString *)aDomainURL :(NSString *)aDomainContentUrl :(NSString *)aDomainScriptsUrl
{
    self.domainURL = aDomainURL;
    self.domainContentUrl = aDomainContentUrl;
    self.domainScriptsUrl = aDomainScriptsUrl;

    [[Settings sharedSettings] setDomainUrl:aDomainURL];
}

- (void) setOrganizationBuildTag:(NSString*) organizationBuildTag;
{
    [[Settings sharedSettings] setOrganizationBuildTag:organizationBuildTag];
}

- (NSString*) getAppPath
{
    return [self getDownloadPathForContentAndScripts];
}

- (BOOL) loginShouldSetErrorState
{
    return !self.credentialsAccepted;
}

- (void) displayNameEntered:(NSString*)aDiplayName
{
    self.processState = DOWNLOADING_INTERFACE;
    [self startUpdateProgressIndicatorTimer];
    ProcessScreen* processScreen = [[ProcessScreen alloc] initWithNibName:@"ProcessScreen" bundle:nil];
    [[[[NSApplication sharedApplication] windows] objectAtIndex:0] setContentViewController: processScreen];
    [self.downloadDomainContent downloadDomainContent:self.domainContentUrl];
    self.displayName = aDiplayName;
}

- (NSInteger) getCurrentVersion {
    NSInteger currentVersion;
    @try {
        NSString* interfaceAppPath = [[self getAppPath] stringByAppendingString:@"interface.app"];
        NSError* error = nil;
        Interface* interface = [[Interface alloc] initWith:interfaceAppPath];
        currentVersion = [interface getVersion:&error];
        if (currentVersion == 0 && error != nil) {
            NSLog(@"can't get version from interface: %@", error);
        }
    } @catch (NSException *exception) {
        NSLog(@"an exception was thrown while getting current interface version: %@", exception);
        currentVersion = 0;
    }
    return currentVersion;
}

- (void) domainContentDownloadFinished
{
    [self tryDownloadLatestBuild:TRUE];
}

- (void) domainScriptsDownloadFinished
{
    [self.latestBuildRequest requestLatestBuildInfo];
}

- (void) saveCredentialsAccepted:(BOOL)aCredentialsAccepted
{
    self.credentialsAccepted = aCredentialsAccepted;
}

- (void) credentialsAccepted:(BOOL)aCredentialsAccepted
{
    self.credentialsAccepted = aCredentialsAccepted;
    if (aCredentialsAccepted) {
        DisplayNameScreen* displayNameScreen = [[DisplayNameScreen alloc] initWithNibName:@"DisplayNameScreen" bundle:nil];
        [[[[NSApplication sharedApplication] windows] objectAtIndex:0] setContentViewController: displayNameScreen];
    } else {
        LoginScreen* loginScreen = [[LoginScreen alloc] initWithNibName:@"LoginScreen" bundle:nil];
        [[[[NSApplication sharedApplication] windows] objectAtIndex:0] setContentViewController: loginScreen];
    }
}

- (void) interfaceFinishedDownloading
{
    [self endUpdateProgressIndicatorTimer];
    NSProgressIndicator* progressIndicator = [self getProgressView];
    progressIndicator.doubleValue = self.progressTarget;
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    if ([sharedLauncher currentProccessState] == DOWNLOADING_INTERFACE) {
        [sharedLauncher setCurrentProcessState: RUNNING_INTERFACE_AFTER_DOWNLOAD];
    } else {
        [sharedLauncher setCurrentProcessState: RUNNING_INTERFACE_AFTER_UPDATE];
    }

    [NSTimer scheduledTimerWithTimeInterval: 0.2
                                     target: self
                                   selector: @selector(callLaunchInterface:)
                                   userInfo:nil
                                    repeats: NO];
}

- (void) credentialsEntered:(NSString*)aOrginization :(NSString*)aUsername :(NSString*)aPassword
{
    self.organization = aOrginization;
    self.username = aUsername;
    self.password = aPassword;
    [self.organizationRequest confirmOrganization:aOrginization :aUsername];
}

- (void) organizationRequestFinished:(BOOL)aOriginzationAccepted
{
    self.credentialsAccepted = aOriginzationAccepted;
    if (aOriginzationAccepted) {
        [self updateLatestBuildInfo];
        [self.credentialsRequest confirmCredentials:self.username : self.password];
    } else {
        LoginScreen* loginScreen = [[LoginScreen alloc] initWithNibName:@"LoginScreen" bundle:nil];
        [[[[NSApplication sharedApplication] windows] objectAtIndex:0] setContentViewController: loginScreen];
    }
}

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

-(void) showLoginScreen
{
    LoginScreen* loginScreen = [[LoginScreen alloc] initWithNibName:@"LoginScreen" bundle:nil];
    [[[[NSApplication sharedApplication] windows] objectAtIndex:0] setContentViewController: loginScreen];
}

- (void) shouldDownloadLatestBuild:(NSArray*) latestBuilds :(NSString*) defaultBuildTag :(BOOL) newLauncherAvailable :(NSString*) launcherUrl
{
    self.latestBuilds = [[NSArray alloc] initWithArray:latestBuilds copyItems:true];
    self.defaultBuildTag = defaultBuildTag;

    [self updateLatestBuildInfo];

    NSString *kLauncherUrl = @"LAUNCHER_URL";
    NSString *envLauncherUrl = [[NSProcessInfo processInfo] environment][kLauncherUrl];
    if (envLauncherUrl != nil) {
        NSLog(@"Using launcherUrl from environment: %@ = %@", kLauncherUrl, envLauncherUrl);
        launcherUrl = envLauncherUrl;
    }

    NSDictionary* launcherArguments = [LauncherCommandlineArgs arguments];
    if (newLauncherAvailable && ![launcherArguments valueForKey: @"--noUpdate"]) {
        [self.downloadLauncher downloadLauncher: launcherUrl];
    } else {
        self.latestBuildRequestFinished = TRUE;
        if ([self isLoggedIn]) {
            [self tryDownloadLatestBuild:FALSE];
        } else {
            [[NSApplication sharedApplication] activateIgnoringOtherApps:TRUE];
            [self showLoginScreen];
        }
    }
}

// The latest builds are always retrieved on application start because they contain not only
// the latest interface builds, but also the latest launcher builds, which are required to know if
// we need to self-update first. The interface builds are categorized by build tag, and we may
// not know at application start which build tag we should be using. There are 2 scenarios where
// we call this function to determine our build tag and the correct build:
//
//   1. If we are logged in, we will have our build tag and can immediately get the correct build
//      after receiving the builds.
//   2. If we are not logged in, we need to wait until we have logged in and received the org
//      metadata for the user. The latest build info also needs to be updated _before_ downloading
//      the content set cache because the progress bar value depends on it.
//
- (void) updateLatestBuildInfo {
    NSLog(@"Updating latest build info");

    NSInteger currentVersion = [self getCurrentVersion];
    NSInteger latestVersion = 0;
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    [sharedLauncher setCurrentProcessState:CHECKING_UPDATE];
    BOOL newVersionAvailable = false;
    NSString* url = @"";
    NSString* buildTag = [[Settings sharedSettings] organizationBuildTag];
    if ([buildTag length] == 0) {
        buildTag = self.defaultBuildTag;
    }

    for (NSDictionary* build in self.latestBuilds) {
        NSString* name = [build valueForKey:@"name"];
        NSLog(@"Checking %@", name);
        if ([name isEqual:buildTag]) {
            url = [[[build objectForKey:@"installers"] objectForKey:@"mac"] valueForKey:@"zip_url"];
            NSString* thisLatestVersion = [build valueForKey:@"latest_version"];
            latestVersion = thisLatestVersion.integerValue;
            newVersionAvailable = currentVersion != latestVersion;
            NSLog(@"Using %@, %ld", name, latestVersion);
            break;
        }
    }

    self.shouldDownloadInterface = newVersionAvailable;
    self.interfaceDownloadUrl = url;

    NSLog(@"Updating latest build info, currentVersion=%ld, latestVersion=%ld, %@ %@",
          currentVersion, latestVersion, (self.shouldDownloadInterface ? @"Yes" : @"No"), self.interfaceDownloadUrl);
}

- (void) tryDownloadLatestBuild:(BOOL)progressScreenAlreadyDisplayed
{
    if (self.shouldDownloadInterface) {
        if (!progressScreenAlreadyDisplayed) {
            ProcessScreen* processScreen = [[ProcessScreen alloc] initWithNibName:@"ProcessScreen" bundle:nil];
            [[[[NSApplication sharedApplication] windows] objectAtIndex:0] setContentViewController: processScreen];
            [self startUpdateProgressIndicatorTimer];
        }
        [self.downloadInterface downloadInterface: self.interfaceDownloadUrl];
        return;
    }

    [self interfaceFinishedDownloading];
}

-(void)runAutoupdater
{
    NSException *exception;
    bool launched = false;
    NSString* newLauncher =  [[[Launcher sharedLauncher] getDownloadPathForContentAndScripts] stringByAppendingPathComponent: @"HQ Launcher.app"];

    // Older versions of Launcher put updater in `/Contents/Resources/updater`.
    for (NSString *bundlePath in @[@"/Contents/MacOS/updater",
                                   @"/Contents/Resources/updater",
                                   ]) {
        NSTask* task = [[NSTask alloc] init];
        task.launchPath = [newLauncher stringByAppendingString: bundlePath];
        task.arguments = @[[[NSBundle mainBundle] bundlePath], newLauncher];

        NSLog(@"launching updater: %@ %@", task.launchPath, task.arguments);

        @try {
            [task launch];
        }
        @catch (NSException *e) {
            NSLog(@"couldn't launch updater: %@, %@", e.name, e.reason);
            exception = e;
            continue;
        }

        launched = true;
        break;
    }

    if (!launched) {
        @throw exception;
    }

    [NSApp terminate:self];
}

-(void)onSplashScreenTimerFinished:(NSTimer *)timer
{
    [self.latestBuildRequest requestLatestBuildInfo];
}

-(void)setCurrentProcessState:(ProcessState)aProcessState
{
    self.processState = aProcessState;
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{
    [self.window makeKeyAndOrderFront:self];
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
    // Sanity check the HQDefaults so we fail early if there's an issue.
    HQDefaults *defaults = [HQDefaults sharedDefaults];
    if ([defaults defaultNamed:@"thunderURL"] == nil) {
        @throw [NSException exceptionWithName:@"DefaultsNotConfigured"
                                       reason:@"thunderURL is not configured"
                                     userInfo:nil];
    }
}

- (void) setDownloadFilename:(NSString *)aFilename
{
    self.filename = aFilename;
}

- (NSString*) getDownloadFilename
{
    return self.filename;
}

- (void) setTokenString:(NSString *)aToken
{
    self.userToken = aToken;
}

- (NSString*) getTokenString
{
    return self.userToken;
}

- (void) setLoginErrorState:(LoginError)aLoginError
{
    self.loginError = aLoginError;
}

- (LoginError) getLoginErrorState
{
    return self.loginError;
}

- (void) launchInterface
{
    NSString* launcherPath = [[self getLauncherPath] stringByAppendingString:@"HQ Launcher"];

    [[Settings sharedSettings] setLauncherPath:launcherPath];
    [[Settings sharedSettings] save];
    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSURL *url = [NSURL fileURLWithPath:[workspace fullPathForApplication:[[self getAppPath] stringByAppendingString:@"interface.app/Contents/MacOS/interface"]]];

    NSString* contentPath = [[self getDownloadPathForContentAndScripts] stringByAppendingString:@"content"];
    NSString* displayName = [ self displayName];
    NSString* scriptsPath = [[self getAppPath] stringByAppendingString:@"interface.app/Contents/Resources/scripts/simplifiedUIBootstrapper.js"];
    NSString* domainUrl = [[Settings sharedSettings] getDomainUrl];
    NSString* userToken = [[Launcher sharedLauncher] getTokenString];
    NSString* homeBookmark = [[NSString stringWithFormat:@"hqhome="] stringByAppendingString:domainUrl];
    NSArray* arguments;
    if (userToken != nil) {
        arguments = [NSArray arrayWithObjects:
                        @"--url" , domainUrl ,
                        @"--tokens", userToken,
                        @"--cache", contentPath,
                        @"--displayName", displayName,
                        @"--defaultScriptsOverride", scriptsPath,
                        @"--setBookmark", homeBookmark,
                        @"--no-updater",
                        @"--no-launcher", nil];
    } else {
        arguments = [NSArray arrayWithObjects:
                            @"--url" , domainUrl,
                            @"--cache", contentPath,
                            @"--defaultScriptsOverride", scriptsPath,
                            @"--setBookmark", homeBookmark,
                            @"--no-updater",
                            @"--no-launcher", nil];
    }

    NSTask *task = [[NSTask alloc] init];
    task.launchPath = [url path];
    task.arguments = arguments;
    [task replaceThisProcess];
}

- (ProcessState) currentProccessState
{
    return self.processState;
}

- (void) callLaunchInterface:(NSTimer*) timer
{
    NSWindow* mainWindow = [[[NSApplication sharedApplication] windows] objectAtIndex:0];

    ProcessScreen* processScreen = [[ProcessScreen alloc] initWithNibName:@"ProcessScreen" bundle:nil];
    [mainWindow setContentViewController: processScreen];
    @try
    {
        [self launchInterface];
    }
    @catch (NSException *exception)
    {
        NSLog(@"Caught exception: Name: %@, Reason: %@", exception.name, exception.reason);
        ErrorViewController* errorViewController = [[ErrorViewController alloc] initWithNibName:@"ErrorScreen" bundle:nil];
        [mainWindow setContentViewController: errorViewController];
    }
}

@end
