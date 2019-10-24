//
//  Created by Matt Hardcastle <m@hardcastle.com>
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#import <XCTest/XCTest.h>

#import "HQDefaults.h"

// Expose the `initWithArray:environment` initializer for testing.
@interface HQDefaults(HQDefaultsTesting)
-(id)initWithArray:(NSArray *)array environment:(NSDictionary<NSString *, NSString *> *)environment;
@end

@interface HQDefaultsTests : XCTestCase

@end

@implementation HQDefaultsTests

// If there is not environment the default value should be used.
- (void)testNoEnvironmentReturnsDefaultValue {
    NSArray *array = @[@{@"name":@"foo",
                         @"environmentVariable":@"FOO",
                         @"defaultValue":@"bar"}];

    HQDefaults *defaults = [[HQDefaults alloc] initWithArray:array
                                                 environment:@{}];

    XCTAssertEqual([defaults defaultNamed:@"foo"], @"bar");
}

// The value from the environment should overwrite the default value.
- (void)testEnvironmentOverWritesDefaultValue {
    NSArray *array = @[@{@"name":@"foo",
                         @"environmentVariable":@"FOO",
                         @"defaultValue":@"bar"}];
    NSDictionary *environment = @{@"FOO":@"FOO_VALUE_FROM_ENVIRONMENT"};

    HQDefaults *defaults = [[HQDefaults alloc] initWithArray:array environment:environment];

    NSString *value = [defaults defaultNamed:@"foo"];

    XCTAssertEqual(value, @"FOO_VALUE_FROM_ENVIRONMENT");
}

// An exception should be thrown if a defaults object is missing `name`.
- (void)testMissingNameThrowsException {
    NSArray *array = @[@{@"defaultValue":@"bar"}];
    XCTAssertThrowsSpecificNamed([[HQDefaults alloc] initWithArray:array environment:@{}],
                                 NSException,
                                 @"InvalidHQDefaults");
}

// An exception should be thrown if a defaults object is missing `defaultValue`.
- (void)testMissingDefaultValueThrowsException {
    NSArray *array = @[@{@"name":@"foo" }];
    XCTAssertThrowsSpecificNamed([[HQDefaults alloc] initWithArray:array environment:@{}],
                                 NSException,
                                 @"InvalidHQDefaults");
}

// An exception should **NOT** be thrown if a defaults object is missing `environmentVariable`.
- (void)testMissingEnvironmentVariableDoesNotThrowException {
    NSArray *array = @[@{@"name":@"foo", @"defaultValue":@"bar"}];
    XCTAssertNoThrow([[HQDefaults alloc] initWithArray:array environment:@{}]);
}

// A `nil` should be returned if a default is missing.
- (void)testEmptyDefaultIsNil {
    HQDefaults *defaults = [[HQDefaults alloc] initWithArray:@[] environment:@{}];
    XCTAssertNil([defaults defaultNamed:@"foo"]);
}

@end
