#import "CustomUI.h"

#import <QuartzCore/QuartzCore.h>

NSString* hifiSmallLogoFilename = @"hifi_logo_small";
NSString* hifiLargeLogoFilename = @"hifi_logo_large";
NSString* hifiVoxelFilename = @"HiFi_Voxel";
NSString* hifiBackgroundFilename = @"hifi_window";

@implementation TextField
- (void) textDidBeginEditing:(NSNotification *)notification
{
    NSColor *insertionPointColor = [NSColor whiteColor];
    NSTextView *fieldEditor = (NSTextView*)[self.window fieldEditor:YES
                                                          forObject:self];
    fieldEditor.insertionPointColor = insertionPointColor;
}

- (BOOL) performKeyEquivalent:(NSEvent *)event
{
    if ([event type] == NSEventTypeKeyDown) {
        if ([event modifierFlags] & NSEventModifierFlagCommand) {
            if ([[event charactersIgnoringModifiers] isEqualToString:@"v"]) {
                [NSApp sendAction:(NSSelectorFromString(@"paste:")) to:nil from:self];
                return TRUE;
            }

            if ([[event charactersIgnoringModifiers] isEqualToString:@"c"]) {
                [NSApp sendAction:(NSSelectorFromString(@"copy:")) to:nil from:self];
                return TRUE;
            }

            if ([[event charactersIgnoringModifiers] isEqualToString:@"a"]) {
                [NSApp sendAction:(NSSelectorFromString(@"selectAll:")) to:nil from:self];
                return TRUE;
            }
        }
    }

    return [super performKeyEquivalent:event];
}

- (void) mouseDown:(NSEvent *)event
{
    NSColor *insertionPointColor = [NSColor whiteColor];
    NSTextView *fieldEditor = (NSTextView*)[self.window fieldEditor:YES
                                                             forObject:self];
    fieldEditor.insertionPointColor = insertionPointColor;

}

-(BOOL)becomeFirstResponder
{
    BOOL status = [super becomeFirstResponder];
    NSColor *insertionPointColor = [NSColor whiteColor];
    NSTextView *fieldEditor = (NSTextView*)[self.window fieldEditor:YES
                                                          forObject:self];
    fieldEditor.insertionPointColor = insertionPointColor;
    return status;
}
@end

@implementation SecureTextField

- (void) textDidBeginEditing:(NSNotification *)notification
{
    NSColor *insertionPointColor = [NSColor whiteColor];
    NSTextView *fieldEditor = (NSTextView*)[self.window fieldEditor:YES
                                                          forObject:self];
    fieldEditor.insertionPointColor = insertionPointColor;
}
- (void) mouseDown:(NSEvent *)event
{
    NSColor *insertionPointColor = [NSColor whiteColor];
    NSTextView *fieldEditor = (NSTextView*)[self.window fieldEditor:YES
                                                          forObject:self];
    fieldEditor.insertionPointColor = insertionPointColor;

}


-(BOOL)becomeFirstResponder
{
    BOOL status = [super becomeFirstResponder];
    NSColor *insertionPointColor = [NSColor whiteColor];
    NSTextView *fieldEditor = (NSTextView*)[self.window fieldEditor:YES
                                                          forObject:self];
    fieldEditor.insertionPointColor = insertionPointColor;
    return status;
}

- (BOOL) performKeyEquivalent:(NSEvent *)event
{
    if ([event type] == NSEventTypeKeyDown) {
        if ([event modifierFlags] & NSEventModifierFlagCommand) {
            if ([[event charactersIgnoringModifiers] isEqualToString:@"v"]) {
                [NSApp sendAction:(NSSelectorFromString(@"paste:")) to:nil from:self];
                return TRUE;
            }

            if ([[event charactersIgnoringModifiers] isEqualToString:@"c"]) {
                [NSApp sendAction:(NSSelectorFromString(@"copy:")) to:nil from:self];
                return TRUE;
            }

            if ([[event charactersIgnoringModifiers] isEqualToString:@"a"]) {
                [NSApp sendAction:(NSSelectorFromString(@"selectAll:")) to:nil from:self];
                return TRUE;
            }
        }
    }

    return [super performKeyEquivalent:event];
}
@end


@implementation HFButton
- (void)drawRect:(NSRect)dirtyRect {
    // The UI layers implemented in awakeFromNib will handle drawing
}

- (BOOL)layer:(CALayer *)layer shouldInheritContentsScale:(CGFloat)newScale fromWindow:(NSWindow *)window {
    return YES;
}

- (void)awakeFromNib {
    [super awakeFromNib];

    self.wantsLayer = YES;
    self.layer.backgroundColor = [NSColor blackColor].CGColor;
    self.layer.borderColor = [NSColor whiteColor].CGColor;
    self.layer.borderWidth = 2.0f;
    self.layer.masksToBounds = YES;

    _titleLayer = [[CATextLayer alloc] init];

    CGSize buttonSize = self.frame.size;
    CGSize titleSize = [self.title sizeWithAttributes:@{NSFontAttributeName: self.font}];
    CGFloat x = (buttonSize.width - titleSize.width) / 2.0; // Title's origin x
    CGFloat y = (buttonSize.height - titleSize.height) / 2.0; // Title's origin y

    self.titleLayer.frame = NSMakeRect(round(x), round(y), ceil(titleSize.width), ceil(titleSize.height));
    self.titleLayer.string = self.title;
    self.titleLayer.foregroundColor = [NSColor whiteColor].CGColor;

    // TODO(huffman) Fix this to be dynamic based on screen?
    self.titleLayer.contentsScale = 2.0;

    self.titleLayer.font = (__bridge CFTypeRef _Nullable)(self.font);
    self.titleLayer.fontSize = self.font.pointSize;
    //self.titleLayer.allowsEdgeAntialiasing = YES;
    //self.titleLayer.allowsFontSubpixelQuantization = YES;

    [self.layer addSublayer:self.titleLayer];
}

@end

@implementation Hyperlink

- (void) mouseDown:(NSEvent *)event
{
    [self sendAction:[self action] to:[self target]];
}

@end
