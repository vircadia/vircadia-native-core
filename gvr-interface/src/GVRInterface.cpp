//
//  GVRInterface.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 11/18/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtAndroidExtras/QAndroidJniEnvironment>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtCore/QTimer>
#include <qpa/qplatformnativeinterface.h>
#include <QtWidgets/QMenuBar>

#include <VrApi/VrApi.h>
#include <VrApi/VrApi_local.h>
#include <VrApi/LocalPreferences.h>

#include "GVRMainWindow.h"
#include "RenderingClient.h"

#include "GVRInterface.h"

GVRInterface::GVRInterface(int argc, char* argv[]) : 
    QApplication(argc, argv),
    _inVRMode(false)
{
    _client = new RenderingClient(this);
    
    connect(this, &QGuiApplication::applicationStateChanged, this, &GVRInterface::handleApplicationStateChange);
    
    QAndroidJniEnvironment jniEnv;
    
    QPlatformNativeInterface* interface = QApplication::platformNativeInterface();
    jobject activity = (jobject) interface->nativeResourceForIntegration("QtActivity");

    ovr_RegisterHmtReceivers(&*jniEnv, activity);
    
    // call our idle function whenever we can
    QTimer* idleTimer = new QTimer(this);
    connect(idleTimer, &QTimer::timeout, this, &GVRInterface::idle);
    idleTimer->start(0);
}

void GVRInterface::idle() {
    if (!_inVRMode && ovr_IsHeadsetDocked()) {
        qDebug() << "The headset just got docked - we should try and go into VR mode";
        _inVRMode = true;
        enterVRMode();
    } else if (_inVRMode) {
        if (!ovr_IsHeadsetDocked()) {
            qDebug() << "The headset was just undocked - we should leave VR mode now";
            leaveVRMode();
        } else {
            // Get the latest head tracking state, predicted ahead to the midpoint of the time
            // it will be displayed.  It will always be corrected to the real values by
            // time warp, but the closer we get, the less black will be pulled in at the edges.
            const double now = ovr_GetTimeInSeconds();
            static double prev;
            const double rawDelta = now - prev;
            prev = now;
            const double clampedPrediction = std::min( 0.1, rawDelta * 2);
            ovrSensorState sensor = ovrHmd_GetSensorState(OvrHmd, now + clampedPrediction, true );   
            
            float w = sensor.Predicted.Pose.Orientation.w;
            float x = sensor.Predicted.Pose.Orientation.x;
            float y = sensor.Predicted.Pose.Orientation.y;
            float z = sensor.Predicted.Pose.Orientation.z;
            
            qDebug() << "GetSensorState: " << x << y << z << w;
        }
       
    } 
}

void GVRInterface::handleApplicationStateChange(Qt::ApplicationState state) {
    switch(state) {
        case Qt::ApplicationActive:
            qDebug() << "The application is active.";
            break;
        case Qt::ApplicationSuspended:
            qDebug() << "The application is being suspended.";
            break;
        default:
            break;
    }
}

void GVRInterface::enterVRMode() {
    // Reload local preferences, in case we are coming back from a
    // switch to the dashboard that changed them.
    ovr_UpdateLocalPreferences();

    // Default vrModeParms
    ovrModeParms vrModeParms;
    vrModeParms.AsynchronousTimeWarp = true;
    vrModeParms.AllowPowerSave = true;
    vrModeParms.DistortionFileName = NULL;
    vrModeParms.EnableImageServer = false;
    vrModeParms.CpuLevel = 2;
    vrModeParms.GpuLevel = 2;
    vrModeParms.GameThreadTid = 0;
    
    QPlatformNativeInterface* interface = QApplication::platformNativeInterface();
    
    vrModeParms.ActivityObject = (jobject) interface->nativeResourceForIntegration("QtActivity");
    
    _hmdInfo = new ovrHmdInfo;

    const char* cpuLevelStr = ovr_GetLocalPreferenceValueForKey( LOCAL_PREF_DEV_CPU_LEVEL, "-1" );
    const int cpuLevel = atoi( cpuLevelStr );
    if ( cpuLevel >= 0 ) {
        vrModeParms.CpuLevel = cpuLevel;
        qDebug() << "Local Preferences: Setting cpuLevel" << vrModeParms.CpuLevel;
    }

    const char* gpuLevelStr = ovr_GetLocalPreferenceValueForKey( LOCAL_PREF_DEV_GPU_LEVEL, "-1" );
    const int gpuLevel = atoi( gpuLevelStr );
    if ( gpuLevel >= 0 ) {
        vrModeParms.GpuLevel = gpuLevel;
        qDebug() << "Local Preferences: Setting gpuLevel" << vrModeParms.GpuLevel;
    }

    _ovr = ovr_EnterVrMode(vrModeParms, _hmdInfo);
}

void GVRInterface::leaveVRMode() {
    ovr_LeaveVrMode(_ovr);
}
