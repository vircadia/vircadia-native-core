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
#include <qpa/qplatformnativeinterface.h>
#include <QtWidgets/QMenuBar>

#include <VrApi/VrApi.h>

#include "GVRMainWindow.h"
#include "RenderingClient.h"

#include "GVRInterface.h"

GVRInterface::GVRInterface(int argc, char* argv[]) : 
    QApplication(argc, argv)
{
    _client = new RenderingClient(this);
    
    connect(this, &QGuiApplication::applicationStateChanged, this, &GVRInterface::handleApplicationStateChange);
    
    QAndroidJniEnvironment jniEnv;
    
    QPlatformNativeInterface* interface = QApplication::platformNativeInterface();
    jobject activity = (jobject) interface->nativeResourceForIntegration("QtActivity");

    ovr_RegisterHmtReceivers(&*jniEnv, activity);
}

void GVRInterface::handleApplicationStateChange(Qt::ApplicationState state) {
    switch(state) {
        case Qt::ApplicationActive:
            qDebug() << "The application is active.";
            resumeOVR();
            break;
        case Qt::ApplicationSuspended:
            qDebug() << "The application is being suspended.";
            pauseOVR();
            break;
        default:
            break;
    }
}

void GVRInterface::resumeOVR() {
    // Reload local preferences, in case we are coming back from a
    // switch to the dashboard that changed them.
    // ovr_UpdateLocalPreferences();
//
//     // Default vrModeParms
//     ovrModeParms vrModeParms;
//     ovrHmdInfo hmdInfo;
//
//     const char* cpuLevelStr = ovr_GetLocalPreferenceValueForKey( LOCAL_PREF_DEV_CPU_LEVEL, "-1" );
//     const int cpuLevel = atoi( cpuLevelStr );
//     if ( cpuLevel >= 0 ) {
//         vrModeParms.CpuLevel = cpuLevel;
//         qDebug() << "Local Preferences: Setting cpuLevel" << vrModeParms.CpuLevel;
//     }
//
//     const char* gpuLevelStr = ovr_GetLocalPreferenceValueForKey( LOCAL_PREF_DEV_GPU_LEVEL, "-1" );
//     const int gpuLevel = atoi( gpuLevelStr );
//     if ( gpuLevel >= 0 ) {
//         vrModeParms.GpuLevel = gpuLevel;
//         qDebug() << "Local Preferences: Setting gpuLevel" << vrModeParms.GpuLevel;
//     }
//
//     ovr_EnterVrMode(vrModeParms, &hmdInfo);
}

void GVRInterface::pauseOVR() {
    // ovr_LeaveVrMode();
}
