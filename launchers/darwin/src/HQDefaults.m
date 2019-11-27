//
//  Created by Matt Hardcastle <m@hardcastle.com>
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#import "HQDefaults.h"

@implementation HQDefaults;

// This initializer is for testing purposes. See `init()` for normal use.
-(id)initWithArray:(NSArray *)array environment:(NSDictionary<NSString *, NSString *> *)environment
{
    self = [super init];
    if (self) {
        NSMutableDictionary<NSString *, NSString *> *__defaults = [[NSMutableDictionary alloc] init];
        NSString *name, *defaultValue, *environmentVariable;

        for (NSDictionary *obj in array) {
            NSMutableArray<NSString *> *missingKeys = [[NSMutableArray alloc] init];
            if ((name = [obj objectForKey:@"name"]) == nil) {
                [missingKeys addObject:@"name"];
            }
            if ((defaultValue = [obj objectForKey:@"defaultValue"]) == nil) {
                [missingKeys addObject:@"defaultValue"];
            }
            if ([missingKeys count] > 0) {
                @throw [NSException exceptionWithName:@"InvalidHQDefaults"
                                               reason:@"A required key is missing"
                                             userInfo:@{@"missingKeys": missingKeys}];
            }

            environmentVariable = [obj objectForKey:@"environmentVariable"];
            if (environmentVariable == nil) {
                __defaults[name] = defaultValue;
                continue;
            }

            NSString *value = environment[environmentVariable];
            __defaults[name] = value == nil ? defaultValue : value;
        }

        // Make the dictionary immutable.
        _defaults = __defaults;
    }
    return self;
}

// Initialize an `HQLauncher` object using the bundles "HQDefaults.plist" and the current process's environment.
-(id)init {
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *defaultsPath = [bundle pathForResource:@"HQDefaults" ofType:@"plist"];
    NSArray *array = [NSArray arrayWithContentsOfFile:defaultsPath];
    return [self initWithArray:array environment:NSProcessInfo.processInfo.environment];
}

// Retrieve a default.
-(NSString *)defaultNamed:(NSString *)name {
    return _defaults[name];
}

// A singleton HQDefaults using the mainBundle's "HQDefaults.plist" and the environment.
+(HQDefaults *)sharedDefaults {
    static HQDefaults *defaults = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        defaults = [[HQDefaults alloc] init];
    });
    return defaults;
}

@end
