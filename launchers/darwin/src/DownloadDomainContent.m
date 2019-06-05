#import "DownloadDomainContent.h"
#import "Launcher.h"

@implementation DownloadDomainContent

- (void) downloadDomainContent:(NSString *)domainContentUrl
{
    NSURLRequest* request = [NSURLRequest requestWithURL:[NSURL URLWithString:domainContentUrl]
                                                cachePolicy:NSURLRequestUseProtocolCachePolicy
                                            timeoutInterval:60.0];
    NSURLDownload* theDownload = [[NSURLDownload alloc] initWithRequest:request delegate:self];
    
    /*NSURLSession *session = [NSURLSession sharedSession];
    NSURLSessionDownloadTask *downloadTask = [session downloadTaskWithRequest:request
                                            completionHandler:
    ^(NSURL *location, NSURLResponse *response,NSError *error) {
        NSString* finalFilePath = [[[Launcher sharedLauncher] getDownloadPathForContentAndScripts] stringByAppendingPathComponent:[response suggestedFilename]];
        [[NSFileManager defaultManager] moveItemAtURL:location
                                                toURL:[NSURL URLWithString: [finalFilePath stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLFragmentAllowedCharacterSet]]]
                                                error:nil];
        Launcher* sharedLauncher = [Launcher sharedLauncher];
        [sharedLauncher setDownloadContextFilename:[response suggestedFilename]];
        NSLog(@"extracting domain content file");
        [sharedLauncher extractZipFileAtDestination:[[sharedLauncher getDownloadPathForContentAndScripts] stringByAppendingString:@"content"] :[[sharedLauncher getDownloadPathForContentAndScripts] stringByAppendingString:[sharedLauncher getDownloadContentFilename]]];
        
        NSLog(@"finished extracting content file");
        dispatch_async(dispatch_get_main_queue(), ^{
            [sharedLauncher domainContentDownloadFinished];
        });
    
    }];*/
    
    //[downloadTask resume];
    
   if (!theDownload) {
        NSLog(@"Download Failed");
        [[Launcher sharedLauncher] displayErrorPage];
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
    NSLog(@"download domain content: data received");
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
    NSLog(@"download domain content - Conn Err: %@", [error localizedDescription]);
    [[Launcher sharedLauncher] displayErrorPage];
}

- (void)download:(NSURLDownload *)download decideDestinationWithSuggestedFilename:(NSString *)filename
{
    NSString* finalFilePath = [[[Launcher sharedLauncher] getDownloadPathForContentAndScripts] stringByAppendingPathComponent:filename];
    [download setDestination:finalFilePath allowOverwrite:YES];
    
    [[Launcher sharedLauncher] setDownloadContextFilename:filename];
}

- (void)downloadDidFinish:(NSURLDownload*)download
{
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    NSLog(@"extracting domain content file");
    [sharedLauncher extractZipFileAtDestination:[[sharedLauncher getDownloadPathForContentAndScripts] stringByAppendingString:@"content"] :[[sharedLauncher getDownloadPathForContentAndScripts] stringByAppendingString:[sharedLauncher getDownloadContentFilename]]];
    
    NSLog(@"finished extracting content file");
    
    [sharedLauncher domainContentDownloadFinished];
}

- (void)download:(NSURLDownload*)download didReceiveResponse:(NSURLResponse*)response
{
    NSLog(@"Download content set response");
}

@end
