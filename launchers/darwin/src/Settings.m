#import "Settings.h"
#import "Launcher.h"


@implementation Settings

+ (id) sharedSettings
{
    static Settings* sharedSettings = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedSettings = [[self alloc]init];
    });
    return sharedSettings;
}

- (NSString*) getOldFilePath {
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    NSString* appPath = [sharedLauncher getAppPath];
    NSString* filePath = [appPath stringByAppendingString:@"interface.app/Contents/MacOS/"];
    return filePath;
}

- (NSString*) getFilePath
{
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    NSString* appPath = [sharedLauncher getAppPath];
    return appPath;
}

- (void) readDataFromJsonFile
{
    NSString* oldPath = [self getOldFilePath];
    NSString* fileAtOldPath = [oldPath stringByAppendingString:@"config.json"];
    NSString* filePath = [self getFilePath];
    NSString* fileAtPath = [filePath stringByAppendingString:@"config.json"];

    if ([[NSFileManager defaultManager] fileExistsAtPath:fileAtOldPath] && ![[NSFileManager defaultManager] fileExistsAtPath:fileAtPath]) {
        BOOL success = [[NSFileManager defaultManager] moveItemAtPath:fileAtOldPath toPath:fileAtPath error:nil];
        NSLog(@"move config to new location -> status: %@", success ? @"SUCCESS" : @"FAILED");
    }

    if ([[NSFileManager defaultManager] fileExistsAtPath:fileAtPath]) {
        NSString* jsonString = [[NSString alloc] initWithData:[NSData dataWithContentsOfFile:fileAtPath] encoding:NSUTF8StringEncoding];
        NSError * err;
        NSData *data =[jsonString dataUsingEncoding:NSUTF8StringEncoding];
        NSDictionary * json;
        if (data != nil) {
            json = (NSDictionary *)[NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:&err];
            self.loggedIn = [[json valueForKey:@"loggedIn"] boolValue];
            self.build = [[json valueForKey:@"build_version"] integerValue];
            self.launcher = [json valueForKey:@"luancherPath"];
            self.domain = [json valueForKey:@"domain"];
            self.organizationBuildTag = [json valueForKey:@"organizationBuildTag"];
            if ([self.organizationBuildTag length] == 0) {
                self.organizationBuildTag = @"";
            }

            return;
        }
    }
    self.loggedIn = false;
    self.build = 0;
    self.launcher = nil;
    self.domain = nil;
}

- (void) writeDataToFile {
    NSDictionary* json = [[NSDictionary alloc] initWithObjectsAndKeys:
                          [NSString stringWithFormat:@"%ld", self.build], @"build_version",
                          self.loggedIn ? @"TRUE" : @"FALSE", @"loggedIn",
                          self.domain, @"domain",
                          self.launcher, @"launcherPath",
                          self.organizationBuildTag, @"organizationBuildTag",
                          nil];
    NSError * err;
    NSData * jsonData = [NSJSONSerialization  dataWithJSONObject:json options:0 error:&err];
    NSString * jsonString = [[NSString alloc] initWithData:jsonData   encoding:NSUTF8StringEncoding];

    NSString* filePath = [self getFilePath];
    NSString* fileAtPath = [filePath stringByAppendingString:@"config.json"];

    if (![[NSFileManager defaultManager] fileExistsAtPath:fileAtPath]) {
        NSError * error = nil;
        [[NSFileManager defaultManager] createDirectoryAtPath:filePath withIntermediateDirectories:FALSE attributes:nil error:&error];
        [[NSFileManager defaultManager] createFileAtPath:fileAtPath contents:nil attributes:nil];

    }
    [[jsonString dataUsingEncoding:NSUTF8StringEncoding] writeToFile:fileAtPath atomically:NO];

}

-(id)init
{
    if (self = [super init]) {
        [self readDataFromJsonFile];
    }
    return self;
}

- (BOOL) isLoggedIn
{
    return self.loggedIn;
}

- (NSInteger) latestBuildVersion
{
    return self.build;
}

- (void) buildVersion:(NSInteger)aBuildVersion
{
    self.build = aBuildVersion;
}

- (void) login:(BOOL)aLoggedIn
{
    self.loggedIn = aLoggedIn;
}

- (void) setLauncherPath:(NSString *)aLauncherPath
{
    self.launcher = aLauncherPath;
}

- (void) setDomainUrl:(NSString *)aDomainUrl
{
    self.domain = aDomainUrl;
}

- (NSString*) getDomainUrl
{
    return self.domain;
}

- (void) setOrganizationBuildTag:(NSString*) aOrganizationBuildTag
{
    self._organizationBuildTag = aOrganizationBuildTag;
}

- (NSString*) organizationBuildTag
{
    return self._organizationBuildTag;
}

- (NSString*) getLaucnherPath
{
    return self.launcher;
}

- (void) save
{
    [self writeDataToFile];
}

@end
