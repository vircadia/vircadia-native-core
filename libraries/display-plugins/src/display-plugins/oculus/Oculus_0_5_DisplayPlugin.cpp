//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Oculus_0_5_DisplayPlugin.h"

#include <memory>

#include <QMainWindow>
#include <QGLWidget>
#include <GLMHelpers.h>
#include <GlWindow.h>
#include <QEvent>
#include <QResizeEvent>
#include <QOpenGLContext>
#include <QGuiApplication>
#include <QScreen>


#include <OVR_CAPI_GL.h>

#include <PerfStat.h>
#include <OglplusHelpers.h>

#include "plugins/PluginContainer.h"
#include "OculusHelpers.h"

using namespace Oculus;
ovrTexture _eyeTextures[2];
int _hmdScreen{ -1 };
bool _hswDismissed{ false };

DisplayPlugin* makeOculusDisplayPlugin() {
    return new Oculus_0_5_DisplayPlugin();
}

using namespace oglplus;

const QString Oculus_0_5_DisplayPlugin::NAME("Oculus Rift (0.5)");

const QString & Oculus_0_5_DisplayPlugin::getName() const {
    return NAME;
}


bool Oculus_0_5_DisplayPlugin::isSupported() const {
#if (OVR_MAJOR_VERSION == 5)
    if (!ovr_Initialize(nullptr)) {
        return false;
    }
    bool result = false;
    if (ovrHmd_Detect() > 0) {
        result = true;
    }

    auto hmd = ovrHmd_Create(0);
    if (hmd) {
        QPoint targetPosition{ hmd->WindowsPos.x, hmd->WindowsPos.y };
        auto screens = qApp->screens();
        for(int i = 0; i < screens.size(); ++i) {
            auto screen = screens[i];
            QPoint position = screen->geometry().topLeft();
            if (position == targetPosition) {
                _hmdScreen = i;
                break;
            }
        }
    }
  
    ovr_Shutdown();
    return result;
#else 
    return false;
#endif
}

void Oculus_0_5_DisplayPlugin::activate() {
#if (OVR_MAJOR_VERSION == 5)
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        Q_ASSERT(false);
        qFatal("Failed to Initialize SDK");
    }
    _hswDismissed = false;
    _hmd = ovrHmd_Create(0);
    if (!_hmd) {
        qFatal("Failed to acquire HMD");
    }
    
    OculusBaseDisplayPlugin::activate();
    int screen = getHmdScreen();
    if (screen != -1) {
        CONTAINER->setFullscreen(qApp->screens()[screen]);
    }
    
    _window->installEventFilter(this);
    _window->makeCurrent();
    ovrGLConfig config; memset(&config, 0, sizeof(ovrRenderAPIConfig));
    auto& header = config.Config.Header;
    header.API = ovrRenderAPI_OpenGL;
    header.BackBufferSize = _hmd->Resolution;
    header.Multisample = 1;
    int distortionCaps = 0
        | ovrDistortionCap_TimeWarp
        ;

    memset(_eyeTextures, 0, sizeof(ovrTexture) * 2);
    ovr_for_each_eye([&](ovrEyeType eye) {
        auto& header = _eyeTextures[eye].Header;
        header.API = ovrRenderAPI_OpenGL;
        header.TextureSize = { (int)_desiredFramebufferSize.x, (int)_desiredFramebufferSize.y };
        header.RenderViewport.Size = header.TextureSize;
        header.RenderViewport.Size.w /= 2;
        if (eye == ovrEye_Right) {
            header.RenderViewport.Pos.x = header.RenderViewport.Size.w;
        }
    });

    ovrEyeRenderDesc _eyeRenderDescs[ovrEye_Count];
    ovrBool result = ovrHmd_ConfigureRendering(_hmd, &config.Config, distortionCaps, _eyeFovs, _eyeRenderDescs);
    Q_ASSERT(result);
#endif
}

void Oculus_0_5_DisplayPlugin::deactivate() {
#if (OVR_MAJOR_VERSION == 5)
    _window->removeEventFilter(this);

    OculusBaseDisplayPlugin::deactivate();
    
    QScreen* riftScreen = nullptr;
    if (_hmdScreen >= 0) {
        riftScreen = qApp->screens()[_hmdScreen];
    }
    CONTAINER->unsetFullscreen(riftScreen);
    
    ovrHmd_Destroy(_hmd);
    _hmd = nullptr;
    ovr_Shutdown();
#endif
}

void Oculus_0_5_DisplayPlugin::preRender() {
#if (OVR_MAJOR_VERSION == 5)
    OculusBaseDisplayPlugin::preRender();
    ovrHmd_BeginFrame(_hmd, _frameIndex);
#endif
}

void Oculus_0_5_DisplayPlugin::preDisplay() {
    _window->makeCurrent();
}

void Oculus_0_5_DisplayPlugin::display(GLuint finalTexture, const glm::uvec2& sceneSize) {
    ++_frameIndex;
#if (OVR_MAJOR_VERSION == 5)
    ovr_for_each_eye([&](ovrEyeType eye) {
        reinterpret_cast<ovrGLTexture&>(_eyeTextures[eye]).OGL.TexId = finalTexture;
    });
    ovrHmd_EndFrame(_hmd, _eyePoses, _eyeTextures);
#endif
}

// Pass input events on to the application
bool Oculus_0_5_DisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
#if (OVR_MAJOR_VERSION == 5)
    if (!_hswDismissed && (event->type() == QEvent::KeyPress)) {
        static ovrHSWDisplayState hswState;
        ovrHmd_GetHSWDisplayState(_hmd, &hswState);
        if (hswState.Displayed) {
            ovrHmd_DismissHSWDisplay(_hmd);
        } else {
            _hswDismissed = true;
        }
    }	
#endif
    return OculusBaseDisplayPlugin::eventFilter(receiver, event);
}

// FIXME mirroring tot he main window is diffucult on OSX because it requires that we
// trigger a swap, which causes the client to wait for the v-sync of the main screen running
// at 60 Hz.  This would introduce judder.  Perhaps we can push mirroring to a separate
// thread
void Oculus_0_5_DisplayPlugin::finishFrame() {
    _window->doneCurrent();
};

int Oculus_0_5_DisplayPlugin::getHmdScreen() const {
    return _hmdScreen;
}
