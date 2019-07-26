#import "ProcessScreen.h"
#import "Launcher.h"
#import "CustomUI.h"

@interface ProcessScreen ()
@property (nonatomic, assign) IBOutlet NSImageView* background;
@property (nonatomic, assign) IBOutlet NSImageView* smallLogo;
@property (nonatomic, assign) IBOutlet NSImageView* voxelImage;
@property (nonatomic, assign) IBOutlet NSTextField* boldStatus;
@property (nonatomic, assign) IBOutlet NSTextField* smallStatus;
@property (nonatomic, assign) IBOutlet NSProgressIndicator* progressView;
@property (nonatomic, assign) IBOutlet NSTextField* buildVersion;
@end

@implementation ProcessScreen

- (void) awakeFromNib {
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    switch ([sharedLauncher currentProccessState]) {
        case DOWNLOADING_INTERFACE:
            [self.boldStatus setStringValue:@"We're building your virtual HQ"];
            [self.smallStatus setStringValue:@"Set up may take several minutes."];
            break;
        case RUNNING_INTERFACE_AFTER_DOWNLOAD:
            [self.progressView setHidden: YES];
            [self.boldStatus setStringValue:@"Your new HQ is all setup"];
            [self.smallStatus setStringValue:@"Thanks for being patient."];
            break;
        case CHECKING_UPDATE:
            [self.boldStatus setStringValue:@"Getting updates..."];
            [self.smallStatus setStringValue:@"We're getting the latest and greatest for you, one sec."];
            break;
        case RUNNING_INTERFACE_AFTER_UPDATE:
            [self.progressView setHidden: YES];
            [self.boldStatus setStringValue:@"You're good to go!"];
            [self.smallStatus setStringValue:@"Thanks for being patient."];
            break;
        default:
            break;
    }
    [self.buildVersion setStringValue: [@"V." stringByAppendingString:@LAUNCHER_BUILD_VERSION]];
    [self.background setImage: [NSImage imageNamed:hifiBackgroundFilename]];
    [self.smallLogo setImage: [NSImage imageNamed:hifiSmallLogoFilename]];
    [self.voxelImage setImage: [NSImage imageNamed:hifiVoxelFilename]];
    if (self.progressView != nil) {
        [sharedLauncher setProgressView: self.progressView];
    }

    self.imageRotation = 0;
    [NSTimer scheduledTimerWithTimeInterval:0.016
                                         target:self
                                       selector:@selector(rotateView:)
                                       userInfo:nil
                                        repeats:YES];
}

- (void) rotateView:(NSTimer *)timer{
    self.imageRotation -= 1;
    [self.voxelImage setFrameCenterRotation:self.imageRotation];
}
@end
