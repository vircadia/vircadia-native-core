#import "LoginScreen.h"
#import "Launcher.h"
#import "CustomUI.h"

@interface LoginScreen ()
@property (nonatomic, assign) IBOutlet NSImageView* backgroundImage;
@property (nonatomic, assign) IBOutlet NSImageView* smallLogo;
@property (nonatomic, assign) IBOutlet NSTextField* username;
@property (nonatomic, assign) IBOutlet NSTextField* password;
@property (nonatomic, assign) IBOutlet NSTextField* orginization;
@property (nonatomic, assign) IBOutlet NSTextField* header;
@property (nonatomic, assign) IBOutlet NSTextField* smallHeader;
@property (nonatomic, assign) IBOutlet NSTextField* trouble;
@property (nonatomic, assign) IBOutlet NSButton* button;
@property (nonatomic, assign) IBOutlet NSTextField* buildVersion;
@end

@implementation LoginScreen

- (void) awakeFromNib
{
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    if ([sharedLauncher loginShouldSetErrorState]) {
        [self.header setStringValue:@"Uh-oh, we have a problem"];
        switch ([sharedLauncher getLoginErrorState]) {
            case CREDENTIALS:
                [self.smallHeader setStringValue:@"There is a problem with your credentials, please try again"];
                break;
            case ORGANIZATION:
                [self.smallHeader setStringValue:@"There is a problem with your organization name, please try again"];
                break;
            case NONE:
                break;
                default:
                break;
        }
        [self.button setTitle:@"TRY AGAIN"];
    }

    [self.buildVersion setStringValue: [@"V." stringByAppendingString:@LAUNCHER_BUILD_VERSION]];
    [self.backgroundImage setImage:[NSImage imageNamed:hifiBackgroundFilename]];
    [self.smallLogo setImage:[NSImage imageNamed:hifiSmallLogoFilename]];

    NSMutableAttributedString* usernameString = [[NSMutableAttributedString alloc] initWithString:@"Username"];

    [usernameString addAttribute:NSForegroundColorAttributeName value:[NSColor grayColor] range:NSMakeRange(0,8)];
    [usernameString addAttribute:NSFontAttributeName value:[NSFont systemFontOfSize:18] range:NSMakeRange(0,8)];

    NSMutableAttributedString* orgName = [[NSMutableAttributedString alloc] initWithString:@"Organization Name"];
    [orgName addAttribute:NSForegroundColorAttributeName value:[NSColor grayColor] range:NSMakeRange(0,17)];
    [orgName addAttribute:NSFontAttributeName value:[NSFont systemFontOfSize:18] range:NSMakeRange(0,17)];

    NSMutableAttributedString* passwordString = [[NSMutableAttributedString alloc] initWithString:@"Password"];

    [passwordString addAttribute:NSForegroundColorAttributeName value:[NSColor grayColor] range:NSMakeRange(0,8)];
    [passwordString addAttribute:NSFontAttributeName value:[NSFont systemFontOfSize:18] range:NSMakeRange(0,8)];

    [self.username setPlaceholderAttributedString:usernameString];
    [self.orginization setPlaceholderAttributedString:orgName];
    [self.password setPlaceholderAttributedString:passwordString];

    [self.password setTarget:self];
    [self.password setAction:@selector(goToLogin:)];
}

- (IBAction)goToLogin:(id)sender
{
    printf("In gotologin");
    Launcher* sharedLauncher = [Launcher sharedLauncher];
    [sharedLauncher credentialsEntered:[self.orginization stringValue] :[self.username stringValue] :[self.password stringValue]];
}

- (IBAction)havingTrouble:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://www.highfidelity.com/hq-support"]];
}

@end
