#import "ErrorViewController.h"
#import "Launcher.h"
#import "CustomUI.h"

@interface ErrorViewController()
@property (nonatomic, assign) IBOutlet NSImageView* backgroundImage;
@property (nonatomic, assign) IBOutlet NSImageView* smallLogo;
@property (nonatomic, assign) IBOutlet NSImageView* voxelImage;
@property (nonatomic, assign) IBOutlet NSTextField* buildVersion;

@end

@implementation ErrorViewController

- (void) awakeFromNib
{
    [self.backgroundImage setImage:[NSImage imageNamed:hifiBackgroundFilename]];
    [self.smallLogo setImage:[NSImage imageNamed:hifiSmallLogoFilename]];
    [self.voxelImage setImage:[NSImage imageNamed:hifiVoxelFilename]];
    [self.buildVersion setStringValue: [@"V." stringByAppendingString:@LAUNCHER_BUILD_VERSION]];
}

-(IBAction)resartLauncher:(id)sender
{
    [[Launcher sharedLauncher] restart];
}

@end
