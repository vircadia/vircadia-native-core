//
//  Created by Bradley Austin Davis on 2015/11/01
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenVrHelpers.h"

#include <atomic>
#include <mutex>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>


Q_DECLARE_LOGGING_CATEGORY(displayplugins)

using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;

static int refCount { 0 };
static Mutex mutex;
static vr::IVRSystem* activeHmd { nullptr };
static bool hmdPresent = vr::VR_IsHmdPresent();

static const uint32_t RELEASE_OPENVR_HMD_DELAY_MS = 5000;

vr::IVRSystem* acquireOpenVrSystem() {
    if (hmdPresent) {
        Lock lock(mutex);
        if (!activeHmd) {
            qCDebug(displayplugins) << "openvr: No vr::IVRSystem instance active, building";
            vr::EVRInitError eError = vr::VRInitError_None;
            activeHmd = vr::VR_Init(&eError);
            qCDebug(displayplugins) << "openvr display: HMD is " << activeHmd << " error is " << eError;
        }
        if (activeHmd) {
            qCDebug(displayplugins) << "openvr: incrementing refcount";
            ++refCount;
        }
    } else {
        qCDebug(displayplugins) << "openvr: no hmd present";
    }
    return activeHmd;
}

void releaseOpenVrSystem() {
    if (activeHmd) {
        Lock lock(mutex);
        qCDebug(displayplugins) << "openvr: decrementing refcount";
        --refCount;
        if (0 == refCount) {
            qCDebug(displayplugins) << "openvr: zero refcount, deallocate VR system";
            // Avoid spamming the VR system with activate/deactivate calls at system startup by
            // putting in a delay before we destory the shutdown the VR subsystem

            // FIXME releasing the VR system at all seems to trigger an exception deep inside the Oculus DLL.  
            // disabling for now.
            //QTimer* releaseTimer = new QTimer();
            //releaseTimer->singleShot(RELEASE_OPENVR_HMD_DELAY_MS, [releaseTimer] {
            //    Lock lock(mutex);
            //    qDebug() << "Delayed openvr destroy activated";
            //    if (0 == refCount && nullptr != activeHmd) {
            //        qDebug() << "Delayed openvr destroy: releasing resources";
            //        activeHmd = nullptr;
            //        vr::VR_Shutdown();
            //    } else {
            //        qDebug() << "Delayed openvr destroy: HMD still in use";
            //    }
            //    releaseTimer->deleteLater();
            //});
        }
    }
}
