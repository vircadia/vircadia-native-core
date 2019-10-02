#import "DownloadLauncher.h"
#import "Launcher.h"

#include <sys/stat.h>

static const __int32_t kMinLauncherSize = 250000;  // 308kb is our smallest launcher
static const NSString *kIOError = @"IOError";

@implementation DownloadLauncher

-(id)init {
    if ((self = [super init]) != nil) {
        _didBecomeDownloadTask = false;
    }
    return self;
}

- (void) downloadLauncher:(NSString*)launcherUrl {
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:launcherUrl]
                                                           cachePolicy:NSURLRequestUseProtocolCachePolicy
                                                       timeoutInterval:60.0];
    [request setValue:@USER_AGENT_STRING forHTTPHeaderField:@"User-Agent"];

    NSURLSessionConfiguration *defaultConfigObject = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSURLSession *defaultSession = [NSURLSession sessionWithConfiguration: defaultConfigObject delegate: self delegateQueue: [NSOperationQueue mainQueue]];
    NSURLSessionDataTask *task = [defaultSession dataTaskWithRequest:request];
    [task resume];
}

-(void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didWriteData:(int64_t)bytesWritten totalBytesWritten:(int64_t)totalBytesWritten totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite {
    CGFloat prog = (float)totalBytesWritten/totalBytesExpectedToWrite;
    NSLog(@"Launcher downloaded %f", (100.0*prog));

}

-(void)validateHQLauncherZipAt:(NSURL *)location {
    // Does the file look like a valid HQLauncher ZIP?
    struct stat lStat;
    const char *cStrLocation = location.fileSystemRepresentation;
    if (stat(cStrLocation, &lStat) != 0) {
        NSLog(@"couldn't stat download file: %s", cStrLocation);
        @throw [NSException exceptionWithName:(NSString *)kIOError
                                       reason:@"couldn't stat download file"
                                     userInfo:nil];
    }
    if (lStat.st_size <= kMinLauncherSize) {
        NSLog(@"download is too small: %s is %lld bytes, should be at least %d bytes",
              cStrLocation, lStat.st_size, kMinLauncherSize);
        @throw [NSException exceptionWithName:(NSString *)kIOError
                                       reason:@"download is too small"
                                     userInfo:nil];
    }
}

-(void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask
                                didReceiveResponse:(NSURLResponse *)response
                                 completionHandler:(void (^)(NSURLSessionResponseDisposition))completionHandler
{
    NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
    NSURLSessionResponseDisposition disposition = NSURLSessionResponseBecomeDownload;
    if (httpResponse.statusCode != 200) {
        NSLog(@"expected statusCode 200, got %ld", (long)httpResponse.statusCode);
        disposition = NSURLSessionResponseCancel;
    }
    completionHandler(disposition);
}

- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask
                              didBecomeDownloadTask:(NSURLSessionDownloadTask *)downloadTask
{
    _didBecomeDownloadTask = true;
}

-(void)URLSession:(NSURLSession*)session downloadTask:(NSURLSessionDownloadTask*)downloadTask didFinishDownloadingToURL:(NSURL*)location {
    NSLog(@"Did finish downloading to url");
    @try {
        [self validateHQLauncherZipAt:location];
    }
    @catch (NSException *exc) {
        if ([exc.name isEqualToString:(NSString *)kIOError]) {
            [[Launcher sharedLauncher] displayErrorPage];
            return;
        }
        @throw;
    }

     NSError* error = nil;
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSString* destinationFileName = downloadTask.originalRequest.URL.lastPathComponent;
    NSString* finalFilePath = [[[Launcher sharedLauncher] getDownloadPathForContentAndScripts] stringByAppendingPathComponent:destinationFileName];
    NSURL *destinationURL = [NSURL URLWithString: [finalFilePath stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLFragmentAllowedCharacterSet]] relativeToURL: [NSURL URLWithString:@"file://"]];
    NSLog(@"desintation %@", destinationURL);
    if([fileManager fileExistsAtPath:[destinationURL path]]) {
        [fileManager removeItemAtURL:destinationURL error:nil];
    }

    NSLog(@"location: %@", location.path);
    NSLog(@"destination: %@", destinationURL);
    BOOL success = [fileManager moveItemAtURL:location toURL:destinationURL error:&error];


    NSLog(success ? @"TRUE" : @"FALSE");
    Launcher* sharedLauncher = [Launcher sharedLauncher];

    if (error) {
        NSLog(@"Download Launcher: failed to move file to destintation -> error: %@", error);
        [sharedLauncher displayErrorPage];
        return;
    }
    NSLog(@"extracting Launcher file");
    BOOL extractionSuccessful = [sharedLauncher extractZipFileAtDestination:[sharedLauncher getDownloadPathForContentAndScripts] :[[sharedLauncher getDownloadPathForContentAndScripts] stringByAppendingString:destinationFileName]];

    if (!extractionSuccessful) {
        [sharedLauncher displayErrorPage];
        return;
    }
    NSLog(@"finished extracting Launcher file");


    [[Launcher sharedLauncher] runAutoupdater];
}

- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task didCompleteWithError:(NSError*)error {
    if (error) {
        if (_didBecomeDownloadTask && [task class] == [NSURLSessionDataTask class]) {
            return;
        }
        NSLog(@"couldn't complete download: %@", error);
        [[Launcher sharedLauncher] displayErrorPage];
    } else {
        NSLog(@"finished downloading Launcher");
    }
}
@end
