#import "DownloadInterface.h"
#import "Launcher.h"
#import "Settings.h"

@implementation DownloadInterface

- (void) downloadInterface:(NSString*) downloadUrl
{
    NSURLRequest* request = [NSURLRequest requestWithURL:[NSURL URLWithString:downloadUrl]
                                                cachePolicy:NSURLRequestUseProtocolCachePolicy
                                            timeoutInterval:60.0];
    
    NSURLDownload* theDownload = [[NSURLDownload alloc] initWithRequest:request delegate:self];
    
    /*NSURLSession *session = [NSURLSession sharedSession];
    NSURLSessionDownloadTask *downloadTask = [session downloadTaskWithRequest:request
                                                            completionHandler:
    ^(NSURL *location, NSURLResponse *response,NSError *error) {
        Launcher* sharedLauncher = [Launcher sharedLauncher];
        NSString* finalFilePath = [[sharedLauncher getAppPath] stringByAppendingPathComponent:[response suggestedFilename]];
        [[Launcher sharedLauncher] setDownloadFilename:[response suggestedFilename]];
        
        NSLog(@"----------->%@", location);
        [[NSFileManager defaultManager] moveItemAtURL:location
                                                toURL:[NSURL URLWithString: [finalFilePath stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLFragmentAllowedCharacterSet]]]
                                                error:nil];
        dispatch_async(dispatch_get_main_queue(), ^{
            
            NSString* appPath = [sharedLauncher getAppPath];
            NSString* downloadFileName = [sharedLauncher getDownloadFilename];
            NSLog(@"!!!!!!%@", downloadFileName);
            NSLog(@"extract interface zip");
            [sharedLauncher extractZipFileAtDestination:appPath :[appPath stringByAppendingString:downloadFileName]];
            NSLog(@"finished extracting interface zip");
            
            NSLog(@"starting xattr");
            NSTask* quaratineTask = [[NSTask alloc] init];
            quaratineTask.launchPath = @"/usr/bin/xattr";
            quaratineTask.arguments = @[@"-d", @"com.apple.quarantine", [appPath stringByAppendingString:@"interface.app"]];
            
            [quaratineTask launch];
            [quaratineTask waitUntilExit];
            NSLog(@"finished xattr");
            
            NSString* launcherPath = [appPath stringByAppendingString:@"Launcher"];
            
            [[Settings sharedSettings] setLauncherPath:launcherPath];
            [sharedLauncher interfaceFinishedDownloading];
            
        });
    }];
    
    [downloadTask resume];*/
}

- (void)download:(NSURLDownload *)download decideDestinationWithSuggestedFilename:(NSString *)filename
{
    self.finalFilePath = [[[Launcher sharedLauncher] getAppPath] stringByAppendingPathComponent:filename];
    [download setDestination:self.finalFilePath allowOverwrite:YES];
    
    [[Launcher sharedLauncher] setDownloadFilename:filename];
}


- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
    NSHTTPURLResponse *ne = (NSHTTPURLResponse *)response;
    if([ne statusCode] == 200) {
        NSLog(@"download interface connection state is 200 - all okay");
    } else {
        NSLog(@"download interface connection state is NOT 200");
        [[Launcher sharedLauncher] displayErrorPage];
    }
}

-(void) connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
    NSLog(@"download interface content - Conn Err: %@", [error localizedDescription]);
    [[Launcher sharedLauncher] displayErrorPage];
}

- (void)downloadDidFinish:(NSURLDownload*)download
{
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    NSString* appPath = [sharedLauncher getAppPath];
    NSString* downloadFileName = [sharedLauncher getDownloadFilename];
    
    NSLog(@"extract interface zip");
    [sharedLauncher extractZipFileAtDestination:appPath :[appPath stringByAppendingString:downloadFileName]];
    NSLog(@"finished extracting interface zip");
    
    NSLog(@"starting xattr");
    NSTask* quaratineTask = [[NSTask alloc] init];
    quaratineTask.launchPath = @"/usr/bin/xattr";
    quaratineTask.arguments = @[@"-d", @"com.apple.quarantine", [appPath stringByAppendingString:@"interface.app"]];
    
    [quaratineTask launch];
    [quaratineTask waitUntilExit];
    NSLog(@"finished xattr");
    
    NSString* launcherPath = [appPath stringByAppendingString:@"Launcher"];
    
    [[Settings sharedSettings] setLauncherPath:launcherPath];
    [sharedLauncher interfaceFinishedDownloading];
}

- (void)download:(NSURLDownload*)download didReceiveResponse:(NSURLResponse*)response
{
    NSLog(@"Download interface response");
}
@end
