#import <Cocoa/Cocoa.h>

@interface DownloadInterface : NSObject <NSURLDownloadDelegate> {
}
@property (nonatomic, assign) NSString* finalFilePath;

- (void) downloadInterface:(NSString*) downloadUrl;
@end
