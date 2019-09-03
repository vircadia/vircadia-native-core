//
//  Created by Bradley Austin Davis on 2019/08/22
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "../PlatformHelper.h"

#if !defined(Q_OS_ANDROID) && defined(Q_OS_WIN)

#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QAbstractEventDispatcher>
#include <Windows.h>

class WinHelper : public PlatformHelper, public QAbstractNativeEventFilter {
public:
    WinHelper() {
        QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
    }

    ~WinHelper() {
        auto eventDispatcher = QAbstractEventDispatcher::instance();
        if (eventDispatcher) {
            eventDispatcher->removeNativeEventFilter(this);
        }
    }

    bool nativeEventFilter(const QByteArray& eventType, void* message, long*) override {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_POWERBROADCAST) {
            switch (msg->wParam) {
                case PBT_APMRESUMEAUTOMATIC:
                case PBT_APMRESUMESUSPEND:
                    onWake();
                    break;

                case PBT_APMSUSPEND:
                    onSleep();
                    break;
            }
        }
        return false;
    }
};

void PlatformHelper::setup() {
    DependencyManager::set<PlatformHelper, WinHelper>();
}

#endif
