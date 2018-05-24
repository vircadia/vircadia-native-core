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

#include "GVRInterface.h"

#ifdef ANDROID

#include <jni.h>

#include <qpa/qplatformnativeinterface.h>
#include <QtAndroidExtras/QAndroidJniEnvironment>
#include <QtAndroidExtras/QAndroidJniObject>

#ifdef HAVE_LIBOVR

#include <KeyState.h>
#include <VrApi/VrApi.h>

#endif
#endif

#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QMenuBar>

#include "GVRMainWindow.h"
#include "RenderingClient.h"

static QString launchURLString = QString();

#ifdef ANDROID

extern "C" {
    
JNIEXPORT void Java_io_highfidelity_gvrinterface_InterfaceActivity_handleHifiURL(JNIEnv *jni, jclass clazz, jstring hifiURLString) {
    launchURLString = QAndroidJniObject(hifiURLString).toString();
}

}

#endif

GVRInterface::GVRInterface(int argc, char* argv[]) : 
    QApplication(argc, argv),
    _mainWindow(NULL),
    _inVRMode(false)
{
    setApplicationName("gvr-interface");
    setOrganizationName("highfidelity");
    setOrganizationDomain("io");
    
    if (!launchURLString.isEmpty()) {
        // did we get launched with a lookup URL? If so it is time to give that to the AddressManager
        qDebug() << "We were opened via a hifi URL -" << launchURLString;
    }
    
    _client = new RenderingClient(this, launchURLString);
    
    launchURLString = QString();
    
    connect(this, &QGuiApplication::applicationStateChanged, this, &GVRInterface::handleApplicationStateChange);

#if defined(ANDROID) && defined(HAVE_LIBOVR)
    QAndroidJniEnvironment jniEnv;
    
    QPlatformNativeInterface* interface = QApplication::platformNativeInterface();
    jobject activity = (jobject) interface->nativeResourceForIntegration("QtActivity");

    ovr_RegisterHmtReceivers(&*jniEnv, activity);
    
    // PLATFORMACTIVITY_REMOVAL: Temp workaround for PlatformActivity being
    // stripped from UnityPlugin. Alternate is to use LOCAL_WHOLE_STATIC_LIBRARIES
    // but that increases the size of the plugin by ~1MiB
    OVR::linkerPlatformActivity++;
#endif
    
    // call our idle function whenever we can
    QTimer* idleTimer = new QTimer(this);
    connect(idleTimer, &QTimer::timeout, this, &GVRInterface::idle);
    idleTimer->start(0);
    
    // call our quit handler before we go down
    connect(this, &QCoreApplication::aboutToQuit, this, &GVRInterface::handleApplicationQuit);
}

void GVRInterface::handleApplicationQuit() {
    _client->cleanupBeforeQuit();
}

void GVRInterface::idle() {
#if defined(ANDROID) && defined(HAVE_LIBOVR)
    if (!_inVRMode && ovr_IsHeadsetDocked()) {
        qDebug() << "The headset just got docked - enter VR mode.";
        enterVRMode();
    } else if (_inVRMode) {
        
        if (ovr_IsHeadsetDocked()) {
            static int counter = 0;
            
            // Get the latest head tracking state, predicted ahead to the midpoint of the time
            // it will be displayed.  It will always be corrected to the real values by
            // time warp, but the closer we get, the less black will be pulled in at the edges.
            const double now = ovr_GetTimeInSeconds();
            static double prev;
            const double rawDelta = now - prev;
            prev = now;
            const double clampedPrediction = std::min( 0.1, rawDelta * 2);
            ovrSensorState sensor = ovrHmd_GetSensorState(OvrHmd, now + clampedPrediction, true );   
            
            auto ovrOrientation = sensor.Predicted.Pose.Orientation;
            glm::quat newOrientation(ovrOrientation.w, ovrOrientation.x, ovrOrientation.y, ovrOrientation.z);
            _client->setOrientation(newOrientation);
            
            if (counter++ % 100000 == 0) {
                qDebug() << "GetSensorState in frame" << counter << "-" 
                    << ovrOrientation.x <<  ovrOrientation.y <<  ovrOrientation.z <<  ovrOrientation.w;
            }
        } else {
            qDebug() << "The headset was undocked - leaving VR mode.";
            
            leaveVRMode();
        }
    } 
    
    OVR::KeyState& backKeyState = _mainWindow->getBackKeyState();
    auto backEvent = backKeyState.Update(ovr_GetTimeInSeconds());

    if (backEvent == OVR::KeyState::KEY_EVENT_LONG_PRESS) {
        qDebug() << "Attemping to start the Platform UI Activity.";
        ovr_StartPackageActivity(_ovr, PUI_CLASS_NAME, PUI_GLOBAL_MENU);
    } else if (backEvent == OVR::KeyState::KEY_EVENT_DOUBLE_TAP || backEvent == OVR::KeyState::KEY_EVENT_SHORT_PRESS) {
        qDebug() << "Got an event we should cancel for!";
    } else if (backEvent == OVR::KeyState::KEY_EVENT_DOUBLE_TAP) {
        qDebug() << "The button is down!";
    }
#endif
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
#if defined(ANDROID) && defined(HAVE_LIBOVR)
    // Default vrModeParms
    ovrModeParms vrModeParms;
    vrModeParms.AsynchronousTimeWarp = true;
    vrModeParms.AllowPowerSave = true;
    vrModeParms.DistortionFileName = NULL;
    vrModeParms.EnableImageServer = false;
    vrModeParms.CpuLevel = 2;
    vrModeParms.GpuLevel = 2;
    vrModeParms.GameThreadTid = 0;
    
    QAndroidJniEnvironment jniEnv;
    
    QPlatformNativeInterface* interface = QApplication::platformNativeInterface();
    jobject activity = (jobject) interface->nativeResourceForIntegration("QtActivity");
    
    vrModeParms.ActivityObject = activity;
    
    ovrHmdInfo hmdInfo;
    _ovr = ovr_EnterVrMode(vrModeParms, &hmdInfo);
    
    _inVRMode = true;
#endif
}

void GVRInterface::leaveVRMode() {
#if defined(ANDROID) && defined(HAVE_LIBOVR)
    ovr_LeaveVrMode(_ovr);
    _inVRMode = false;
#endif
}
