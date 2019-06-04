#import <Cocoa/Cocoa.h>

@interface DownloadScripts : NSObject <NSURLDownloadDelegate> {
}
@property (nonatomic, assign) NSString* finalFilePath;

- (void) downloadScripts:(NSString*) scriptUrl;
@end
