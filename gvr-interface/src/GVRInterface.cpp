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

#ifdef HAVE_LIBOVR

#include <GlUtils.h>
#include <VrApi/LocalPreferences.h>
#include <VrApi/VrApi.h>
#include <VrApi/VrApi_local.h>

#endif

#include "GVRMainWindow.h"
#include "RenderingClient.h"

#include "GVRInterface.h"

GVRInterface::GVRInterface(int argc, char* argv[]) : 
    QApplication(argc, argv),
    _inVRMode(false)
{
    _client = new RenderingClient(this);
    
    connect(this, &QGuiApplication::applicationStateChanged, this, &GVRInterface::handleApplicationStateChange);

#ifdef HAVE_LIBOVR
    QAndroidJniEnvironment jniEnv;
    
    QPlatformNativeInterface* interface = QApplication::platformNativeInterface();
    jobject activity = (jobject) interface->nativeResourceForIntegration("QtActivity");


    ovr_RegisterHmtReceivers(&*jniEnv, activity);
#endif
    
    // call our idle function whenever we can
    QTimer* idleTimer = new QTimer(this);
    connect(idleTimer, &QTimer::timeout, this, &GVRInterface::idle);
    idleTimer->start(0);
}

void GVRInterface::idle() {
#ifdef HAVE_LIBOVR
    if (!_inVRMode && ovr_IsHeadsetDocked()) {
        qDebug() << "The headset just got docked - assume we are in VR mode.";
        _inVRMode = true;
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
            qDebug() << "The headset was undocked - assume we are no longer in VR mode.";
            _inVRMode = false;
        }
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
