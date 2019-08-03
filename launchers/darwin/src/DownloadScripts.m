#import "DownloadScripts.h"
#import "Launcher.h"

@implementation DownloadScripts

- (void) downloadScripts:(NSString*) scriptsUrl
{
    /*NSURLRequest* theRequest = [NSURLRequest requestWithURL:[NSURL URLWithString:scriptsUrl]
                                                cachePolicy:NSURLRequestUseProtocolCachePolicy
                                            timeoutInterval:6000.0];
    NSURLDownload  *theDownload = [[NSURLDownload alloc] initWithRequest:theRequest delegate:self];
    
    if (!theDownload) {
        NSLog(@"Download Failed");
    }*/
}

- (void)download:(NSURLDownload *)download decideDestinationWithSuggestedFilename:(NSString *)filename
{
    NSString* finalFilePath = [[[Launcher sharedLauncher] getDownloadPathForContentAndScripts] stringByAppendingPathComponent:filename];
    [download setDestination:finalFilePath allowOverwrite:YES];
    
    [[Launcher sharedLauncher] setDownloadScriptsFilename:filename];
}

- (void)downloadDidFinish:(NSURLDownload*)download
{
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    [sharedLauncher extractZipFileAtDestination:[[sharedLauncher getDownloadPathForContentAndScripts] stringByAppendingString:@"scripts"] :[[sharedLauncher getDownloadPathForContentAndScripts] stringByAppendingString:[sharedLauncher getDownloadScriptsFilename]]];
    
    [sharedLauncher domainScriptsDownloadFinished];
    
}

- (void)download:(NSURLDownload*)download didReceiveResponse:(NSURLResponse*)response
{
    NSLog(@"DownloadScripts received a response");
}
@end
