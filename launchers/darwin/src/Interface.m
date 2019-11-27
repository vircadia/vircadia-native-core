#import "Interface.h"

@implementation Interface
{
    NSString *pathTo;
}

-(id) initWith:(NSString*)aPathToInterface
{
    [self init];
    self->pathTo = [NSString stringWithFormat:@"%@/Contents/MacOS/interface", aPathToInterface];
    return self;
}

-(NSInteger) getVersion:(out NSError * _Nullable *) outError
{
    NSTask * interface = [[NSTask alloc] init];
    NSPipe * standardOut = [NSPipe pipe];

    interface.launchPath = self->pathTo;
    interface.arguments = @[ @"--version" ];
    interface.standardOutput = standardOut;

    NSLog(@"calling interface at %@", self->pathTo);

    NSError *error = nil;
    [interface launch];
    [interface waitUntilExit];
    if (0 != [interface terminationStatus]) {
        *outError = [NSError errorWithDomain:@"interface"
                                        code:-1
                                    userInfo:@{NSUnderlyingErrorKey: error}];
        return 0;
    }

    NSFileHandle* fh = [standardOut fileHandleForReading];
    NSData* data = [fh readDataToEndOfFile];

    NSString* output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    if (output == nil || [output length] == 0) {
        NSDictionary * userInfo = @{
            NSLocalizedDescriptionKey: NSLocalizedString(@"Couldn't start interface", nil)
            };
        *outError = [NSError errorWithDomain:@"interface"
                                        code:-1
                                    userInfo:userInfo];
        return 0;
    }

    // Interface returns the build version as a string like this:
    // "Interface 33333-DEADBEEF". This code grabs the substring
    // between "Interface " and the hyphon ("-")
    NSRange start = [output rangeOfString:@"Interface "];
    if (start.length == 0) {
        NSDictionary * userInfo = @{
            NSLocalizedDescriptionKey: NSLocalizedString(@"Couldn't read interface's version", nil)
            };
        *outError = [NSError errorWithDomain:@"interface"
                                        code:-2
                                    userInfo:userInfo];
        return 0;
    }
    NSRange end = [output rangeOfString:@"-"];
    if (end.length == 0) {
        NSDictionary * userInfo = @{
            NSLocalizedDescriptionKey: NSLocalizedString(@"Couldn't read interface's version", nil)
            };
        *outError = [NSError errorWithDomain:@"interface"
                                        code:-2
                                    userInfo:userInfo];
        return 0;
    }
    NSRange subRange = {start.length, end.location - start.length};
    NSString * versionStr;
    @try {
        versionStr = [output substringWithRange:subRange];
    }
    @catch (NSException *) {
        NSDictionary * userInfo = @{
            NSLocalizedDescriptionKey: NSLocalizedString(@"Couldn't read interface's version", nil)
            };
        *outError = [NSError errorWithDomain:@"interface"
                                        code:-2
                                    userInfo:userInfo];
        return 0;
    }

    return versionStr.integerValue;
}

@end
