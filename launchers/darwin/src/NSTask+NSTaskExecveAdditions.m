#import "NSTask+NSTaskExecveAdditions.h"

#import <libgen.h>

char **
toCArray(NSArray<NSString *> *array)
{
    // Add one to count to accommodate the NULL that terminates the array.
    char **cArray = (char **) calloc([array count] + 1, sizeof(char *));
    if (cArray == NULL) {
        NSException *exception = [NSException
                                  exceptionWithName:@"MemoryException"
                                  reason:@"malloc failed"
                                  userInfo:nil];
        @throw exception;
    }
    char *str;
    for (int i = 0; i < [array count]; i++) {
        str = (char *) [array[i] UTF8String];
        if (str == NULL) {
            NSException *exception = [NSException
                                      exceptionWithName:@"NULLStringException"
                                      reason:@"UTF8String was NULL"
                                      userInfo:nil];
            @throw exception;
        }
        if (asprintf(&cArray[i], "%s", str) == -1) {
            for (int j = 0; j < i; j++) {
                free(cArray[j]);
            }
            free(cArray);
            NSException *exception = [NSException
                                      exceptionWithName:@"MemoryException"
                                      reason:@"malloc failed"
                                      userInfo:nil];
            @throw exception;
        }
    }
    return cArray;
}

@implementation NSTask (NSTaskExecveAdditions)

- (void) replaceThisProcess {
    char **args = toCArray([@[[self launchPath]] arrayByAddingObjectsFromArray:[self arguments]]);

    NSMutableArray *env = [[NSMutableArray alloc] init];
    NSDictionary* environvment = [[NSProcessInfo processInfo] environment];
    for (NSString* key in environvment) {
        NSString* environmentVariable = [[key stringByAppendingString:@"="] stringByAppendingString:environvment[key]];
        [env addObject:environmentVariable];
    }

    char** envp = toCArray(env);
    // `execve` replaces the current process with `path`.
    // It will only return if it fails to replace the current process.
    chdir(dirname(args[0]));
    execve(args[0], (char * const *)args, envp);

    // If we're here `execve` failed. :(
    for (int i = 0; i < [[self arguments] count]; i++) {
        free((void *) args[i]);
    }
    free((void *) args);

    NSException *exception = [NSException
                              exceptionWithName:@"ExecveException"
                              reason:[NSString stringWithFormat:@"couldn't execve: %s", strerror(errno)]
                              userInfo:nil];
    @throw exception;
}

@end
