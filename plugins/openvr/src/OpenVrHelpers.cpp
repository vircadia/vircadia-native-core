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
#include <QtCore/QProcessEnvironment>

#include <Windows.h>

Q_DECLARE_LOGGING_CATEGORY(displayplugins)
Q_LOGGING_CATEGORY(displayplugins, "hifi.plugins.display")

using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;

static int refCount { 0 };
static Mutex mutex;
static vr::IVRSystem* activeHmd { nullptr };
static bool _openVrQuitRequested { false };

bool openVrQuitRequested() {
    return _openVrQuitRequested;
}

static const uint32_t RELEASE_OPENVR_HMD_DELAY_MS = 5000;

bool isOculusPresent() {
    bool result = false;
#if defined(Q_OS_WIN32) 
    HANDLE oculusServiceEvent = ::OpenEventW(SYNCHRONIZE, FALSE, L"OculusHMDConnected");
    // The existence of the service indicates a running Oculus runtime
    if (oculusServiceEvent) {
        // A signaled event indicates a connected HMD
        if (WAIT_OBJECT_0 == ::WaitForSingleObject(oculusServiceEvent, 0)) {
            result = true;
        }
        ::CloseHandle(oculusServiceEvent);
    }
#endif 
    return result;
}

bool openVrSupported() {
    static const QString DEBUG_FLAG("HIFI_DEBUG_OPENVR");
    static bool enableDebugOpenVR = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);
    return (enableDebugOpenVR || !isOculusPresent()) && vr::VR_IsHmdPresent();
}

vr::IVRSystem* acquireOpenVrSystem() {
    bool hmdPresent = vr::VR_IsHmdPresent();
    if (hmdPresent) {
        Lock lock(mutex);
        if (!activeHmd) {
            qCDebug(displayplugins) << "OpenVR: No vr::IVRSystem instance active, building";
            vr::EVRInitError eError = vr::VRInitError_None;
            activeHmd = vr::VR_Init(&eError, vr::VRApplication_Scene);
            qCDebug(displayplugins) << "OpenVR display: HMD is " << activeHmd << " error is " << eError;
        }
        if (activeHmd) {
            qCDebug(displayplugins) << "OpenVR: incrementing refcount";
            ++refCount;
        }
    } else {
        qCDebug(displayplugins) << "OpenVR: no hmd present";
    }
    return activeHmd;
}

void releaseOpenVrSystem() {
    if (activeHmd) {
        Lock lock(mutex);
        qCDebug(displayplugins) << "OpenVR: decrementing refcount";
        --refCount;
        if (0 == refCount) {
            qCDebug(displayplugins) << "OpenVR: zero refcount, deallocate VR system";
            vr::VR_Shutdown();
            _openVrQuitRequested = false;
            activeHmd = nullptr;
        }
    }
}

void handleOpenVrEvents() {
    if (!activeHmd) {
        return;
    }
    Lock lock(mutex);
    if (!activeHmd) {
        return;
    }

    vr::VREvent_t event;
    while (activeHmd->PollNextEvent(&event, sizeof(event))) {
        switch (event.eventType) {
            case vr::VREvent_Quit:
                _openVrQuitRequested = true;
                activeHmd->AcknowledgeQuit_Exiting();
                break;

            default:
                break;
        }
        qDebug() << "OpenVR: Event " << event.eventType;
    }

}
