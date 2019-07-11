//
//  MacHelper.h
//  interface/src
//
//  Created by Howard Stearns
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceLogging.h"
#include "MacHelper.h"

#ifdef Q_OS_MAC
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>
 
io_connect_t  root_port;
IONotificationPortRef notifyPortRef;
io_object_t notifierObject;
void* refCon;

void sleepHandler(void* refCon, io_service_t service, natural_t messageType, void* messageArgument) {
    qCInfo(interfaceapp) << "HRS FIXME sleepHandler.";
    if (messageType == kIOMessageSystemHasPoweredOn) {
        qCInfo(interfaceapp) << "HRS FIXME Waking up from sleep or hybernation.";
    }
}
#endif

MacHelper::MacHelper() {
    qCInfo(interfaceapp) << "HRS FIXME Start MacHelper.";
#ifdef Q_OS_MAC
     root_port = IORegisterForSystemPower(refCon, &notifyPortRef, sleepHandler, &notifierObject);
     if (root_port == 0) {
         qCWarning(interfaceapp) << "IORegisterForSystemPower failed";
     } else {
         qCDebug(interfaceapp) << "HRS FIXME IORegisterForSystemPower OK";
     }
     CFRunLoopAddSource(CFRunLoopGetCurrent(),
                        IONotificationPortGetRunLoopSource(notifyPortRef),
                        kCFRunLoopCommonModes);
#endif
}

MacHelper::~MacHelper() {
    qCInfo(interfaceapp) << "HRS FIXME End MacHelper.";
#ifdef Q_OS_MAC
     CFRunLoopRemoveSource(CFRunLoopGetCurrent(),
                           IONotificationPortGetRunLoopSource(notifyPortRef),
                           kCFRunLoopCommonModes);
     IODeregisterForSystemPower(&notifierObject);
     IOServiceClose(root_port);
     IONotificationPortDestroy(notifyPortRef);
#endif
}
