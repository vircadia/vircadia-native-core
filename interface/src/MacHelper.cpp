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
#include <NodeList.h>

#ifdef Q_OS_MAC
#include <IOKit/IOMessage.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

// These type definitions come from IOKit, which includes a definition of Duration that conflicts with ours.
// So... we include these definitions here rather than in the .h, as the .h is included in Application.cpp which
// uses Duration.
static io_connect_t  root_port;
static IONotificationPortRef notifyPortRef;
static io_object_t notifierObject;
static void* refCon;

static void sleepHandler(void* refCon, io_service_t service, natural_t messageType, void* messageArgument) {
    if (messageType == kIOMessageSystemHasPoweredOn) {
        qCInfo(interfaceapp) << "Waking up from sleep or hybernation.";
        QMetaObject::invokeMethod(DependencyManager::get<NodeList>().data(), "noteAwakening", Qt::QueuedConnection);
    }
}
#endif

MacHelper::MacHelper() {
#ifdef Q_OS_MAC
     root_port = IORegisterForSystemPower(refCon, &notifyPortRef, sleepHandler, &notifierObject);
     if (root_port == 0) {
         qCWarning(interfaceapp) << "IORegisterForSystemPower failed";
         return;
     }
     CFRunLoopAddSource(CFRunLoopGetCurrent(),
                        IONotificationPortGetRunLoopSource(notifyPortRef),
                        kCFRunLoopCommonModes);
#endif
}

MacHelper::~MacHelper() {
#ifdef Q_OS_MAC
     CFRunLoopRemoveSource(CFRunLoopGetCurrent(),
                           IONotificationPortGetRunLoopSource(notifyPortRef),
                           kCFRunLoopCommonModes);
     IODeregisterForSystemPower(&notifierObject);
     IOServiceClose(root_port);
     IONotificationPortDestroy(notifyPortRef);
#endif
}
