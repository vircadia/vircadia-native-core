#import <Cocoa/Cocoa.h>
#import <Quartz/Quartz.h>
#import <CoreGraphics/CoreGraphics.h>

extern NSString* hifiSmallLogoFilename;
extern NSString* hifiLargeLogoFilename;
extern NSString* hifiVoxelFilename;
extern NSString* hifiBackgroundFilename;

@interface TextField : NSTextField {
}
@end


@interface SecureTextField : NSSecureTextField {
}

@end


@interface HFButton : NSButton

    @property (nonatomic, strong) CATextLayer *titleLayer;

@end

@interface Hyperlink : NSTextField {

}
@end
