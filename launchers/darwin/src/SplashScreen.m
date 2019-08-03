#import "SplashScreen.h"
#import "Launcher.h"
#import "CustomUI.h"

@interface SplashScreen ()
@property (nonatomic, assign) IBOutlet NSImageView* imageView;
@property (nonatomic, assign) IBOutlet NSImageView* logoImage;
@property (nonatomic, assign) IBOutlet NSButton* button;
@property (nonatomic, assign) IBOutlet NSTextField* buildVersion;
@end

@implementation SplashScreen
- (void) viewDidLoad {
}

-(void)awakeFromNib {
    [self.imageView setImage:[NSImage imageNamed:hifiBackgroundFilename]];
    [self.logoImage setImage:[NSImage imageNamed:hifiLargeLogoFilename]];
    [self.buildVersion setStringValue: [@"V." stringByAppendingString:@LAUNCHER_BUILD_VERSION]];
}
@end
