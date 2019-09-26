//
//  AppNapDisabler.h
//  interface/src
//
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AppNapDisabler_h
#define hifi_AppNapDisabler_h

#import <objc/objc-runtime.h>

class AppNapDisabler {
public:
    AppNapDisabler();
    ~AppNapDisabler();
private:
    id _activity;
};

#endif // hifi_AppNapDisabler_h
