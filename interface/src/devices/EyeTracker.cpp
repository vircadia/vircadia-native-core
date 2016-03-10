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

#include <QFuture>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrentRun>

#include <SharedUtil.h>

#include "InterfaceLogging.h"
#include "OctreeConstants.h"

#ifdef HAVE_IVIEWHMD
char* HIGH_FIDELITY_EYE_TRACKER_CALIBRATION = "HighFidelityEyeTrackerCalibration";
#endif

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
    if (_isStreaming) {
        int result = smi_quit();
        if (result != SMI_RET_SUCCESS) {
            qCWarning(interfaceapp) << "Eye Tracker: Error terminating tracking:" << smiReturnValueToString(result);
        }
    }
#endif
}

#ifdef HAVE_IVIEWHMD
void EyeTracker::processData(smi_CallbackDataStruct* data) {
    _lastProcessDataTimestamp = usecTimestampNow();

    if (!_isEnabled) {
        return;
    }

    if (data->type == SMI_SIMPLE_GAZE_SAMPLE) {
        // Calculate the intersections of the left and right eye look-at vectors with a vertical plane along the monocular
        // gaze direction. Average these positions to give the look-at point.
        // If the eyes are parallel or diverged, gaze at a distant look-at point calculated the same as for non eye tracking.
        // Line-plane intersection: https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection

        smi_SampleHMDStruct* sample = (smi_SampleHMDStruct*)data->result;
        // The iViewHMD coordinate system has x and z axes reversed compared to Interface, i.e., wearing the HMD:
        // - x is left
        // - y is up
        // - z is forwards

        // Plane
        smi_Vec3d point = sample->gazeBasePoint;  // mm
        smi_Vec3d direction = sample->gazeDirection;
        glm::vec3 planePoint = glm::vec3(-point.x, point.y, -point.z) / 1000.0f;
        glm::vec3 planeNormal = glm::vec3(-direction.z, 0.0f, direction.x);
        glm::vec3 monocularDirection = glm::vec3(-direction.x, direction.y, -direction.z);

        // Left eye
        point = sample->left.gazeBasePoint;  // mm
        direction = sample->left.gazeDirection;
        glm::vec3 leftLinePoint = glm::vec3(-point.x, point.y, -point.z) / 1000.0f;
        glm::vec3 leftLineDirection = glm::vec3(-direction.x, direction.y, -direction.z);

        // Right eye
        point = sample->right.gazeBasePoint;  // mm
        direction = sample->right.gazeDirection;
        glm::vec3 rightLinePoint = glm::vec3(-point.x, point.y, -point.z) / 1000.0f;
        glm::vec3 rightLineDirection = glm::vec3(-direction.x, direction.y, -direction.z);

        // Plane - line dot products
        float leftLinePlaneDotProduct = glm::dot(leftLineDirection, planeNormal);
        float rightLinePlaneDotProduct = glm::dot(rightLineDirection, planeNormal);

        // Gaze into distance if eyes are parallel or diverged; otherwise the look-at is the average of look-at points
        glm::vec3 lookAtPosition;
        if (abs(leftLinePlaneDotProduct) <= FLT_EPSILON || abs(rightLinePlaneDotProduct) <= FLT_EPSILON) {
            lookAtPosition = monocularDirection * (float)TREE_SCALE;
        } else {
            float leftDistance = glm::dot(planePoint - leftLinePoint, planeNormal) / leftLinePlaneDotProduct;
            float rightDistance = glm::dot(planePoint - rightLinePoint, planeNormal) / rightLinePlaneDotProduct;
            if (leftDistance <= 0.0f || rightDistance <= 0.0f 
                || leftDistance > (float)TREE_SCALE || rightDistance > (float)TREE_SCALE) {
                lookAtPosition = monocularDirection * (float)TREE_SCALE;
            } else {
                glm::vec3 leftIntersectionPoint = leftLinePoint + leftDistance * leftLineDirection;
                glm::vec3 rightIntersectionPoint = rightLinePoint + rightDistance * rightLineDirection;
                lookAtPosition = (leftIntersectionPoint + rightIntersectionPoint) / 2.0f;
            }
        }

        if (glm::isnan(lookAtPosition.x) || glm::isnan(lookAtPosition.y) || glm::isnan(lookAtPosition.z)) {
            return;
        }

        _lookAtPosition = lookAtPosition;
    }
}
#endif

void EyeTracker::init() {
    if (_isInitialized) {
        qCWarning(interfaceapp) << "Eye Tracker: Already initialized";
        return;
    }
}

#ifdef HAVE_IVIEWHMD
int EyeTracker::startStreaming(bool simulate) {
    return smi_startStreaming(simulate);  // This call blocks execution.
}
#endif

#ifdef HAVE_IVIEWHMD
void EyeTracker::onStreamStarted() {
    if (!_isInitialized) {
        return;
    }

    int result = _startStreamingWatcher.result();
    _isStreaming = (result == SMI_RET_SUCCESS);

    if (result != SMI_RET_SUCCESS) {
        qCWarning(interfaceapp) << "Eye Tracker: Error starting streaming:" << smiReturnValueToString(result);
        // Display error dialog unless SMI SDK has already displayed an error message.
        if (result != SMI_ERROR_HMD_NOT_SUPPORTED) {
            OffscreenUi::warning(nullptr, "Eye Tracker Error", smiReturnValueToString(result));
        }
    } else {
        qCDebug(interfaceapp) << "Eye Tracker: Started streaming";
    }

    if (_isStreaming) {
       // Automatically load calibration if one has been saved.
       QString availableCalibrations = QString(smi_getAvailableCalibrations());
       if (availableCalibrations.contains(HIGH_FIDELITY_EYE_TRACKER_CALIBRATION)) {
           result = smi_loadCalibration(HIGH_FIDELITY_EYE_TRACKER_CALIBRATION);
           if (result != SMI_RET_SUCCESS) {
               qCWarning(interfaceapp) << "Eye Tracker: Error loading calibration:" << smiReturnValueToString(result);
               OffscreenUi::warning(nullptr, "Eye Tracker Error", "Error loading calibration"
                   + smiReturnValueToString(result));
           } else {
               qCDebug(interfaceapp) << "Eye Tracker: Loaded calibration";
           }
       }
    }
}
#endif

void EyeTracker::setEnabled(bool enabled, bool simulate) {
    if (enabled && !_isInitialized) {
#ifdef HAVE_IVIEWHMD
        int result = smi_setCallback(eyeTrackerCallback);
        if (result != SMI_RET_SUCCESS) {
            qCWarning(interfaceapp) << "Eye Tracker: Error setting callback:" << smiReturnValueToString(result);
            OffscreenUi::warning(nullptr, "Eye Tracker Error", smiReturnValueToString(result));
        } else {
            _isInitialized = true;
        }

        connect(&_startStreamingWatcher, SIGNAL(finished()), this, SLOT(onStreamStarted()));
#endif
    }

    if (!_isInitialized) {
        return;
    }

#ifdef HAVE_IVIEWHMD
    qCDebug(interfaceapp) << "Eye Tracker: Set enabled =" << enabled << ", simulate =" << simulate;

    // There is no smi_stopStreaming() method and after an smi_quit(), streaming cannot be restarted (at least not for 
    // simulated data). So keep streaming once started in case tracking is re-enabled after stopping.

    // Try to stop streaming if changing whether simulating or not.
    if (enabled && _isStreaming && _isStreamSimulating != simulate) {
        int result = smi_quit();
        if (result != SMI_RET_SUCCESS) {
            qCWarning(interfaceapp) << "Eye Tracker: Error stopping streaming:" << smiReturnValueToString(result);
        }
        _isStreaming = false;
    }

    if (enabled && !_isStreaming) {
        // Start SMI streaming in a separate thread because it blocks.
        QFuture<int> future = QtConcurrent::run(this, &EyeTracker::startStreaming, simulate);
        _startStreamingWatcher.setFuture(future);
        _isStreamSimulating = simulate;
    }

    _isEnabled = enabled;
    _isSimulating = simulate;

#endif
}

void EyeTracker::reset() {
    // Nothing to do.
}

bool EyeTracker::isTracking() const {
    static const quint64 ACTIVE_TIMEOUT_USECS = 2000000;  // 2 secs
    return _isEnabled && (usecTimestampNow() - _lastProcessDataTimestamp < ACTIVE_TIMEOUT_USECS);
}

#ifdef HAVE_IVIEWHMD
void EyeTracker::calibrate(int points) {

    if (!_isStreaming) {
        qCWarning(interfaceapp) << "Eye Tracker: Cannot calibrate because not streaming";
        return;
    }

    smi_CalibrationHMDStruct* calibrationHMDStruct;
    smi_createCalibrationHMDStruct(&calibrationHMDStruct);

    smi_CalibrationTypeEnum calibrationType;
    switch (points) {
        case 1:
            calibrationType = SMI_ONE_POINT_CALIBRATION;
            qCDebug(interfaceapp) << "Eye Tracker: One point calibration";
            break;
        case 3:
            calibrationType = SMI_THREE_POINT_CALIBRATION;
            qCDebug(interfaceapp) << "Eye Tracker: Three point calibration";
            break;
        case 5:
            calibrationType = SMI_FIVE_POINT_CALIBRATION;
            qCDebug(interfaceapp) << "Eye Tracker: Five point calibration";
            break;
        default:
            qCWarning(interfaceapp) << "Eye Tracker: Invalid calibration specified";
            return;
    }

    calibrationHMDStruct->type = calibrationType;
    calibrationHMDStruct->backgroundColor->blue = 0.5;
    calibrationHMDStruct->backgroundColor->green = 0.5;
    calibrationHMDStruct->backgroundColor->red = 0.5;
    calibrationHMDStruct->foregroundColor->blue = 1.0;
    calibrationHMDStruct->foregroundColor->green = 1.0;
    calibrationHMDStruct->foregroundColor->red = 1.0;
    
    int result = smi_setupCalibration(calibrationHMDStruct);
    if (result != SMI_RET_SUCCESS) {
        qCWarning(interfaceapp) << "Eye Tracker: Error setting up calibration:" << smiReturnValueToString(result);
        return;
    } else {
        result = smi_calibrate();
        if (result != SMI_RET_SUCCESS) {
            qCWarning(interfaceapp) << "Eye Tracker: Error performing calibration:" << smiReturnValueToString(result);
        } else {
            result = smi_saveCalibration(HIGH_FIDELITY_EYE_TRACKER_CALIBRATION);
            if (result != SMI_RET_SUCCESS) {
               qCWarning(interfaceapp) << "Eye Tracker: Error saving calibration:" << smiReturnValueToString(result);
            }
        }
    }

    if (result != SMI_RET_SUCCESS) {
        OffscreenUi::warning(nullptr, "Eye Tracker Error", "Calibration error: " + smiReturnValueToString(result));
    }
}
#endif

#ifdef HAVE_IVIEWHMD
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
        case smi_ErrorReturnValue::SMI_ERROR_FILE_NOT_FOUND:
           return "File not found";
        case smi_ErrorReturnValue::SMI_ERROR_FILE_EMPTY:
           return "File empty";
        case smi_ErrorReturnValue::SMI_ERROR_UNKNOWN:
            return "Unknown error";
        default:
            QString number;
            number.setNum(value);
            return number;
    }
}
#endif
