#import "LatestBuildRequest.h"
#import "Launcher.h"
#import "Settings.h"
#import "Interface.h"

@implementation LatestBuildRequest

- (NSInteger) getCurrentVersion {
    NSInteger currentVersion;
    @try {
        NSString* interfaceAppPath = [[Launcher.sharedLauncher getAppPath] stringByAppendingString:@"interface.app"];
        NSError * error = nil;
        Interface * interface = [[Interface alloc] initWith:interfaceAppPath];
        currentVersion = [interface getVersion:&error];
        if (currentVersion == 0 && error != nil) {
            NSLog(@"can't get version from interface, falling back to settings: %@", error);
            currentVersion = [Settings.sharedSettings latestBuildVersion];
        }
    } @catch (NSException *exception) {
        NSLog(@"an exception was thrown");
        currentVersion = [Settings.sharedSettings latestBuildVersion];
    }
    return currentVersion;
}

- (void) requestLatestBuildInfo {
    NSMutableURLRequest *request = [NSMutableURLRequest new];
    [request setURL:[NSURL URLWithString:@"https://thunder.highfidelity.com/builds/api/tags/latest?format=json"]];
    [request setHTTPMethod:@"GET"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    
    // We're using an ephermeral session here to ensure the tags api response is never cached.
    NSURLSession * session = [NSURLSession sessionWithConfiguration:NSURLSessionConfiguration.ephemeralSessionConfiguration];
    NSURLSessionDataTask* dataTask = [session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        
        
        NSLog(@"Latest Build Request error: %@", error);
        NSLog(@"Latest Build Request Data: %@", data);
         NSHTTPURLResponse *ne = (NSHTTPURLResponse *)response;
        NSLog(@"Latest Build Request Response: %ld", [ne statusCode]);
        Launcher* sharedLauncher = [Launcher sharedLauncher];
        NSMutableData* webData = [NSMutableData data];
        [webData appendData:data];
        NSString* jsonString = [[NSString alloc] initWithBytes: [webData mutableBytes] length:[data length] encoding:NSUTF8StringEncoding];
        NSData *jsonData = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
        NSLog(@"Latest Build Request -> json string: %@", jsonString);
        NSError *jsonError = nil;
        id json = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:&jsonError];
        
        if (jsonError) {
            NSLog(@"Latest Build request: Failed to convert Json to data");
        }
        
        NSFileManager* fileManager = [NSFileManager defaultManager];
        NSArray *values = [json valueForKey:@"results"];
        NSDictionary *value  = [values objectAtIndex:0];
        
        
        NSString* buildNumber = [value valueForKey:@"latest_version"];
        NSDictionary* installers = [value objectForKey:@"installers"];
        NSDictionary* macInstallerObject = [installers objectForKey:@"mac"];
        NSString* macInstallerUrl = [macInstallerObject valueForKey:@"zip_url"];
        
        BOOL appDirectoryExist = [fileManager fileExistsAtPath:[[sharedLauncher getAppPath] stringByAppendingString:@"interface.app"]];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            
            NSInteger currentVersion = [self getCurrentVersion];
            NSLog(@"Latest Build Request -> does build directory exist: %@", appDirectoryExist ? @"TRUE" : @"FALSE");
            NSLog(@"Latest Build Request -> current version: %ld", currentVersion);
            NSLog(@"Latest Build Request -> latest version: %ld", buildNumber.integerValue);
            NSLog(@"Latest Build Request -> mac url: %@", macInstallerUrl);
            BOOL latestVersionAvailable = (currentVersion != buildNumber.integerValue);
            [[Settings sharedSettings] buildVersion:buildNumber.integerValue];
        
            BOOL shouldDownloadInterface = (latestVersionAvailable || !appDirectoryExist);
            NSLog(@"Latest Build Request -> SHOULD DOWNLOAD: %@", shouldDownloadInterface ? @"TRUE" : @"FALSE");
            [sharedLauncher shouldDownloadLatestBuild:shouldDownloadInterface :macInstallerUrl];
        });
    }];
    
    [dataTask resume];

    //NSURLConnection *theConnection = [[NSURLConnection alloc] initWithRequest:request delegate:self];

    /*if(theConnection) {
        self.webData = [NSMutableData data];
        NSLog(@"connection initiated");
    }*/
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
    [self.webData appendData:data];
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
    NSHTTPURLResponse *ne = (NSHTTPURLResponse *)response;
    if([ne statusCode] == 200) {
        NSLog(@"connection state is 200 - all okay");
    } else {
        NSLog(@"connection state is NOT 200");
        [[Launcher sharedLauncher] displayErrorPage];
    }
}

-(void) connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
    NSLog(@"Conn Err: %@", [error localizedDescription]);
    [[Launcher sharedLauncher] displayErrorPage];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    self.jsonString = [[NSString alloc] initWithBytes: [self.webData mutableBytes] length:[self.webData length] encoding:NSUTF8StringEncoding];
    NSData *data = [self.jsonString dataUsingEncoding:NSUTF8StringEncoding];
    id json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSArray      *values = [json valueForKey:@"results"];
    NSDictionary *value  = [values objectAtIndex:0];
    
    
    NSString* buildNumber = [value valueForKey:@"latest_version"];
    NSDictionary* installers = [value objectForKey:@"installers"];
    NSDictionary* macInstallerObject = [installers objectForKey:@"mac"];
    NSString* macInstallerUrl = [macInstallerObject valueForKey:@"zip_url"];
    
    NSString* interfaceAppPath = [[sharedLauncher getAppPath] stringByAppendingString:@"interface.app"];
    BOOL appDirectoryExist = [fileManager fileExistsAtPath:interfaceAppPath];

    BOOL latestVersionAvailable = ([self getCurrentVersion] != buildNumber.integerValue);
    [[Settings sharedSettings] buildVersion:buildNumber.integerValue];
    
    BOOL shouldDownloadInterface = (latestVersionAvailable || !appDirectoryExist);
    [sharedLauncher shouldDownloadLatestBuild:shouldDownloadInterface :macInstallerUrl];
}

@end
