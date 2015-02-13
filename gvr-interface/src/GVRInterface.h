//
//  GVRInterface.h
//  gvr-interface/src
//
//  Created by Stephen Birarda on 11/18/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GVRInterface_h
#define hifi_GVRInterface_h

#include <QtWidgets/QApplication>

#if defined(ANDROID) && defined(HAVE_LIBOVR)
class ovrMobile;
class ovrHmdInfo;

// This is set by JNI_OnLoad() when the .so is initially loaded.
// Must use to attach each thread that will use JNI:
namespace OVR {
    // PLATFORMACTIVITY_REMOVAL: Temp workaround for PlatformActivity being
    // stripped from UnityPlugin. Alternate is to use LOCAL_WHOLE_STATIC_LIBRARIES
    // but that increases the size of the plugin by ~1MiB
    extern int linkerPlatformActivity;
}

#endif 

class GVRMainWindow;
class RenderingClient;
class QKeyEvent;

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<GVRInterface*>(QApplication::instance()))

class GVRInterface : public QApplication {
    Q_OBJECT
public:
    GVRInterface(int argc, char* argv[]);   
    RenderingClient* getClient() { return _client; }
    
    void setMainWindow(GVRMainWindow* mainWindow) { _mainWindow = mainWindow; }
    
protected:
    void keyPressEvent(QKeyEvent* event);
        
private slots:
    void handleApplicationStateChange(Qt::ApplicationState state);
    void idle();
private:
    void handleApplicationQuit();
    
    void enterVRMode();
    void leaveVRMode();
    
#if defined(ANDROID) && defined(HAVE_LIBOVR)
    ovrMobile* _ovr;
    ovrHmdInfo* _hmdInfo;
#endif
    
    GVRMainWindow* _mainWindow;
    
    RenderingClient* _client;
    bool _inVRMode;
};

#endif // hifi_GVRInterface_h
