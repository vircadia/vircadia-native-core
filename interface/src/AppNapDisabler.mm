//
//  AppNapDisabler.mm
//  interface/src
//
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtGlobal>
#ifdef Q_OS_MAC

#include "AppNapDisabler.h"

#import <AppKit/AppKit.h>

AppNapDisabler::AppNapDisabler() {
    _activity = [[NSProcessInfo processInfo] beginActivityWithOptions:NSActivityUserInitiated reason:@"Audio is in use"];
    [_activity retain];
}

AppNapDisabler::~AppNapDisabler() {
    [[NSProcessInfo processInfo] endActivity:_activity];
    [_activity release];
}

#endif // Q_OS_MAC
