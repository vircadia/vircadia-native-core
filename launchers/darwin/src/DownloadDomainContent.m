#import "DownloadDomainContent.h"
#import "Launcher.h"

@implementation DownloadDomainContent

- (double) getProgressPercentage
{
    return (self.progressPercentage * 0.70) + (self.taskProgressPercentage * 0.30);
}

- (void) downloadDomainContent:(NSString *)domainContentUrl
{
    self.progressPercentage = 0.0;
    self.taskProgressPercentage = 0.0;
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:domainContentUrl]
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
    NSLog(@"domain content downloaded %f", (100.0*prog));

    self.progressPercentage = (int)(100.0 * prog);
    [[Launcher sharedLauncher] updateProgressIndicator];

}

-(void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didResumeAtOffset:(int64_t)fileOffset expectedTotalBytes:(int64_t)expectedTotalBytes {
    // unused in this example
}

-(void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didFinishDownloadingToURL:(NSURL *)location {
    NSLog(@"Did finish downloading to url");
    NSTimer* timer = [NSTimer scheduledTimerWithTimeInterval: 0.1
                                                      target: self
                                                    selector: @selector(updatePercentage:)
                                                    userInfo:nil
                                                     repeats: YES];
    NSError *error = nil;
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *destinationFileName = downloadTask.originalRequest.URL.lastPathComponent;
    NSString* finalFilePath = [[[Launcher sharedLauncher] getDownloadPathForContentAndScripts] stringByAppendingPathComponent:destinationFileName];
    NSURL *destinationURL = [NSURL URLWithString: [finalFilePath stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLFragmentAllowedCharacterSet]] relativeToURL: [NSURL URLWithString:@"file://"]];
    if([fileManager fileExistsAtPath:[destinationURL path]])
    {
        [fileManager removeItemAtURL:destinationURL error:nil];
    }

    NSLog(@"location: %@", location.path);
    NSLog(@"destination: %@", destinationURL);
    BOOL success = [fileManager moveItemAtURL:location toURL:destinationURL error:&error];


    NSLog(success ? @"TRUE" : @"FALSE");
    Launcher* sharedLauncher = [Launcher sharedLauncher];

    if (error) {
        NSLog(@"DownlodDomainContent: failed to move file to destintation -> error: %@", error);
        [timer invalidate];
        [sharedLauncher displayErrorPage];
        return;
    }
    [sharedLauncher setDownloadContextFilename:destinationFileName];
    NSLog(@"extracting domain content file");
    BOOL extractionSuccessful = [sharedLauncher extractZipFileAtDestination:[[sharedLauncher getDownloadPathForContentAndScripts] stringByAppendingString:@"content"] :[[sharedLauncher getDownloadPathForContentAndScripts] stringByAppendingString:[sharedLauncher getDownloadContentFilename]]];

    if (!extractionSuccessful) {
        [timer invalidate];
        [sharedLauncher displayErrorPage];
        return;
    }
    NSLog(@"finished extracting content file");
    [timer invalidate];
    self.taskProgressPercentage = 100.0;
    [sharedLauncher updateProgressIndicator];
    [sharedLauncher domainContentDownloadFinished];
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error {
    NSLog(@"completed; error: %@", error);
    if (error) {
        [[Launcher sharedLauncher] displayErrorPage];
    }
}

- (void) updatePercentage:(NSTimer*) timer {
    if (self.taskProgressPercentage < 100.0) {
        self.taskProgressPercentage += 1.5;

        if (self.taskProgressPercentage > 100.0) {
            self.taskProgressPercentage = 100.0;
        }
    }
    [[Launcher sharedLauncher] updateProgressIndicator];
}

@end
