#import "LauncherCommandlineArgs.h"

@implementation LauncherCommandlineArgs
+(NSDictionary*) arguments {

    NSArray* arguments = [[NSProcessInfo processInfo] arguments];
    if (arguments.count < 2)
    {
        return nil;
    }

    NSMutableDictionary* argsDict = [[NSMutableDictionary alloc] init];
    NSMutableArray* args = [arguments mutableCopy];

    for (NSString* arg in args)
    {
        if ([arg rangeOfString:@"="].location != NSNotFound && [arg rangeOfString:@"--"].location != NSNotFound) {
            NSArray* components = [arg componentsSeparatedByString:@"="];
            NSString* key = [components objectAtIndex:0];
            NSString* value = [components objectAtIndex:1];
            [argsDict setObject:value forKey:key];
        } else if ([arg rangeOfString:@"--"].location != NSNotFound) {
            [argsDict setObject:@TRUE forKey:arg];
        }
    }
    NSLog(@"AGS: %@", argsDict);
    return argsDict;
}
@end
