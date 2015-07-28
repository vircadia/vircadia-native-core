//
//  EyeTracker.cpp
//  interface/src/devices
//
//  Created by David Rowe on 27 Jul 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EyeTracker.h"

#include "InterfaceLogging.h"

#ifdef HAVE_IVIEWHMD
static void CALLBACK eyeTrackerCallback(smi_CallbackDataStruct* data) {
    auto eyeTracker = DependencyManager::get<EyeTracker>();
    if (eyeTracker) {  // Guard against a few callbacks that continue to be received after smi_quit().
        eyeTracker->processData(data);
    }
}
#endif

EyeTracker::~EyeTracker() {
#ifdef HAVE_IVIEWHMD
    int result = smi_quit();
    if (result != SMI_RET_SUCCESS) {
        qCWarning(interfaceapp) << "Eye Tracker: Error terminating tracking:" << result;
    }
#endif
}

void EyeTracker::processData(smi_CallbackDataStruct* data) {
    if (!_isEnabled) {
        return;
    }

#ifdef HAVE_IVIEWHMD
    if (data->type == SMI_SIMPLE_GAZE_SAMPLE) {
        smi_SampleHMDStruct* sample = (smi_SampleHMDStruct*)data->result;
    }
#endif
}

void EyeTracker::init() {
    if (_isInitialized) {
        qCWarning(interfaceapp) << "Eye Tracker: Already initialized";
        return;
    }

#ifdef HAVE_IVIEWHMD
    int result = smi_setCallback(eyeTrackerCallback);
    if (result != SMI_RET_SUCCESS) {
        qCWarning(interfaceapp) << "Eye Tracker: Error setting callback:" << result;
    } else {
        _isInitialized = true;
    }
#endif
}

void EyeTracker::setEnabled(bool enabled) {
    if (!_isInitialized) {
        qCWarning(interfaceapp) << "Eye Tracker: Not initialized before setting enabled";
        return;
    }

#ifdef HAVE_IVIEWHMD
    qCDebug(interfaceapp) << "Eye Tracker: Set enabled =" << enabled;

    if (enabled && !_isStreaming) {
        // There is no smi_stopStreaming() method so start streaming a maximum of once per program run.
        int result = smi_startStreaming(true);
        if (result != SMI_RET_SUCCESS) {
            qCWarning(interfaceapp) << "Eye Tracker: Error starting streaming:" << result;
        } else {
            _isStreaming = true;
        }
    }

    _isEnabled = enabled && _isStreaming;
#endif
}

void EyeTracker::reset() {
}
