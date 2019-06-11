#import "DownloadInterface.h"
#import "Launcher.h"
#import "Settings.h"

@implementation DownloadInterface

- (void) downloadInterface:(NSString*) downloadUrl
{
    NSURLRequest* request = [NSURLRequest requestWithURL:[NSURL URLWithString:downloadUrl]
                                             cachePolicy:NSURLRequestUseProtocolCachePolicy
                                         timeoutInterval:60.0];
    
    NSURLSessionConfiguration *defaultConfigObject = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSURLSession *defaultSession = [NSURLSession sessionWithConfiguration: defaultConfigObject delegate: self delegateQueue: [NSOperationQueue mainQueue]];
    NSURLSessionDownloadTask *downloadTask = [defaultSession downloadTaskWithRequest:request];
    
    [downloadTask resume];
}

-(void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didWriteData:(int64_t)bytesWritten totalBytesWritten:(int64_t)totalBytesWritten totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite {
    CGFloat prog = (float)totalBytesWritten/totalBytesExpectedToWrite;
    NSLog(@"interface downloaded %d%%", (int)(100.0*prog));
    
}

-(void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didResumeAtOffset:(int64_t)fileOffset expectedTotalBytes:(int64_t)expectedTotalBytes {
    // unused in this example
}

-(void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didFinishDownloadingToURL:(NSURL *)location {
    NSLog(@"Did finish downloading to url");
    NSError *error;
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *destinationFileName = downloadTask.originalRequest.URL.lastPathComponent;
    NSString* finalFilePath = [[[Launcher sharedLauncher] getAppPath] stringByAppendingPathComponent:destinationFileName];
    NSURL *destinationURL = [NSURL URLWithString: [finalFilePath stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLFragmentAllowedCharacterSet]] relativeToURL: [NSURL URLWithString:@"file://"]];
    if([fileManager fileExistsAtPath:[destinationURL path]])
    {
        [fileManager removeItemAtURL:destinationURL error:nil];
    }
    [fileManager moveItemAtURL:location toURL:destinationURL error:&error];
    
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    [sharedLauncher setDownloadFilename:destinationFileName];
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

-(void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error {
    NSLog(@"completed; error: %@", error);
    if (error) {
        [[Launcher sharedLauncher] displayErrorPage];
    }
}


@end
