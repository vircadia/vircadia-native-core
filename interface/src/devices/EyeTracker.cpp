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
#include "OctreeConstants.h"

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

#ifdef HAVE_IVIEWHMD
void EyeTracker::processData(smi_CallbackDataStruct* data) {
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
        if (abs(leftLinePlaneDotProduct) <= FLT_EPSILON || abs(rightLinePlaneDotProduct) <= FLT_EPSILON) {
            _lookAtPosition = monocularDirection * (float)TREE_SCALE;
        } else {
            float leftDistance = glm::dot(planePoint - leftLinePoint, planeNormal) / leftLinePlaneDotProduct;
            float rightDistance = glm::dot(planePoint - rightLinePoint, planeNormal) / rightLinePlaneDotProduct;
            if (leftDistance <= 0.0f || rightDistance <= 0.0f 
                || leftDistance > (float)TREE_SCALE || rightDistance > (float)TREE_SCALE) {
                _lookAtPosition = monocularDirection * (float)TREE_SCALE;
            } else {
                glm::vec3 leftIntersectionPoint = leftLinePoint + leftDistance * leftLineDirection;
                glm::vec3 rightIntersectionPoint = rightLinePoint + rightDistance * rightLineDirection;
                _lookAtPosition = (leftIntersectionPoint + rightIntersectionPoint) / 2.0f;
            }
        }
    }
}
#endif

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
    _isSimulating = _isEnabled && simulate;

    if (!success && result != SMI_ERROR_HMD_NOT_SUPPORTED) {
        // Display error dialog after updating _isEnabled. Except if SMI SDK has already displayed an error message.
        QMessageBox::warning(nullptr, "Eye Tracker Error", smiReturnValueToString(result));
    }
#endif
}

void EyeTracker::reset() {
    // Nothing to do.
}

#ifdef HAVE_IVIEWHMD
void EyeTracker::calibrate(int points) {
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
        }
    }

    if (result != SMI_RET_SUCCESS) {
        QMessageBox::warning(nullptr, "Eye Tracker Error", "Calibration error: " + smiReturnValueToString(result));
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
        case smi_ErrorReturnValue::SMI_ERROR_UNKNOWN:
            return "Unknown error";
        default:
            QString number;
            number.setNum(value);
            return number;
    }
}
#endif
