#import "LatestBuildRequest.h"
#import "Launcher.h"
#import "Settings.h"
#import "HQDefaults.h"

@implementation LatestBuildRequest

- (void) requestLatestBuildInfo {
    NSString* buildsURL = [[[NSProcessInfo processInfo] environment] objectForKey:@"HQ_LAUNCHER_BUILDS_URL"];

    if ([buildsURL length] == 0) {
        NSString *thunderURL = [[HQDefaults sharedDefaults] defaultNamed:@"thunderURL"];
        if (thunderURL == nil) {
            @throw [NSException exceptionWithName:@"DefaultMissing"
                                           reason:@"The thunderURL default is missing"
                                         userInfo:nil];
        }
        buildsURL = [NSString stringWithFormat:@"%@/builds/api/tags/latest?format=json", thunderURL];
    }

    NSLog(@"Making request for builds to: %@", buildsURL);

    NSMutableURLRequest* request = [NSMutableURLRequest new];
    [request setURL:[NSURL URLWithString:buildsURL]];
    [request setHTTPMethod:@"GET"];
    [request setValue:@USER_AGENT_STRING forHTTPHeaderField:@"User-Agent"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];

    request.cachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
    NSURLSessionDataTask* dataTask = [NSURLSession.sharedSession dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        NSLog(@"Latest Build Request error: %@", error);
        NSLog(@"Latest Build Request Data: %@", data);
         NSHTTPURLResponse* ne = (NSHTTPURLResponse *)response;
        NSLog(@"Latest Build Request Response: %ld", [ne statusCode]);
        Launcher* sharedLauncher = [Launcher sharedLauncher];

        if (error || [ne statusCode] == 500) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [sharedLauncher displayErrorPage];
            });
            return;
        }
        NSMutableData* webData = [NSMutableData data];
        [webData appendData:data];
        NSString* jsonString = [[NSString alloc] initWithBytes: [webData mutableBytes] length:[data length] encoding:NSUTF8StringEncoding];
        NSData* jsonData = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
        NSLog(@"Latest Build Request -> json string: %@", jsonString);
        NSError* jsonError = nil;
        id json = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:&jsonError];

        if (jsonError) {
            NSLog(@"Latest Build request: Failed to convert Json to data");
        }

        NSFileManager* fileManager = [NSFileManager defaultManager];
        NSArray* values = [json valueForKey:@"results"];
        NSDictionary* launcherValues = [json valueForKey:@"launcher"];

        NSString* defaultBuildTag = [json valueForKey:@"default_tag"];

        NSString* launcherVersion = [launcherValues valueForKey:@"version"];
        NSString* launcherUrl = [[launcherValues valueForKey:@"mac"] valueForKey:@"url"];

        BOOL appDirectoryExist = [fileManager fileExistsAtPath:[[sharedLauncher getAppPath] stringByAppendingString:@"interface.app"]];

        dispatch_async(dispatch_get_main_queue(), ^{
            NSInteger currentLauncherVersion = atoi(LAUNCHER_BUILD_VERSION);
            NSLog(@"Latest Build Request -> current launcher version %ld", currentLauncherVersion);
            NSLog(@"Latest Build Request -> latest launcher version %ld", launcherVersion.integerValue);
            NSLog(@"Latest Build Request -> launcher url %@", launcherUrl);
            NSLog(@"Latest Build Request -> does build directory exist: %@", appDirectoryExist ? @"TRUE" : @"FALSE");
            BOOL latestLauncherVersionAvailable = (currentLauncherVersion != launcherVersion.integerValue);

            [sharedLauncher shouldDownloadLatestBuild:values
                            :defaultBuildTag
                            :latestLauncherVersionAvailable
                            :launcherUrl];
        });
    }];

    [dataTask resume];
}
@end
