#import "DisplayNameScreen.h"
#import "Launcher.h"
#import "CustomUI.h"

@interface DisplayNameScreen ()
@property (nonatomic, assign) IBOutlet NSImageView* backgroundImage;
@property (nonatomic, assign) IBOutlet NSImageView* smallLogo;
@property (nonatomic, assign) IBOutlet NSTextField* displayName;
@end

@implementation DisplayNameScreen
- (void) awakeFromNib {
    [self.backgroundImage setImage: [NSImage imageNamed:hifiBackgroundFilename]];
    [self.smallLogo setImage: [NSImage imageNamed:hifiSmallLogoFilename]];
    NSMutableAttributedString* displayNameString = [[NSMutableAttributedString alloc] initWithString:@"Display Name"];
    
    [displayNameString addAttribute:NSForegroundColorAttributeName value:[NSColor grayColor] range:NSMakeRange(0, displayNameString.length)];
    [displayNameString addAttribute:NSFontAttributeName value:[NSFont systemFontOfSize:18] range:NSMakeRange(0,displayNameString.length)];
    
    [self.displayName setPlaceholderAttributedString:displayNameString];
    [self.displayName setTarget:self];
    [self.displayName setAction:@selector(login:)];
}

- (IBAction)login:(id)sender {
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    [sharedLauncher displayNameEntered: [self.displayName stringValue]];
}

- (IBAction)hyperLink:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://www.highfidelity.com/hq-support"]];
}
@end
