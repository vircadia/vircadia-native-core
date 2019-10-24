#import "DownloadInterface.h"
#import "Launcher.h"
#import "Settings.h"

@implementation DownloadInterface

- (void) downloadInterface:(NSString*) downloadUrl
{
    self.progressPercentage = 0.0;
    self.taskProgressPercentage = 0.0;
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:downloadUrl]
                                                           cachePolicy:NSURLRequestUseProtocolCachePolicy
                                                       timeoutInterval:60.0];
    [request setValue:@USER_AGENT_STRING forHTTPHeaderField:@"User-Agent"];

    NSURLSessionConfiguration *defaultConfigObject = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSURLSession *defaultSession = [NSURLSession sessionWithConfiguration: defaultConfigObject delegate: self delegateQueue: [NSOperationQueue mainQueue]];
    NSURLSessionDownloadTask *downloadTask = [defaultSession downloadTaskWithRequest:request];

    [downloadTask resume];
}

-(void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didWriteData:(int64_t)bytesWritten totalBytesWritten:(int64_t)totalBytesWritten totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite {
    CGFloat prog = (float)totalBytesWritten/totalBytesExpectedToWrite;
    
    if ((int)(100.0 * prog) != (int)self.progressPercentage) {
        NSLog(@"interface downloaded %d%%", (int)(100.0*prog));
    }

    self.progressPercentage = (100.0 * prog);
    [[Launcher sharedLauncher] updateProgressIndicator];

}

- (double) getProgressPercentage
{
    return (self.progressPercentage * 0.70) + (self.taskProgressPercentage * 0.30);
}

-(void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didResumeAtOffset:(int64_t)fileOffset expectedTotalBytes:(int64_t)expectedTotalBytes {
    // unused in this example
}

-(void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didFinishDownloadingToURL:(NSURL *)location {
    NSLog(@"Did finish downloading to url");
    NSTimer* timer = [NSTimer scheduledTimerWithTimeInterval: 0.1
                                                      target: self
                                                    selector: @selector(updateTaskPercentage:)
                                                    userInfo:nil
                                                     repeats: YES];
    NSError *error = nil;
    NSFileManager *fileManager = [NSFileManager defaultManager];
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    NSString* appPath = [sharedLauncher getAppPath];
    NSString *destinationFileName = downloadTask.originalRequest.URL.lastPathComponent;
    NSString* finalFilePath = [[[Launcher sharedLauncher] getAppPath] stringByAppendingPathComponent:destinationFileName];
    NSURL *destinationURL = [NSURL URLWithString: [finalFilePath stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLFragmentAllowedCharacterSet]] relativeToURL: [NSURL URLWithString:@"file://"]];
    if([fileManager fileExistsAtPath:[destinationURL path]])
    {
        [fileManager removeItemAtURL:destinationURL error:nil];
    }
    [fileManager moveItemAtURL:location toURL:destinationURL error:&error];

    NSURL *oldInterfaceURL = [NSURL URLWithString: [[appPath stringByAppendingString:@"interface.app"] stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLFragmentAllowedCharacterSet]] relativeToURL: [NSURL URLWithString:@"file://"]];

    if([fileManager fileExistsAtPath:[oldInterfaceURL path]])
    {
        [fileManager removeItemAtURL:oldInterfaceURL error:nil];
    }

    if (error) {
        NSLog(@"Download Interface: failed to move file to destination -> error: %@", error);
        [timer invalidate];
        [sharedLauncher displayErrorPage];
        return;
    }
    [sharedLauncher setDownloadFilename:destinationFileName];
    NSString* downloadFileName = [sharedLauncher getDownloadFilename];

    NSLog(@"extract interface zip");
    BOOL success = [sharedLauncher extractZipFileAtDestination:appPath :[appPath stringByAppendingString:downloadFileName]];
    if (!success) {
        [timer invalidate];
        [sharedLauncher displayErrorPage];
        return;
    }
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
    [timer invalidate];
    self.taskProgressPercentage = 100.0;
    [sharedLauncher updateProgressIndicator];
    [sharedLauncher interfaceFinishedDownloading];
}

-(void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error {
    if (error) {
        NSLog(@"DownloadInterface: did complete with error -> error: %@", error);
        [[Launcher sharedLauncher] displayErrorPage];
    }
}

- (void) updateTaskPercentage:(NSTimer*) timer {
    if (self.taskProgressPercentage < 100.0) {
        self.taskProgressPercentage += 1.5;

        if (self.taskProgressPercentage > 100.0) {
            self.taskProgressPercentage = 100.0;
        }
    }
    [[Launcher sharedLauncher] updateProgressIndicator];
}


@end
