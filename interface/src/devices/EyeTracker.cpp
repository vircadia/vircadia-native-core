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

#include <QMessageBox>

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
        qCWarning(interfaceapp) << "Eye Tracker: Error terminating tracking:" << smiReturnValueToString(result);
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
        qCWarning(interfaceapp) << "Eye Tracker: Error setting callback:" << smiReturnValueToString(result);
        QMessageBox::warning(nullptr, "Eye Tracker Error", smiReturnValueToString(result));
    } else {
        _isInitialized = true;
    }
#endif
}

void EyeTracker::setEnabled(bool enabled, bool simulate) {
    if (!_isInitialized) {
        return;
    }

#ifdef HAVE_IVIEWHMD
    qCDebug(interfaceapp) << "Eye Tracker: Set enabled =" << enabled << ", simulate =" << simulate;
    bool success = true;
    int result = 0;
    if (enabled) {
        // There is no smi_stopStreaming() method so keep streaming once started in case tracking is re-enabled after stopping.
        result = smi_startStreaming(simulate);
        if (result != SMI_RET_SUCCESS) {
            qCWarning(interfaceapp) << "Eye Tracker: Error starting streaming:" << smiReturnValueToString(result);
            success = false;
        }
    }

    _isEnabled = enabled && success;

    if (!success) {
        // Display error dialog after updating _isEnabled.
        QMessageBox::warning(nullptr, "Eye Tracker Error", smiReturnValueToString(result));
    }
#endif
}

void EyeTracker::reset() {
}

QString EyeTracker::smiReturnValueToString(int value) {
    switch (value)
    {
        case smi_ErrorReturnValue::SMI_ERROR_NO_CALLBACK_SET:
            return "No callback set";
        case smi_ErrorReturnValue::SMI_ERROR_CONNECTING_TO_HMD:
            return "Error connecting to HMD";
        case smi_ErrorReturnValue::SMI_ERROR_HMD_NOT_SUPPORTED:
            return "HMD not supported";
        case smi_ErrorReturnValue::SMI_ERROR_NOT_IMPLEMENTED:
            return "Not implmented";
        case smi_ErrorReturnValue::SMI_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case smi_ErrorReturnValue::SMI_ERROR_EYECAMERAS_NOT_AVAILABLE:
            return "Eye cameras not available";
        case smi_ErrorReturnValue::SMI_ERROR_OCULUS_RUNTIME_NOT_SUPPORTED:
            return "Oculus runtime not supported";
        case smi_ErrorReturnValue::SMI_ERROR_UNKNOWN:
            return "Unknown error";
        default:
            QString number;
            number.setNum(value);
            return number;
    }
}
