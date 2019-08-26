//
//  Created by Bradley Austin Davis on 2019/08/22
//  Based on interface/src/MacHelper.cpp, created by Howard Stearns
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "../PlatformHelper.h"

#if defined(Q_OS_MAC)
#include <IOKit/IOMessage.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

class MacHelper : public PlatformHelper {
public:
    MacHelper() {
        _rootPort = IORegisterForSystemPower(this, &_notifyPortRef, serviceInterestCallback, &_notifierObject);
        if (_rootPort == 0) {
            qWarning() << "IORegisterForSystemPower failed";
            return;
        }
        CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(_notifyPortRef), kCFRunLoopCommonModes);
    }

    ~MacHelper() {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(_notifyPortRef), kCFRunLoopCommonModes);
        IODeregisterForSystemPower(&_notifierObject);
        IOServiceClose(_rootPort);
        IONotificationPortDestroy(_notifyPortRef);
    }

private:
    void onServiceMessage(io_service_t, natural_t messageType, void* messageArgument) {
        switch (messageType) {
            case kIOMessageSystemHasPoweredOn:
                onWake();
                break;

            case kIOMessageSystemWillSleep:
                onSleep();
                // explicit fallthrough

            // Per the documentation for kIOMessageSystemWillSleep and kIOMessageCanSystemSleep, the receiver MUST respond
            // https://developer.apple.com/documentation/iokit/1557114-ioregisterforsystempower?language=objc
            case kIOMessageCanSystemSleep:
                IOAllowPowerChange(_rootPort, (long)messageArgument);
                break;

            default:
                break;
        }
    }

    static void serviceInterestCallback(void* refCon, io_service_t service, natural_t messageType, void* messageArgument) {
        static_cast<MacHelper*>(refCon)->onServiceMessage(service, messageType, messageArgument);
    }

    io_connect_t _rootPort{ 0 };
    IONotificationPortRef _notifyPortRef{};
    io_object_t _notifierObject{};
};

void PlatformHelper::setup() {
    DependencyManager::set<PlatformHelper, MacHelper>();
}

#endif

