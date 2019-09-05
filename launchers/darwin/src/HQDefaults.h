//
//  Created by Matt Hardcastle <m@hardcastle.com>
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/*
 * `HQDefaults` loads defaults from the `HQDefaults.plist` and allows
 * defaults in that plist to be overwritten using an environment variable.
 *
 * For example, the following `HQDefaults.plist` will set a default "foo"
 * with a value "bar". The "bar" value can be overwritten with the value of
 * the FOO environment variables, if the FOO environment variable is set.
 *
 * <?xml version="1.0" encoding="UTF-8"?>
 * <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
 * <plist version="1.0">
 *     <array>
 *         <dict>
 *             <key>name</key>
 *             <string>foo</string>
 *             <key>defaultValue</key>
 *             <string>bar</string>
 *             <key>environmentVariable</key>
 *             <string>FOO</string>
 *         </dict>
 *     </array>
 * </plist>
 */

@interface HQDefaults : NSObject

-(NSString *)defaultNamed:(NSString *)name;
+(HQDefaults *)sharedDefaults;

@property (strong, readonly) NSDictionary<NSString *, NSString *> *defaults;

@end

NS_ASSUME_NONNULL_END
