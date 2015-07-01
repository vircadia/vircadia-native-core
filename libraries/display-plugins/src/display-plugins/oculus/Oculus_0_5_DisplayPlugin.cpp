//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Oculus_0_5_DisplayPlugin.h"

#if (OVR_MAJOR_VERSION == 5)

#include <memory>

#include <QMainWindow>
#include <QGLWidget>
#include <GLMHelpers.h>
#include <GlWindow.h>
#include <QEvent>
#include <QResizeEvent>
#include <QOpenGLContext>

#include <OVR_CAPI_GL.h>

#include <PerfStat.h>

#include "plugins/PluginContainer.h"

#include "OculusHelpers.h"
#include <oglplus/opt/list_init.hpp>
#include <oglplus/shapes/vector.hpp>
#include <oglplus/opt/list_init.hpp>
#include <oglplus/shapes/obj_mesh.hpp>

#include "../OglplusHelpers.h"

const QString Oculus_0_5_DisplayPlugin::NAME("Oculus Rift (0.5)");

const QString & Oculus_0_5_DisplayPlugin::getName() const {
    return NAME;
}

bool Oculus_0_5_DisplayPlugin::isSupported() const {
    if (!ovr_Initialize(nullptr)) {
        return false;
    }
    bool result = false;
    if (ovrHmd_Detect() > 0) {
        result = true;
    }
    ovr_Shutdown();
    return result;
}

void Oculus_0_5_DisplayPlugin::activate(PluginContainer * container) {
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        Q_ASSERT(false);
        qFatal("Failed to Initialize SDK");
    }
    _hmd = ovrHmd_Create(0);
    if (!_hmd) {
        qFatal("Failed to acquire HMD");
    }

    OculusBaseDisplayPlugin::activate(container);

    _window->makeCurrent();
    _hmdWindow = new GlWindow(QOpenGLContext::currentContext());

    QSurfaceFormat format;
    initSurfaceFormat(format);
    _hmdWindow->setFormat(format);
    _hmdWindow->create();
    _hmdWindow->installEventFilter(this);
    _hmdWindow->makeCurrent();
}

void Oculus_0_5_DisplayPlugin::deactivate() {
    _hmdWindow->deleteLater();
    _hmdWindow = nullptr;

    OculusBaseDisplayPlugin::deactivate();

    ovrHmd_Destroy(_hmd);
    _hmd = nullptr;
    ovr_Shutdown();
}

void Oculus_0_5_DisplayPlugin::display(GLuint finalTexture, const glm::uvec2& sceneSize) {
    ++_frameIndex;
}

// Pass input events on to the application
bool Oculus_0_5_DisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
    return OculusBaseDisplayPlugin::eventFilter(receiver, event);
}

/*
    The swapbuffer call here is only required if we want to mirror the content to the screen.
    However, it should only be done if we can reliably disable v-sync on the mirror surface, 
    otherwise the swapbuffer delay will interefere with the framerate of the headset
*/
void Oculus_0_5_DisplayPlugin::finishFrame() {
    swapBuffers();
    doneCurrent();
};


#if 0
/*
An alternative way to render the UI is to pass it specifically as a composition layer to
the Oculus SDK which should technically result in higher quality.  However, the SDK doesn't
have a mechanism to present the image as a sphere section, which is our desired look.
*/
ovrLayerQuad& uiLayer = getUiLayer();
if (nullptr == uiLayer.ColorTexture || overlaySize != _uiFbo->size) {
    _uiFbo->Resize(overlaySize);
    uiLayer.ColorTexture = _uiFbo->color;
    uiLayer.Viewport.Size.w = overlaySize.x;
    uiLayer.Viewport.Size.h = overlaySize.y;
    float overlayAspect = aspect(overlaySize);
    uiLayer.QuadSize.x = 1.0f;
    uiLayer.QuadSize.y = 1.0f / overlayAspect;
}

_uiFbo->Bound([&] {
    Q_ASSERT(0 == glGetError());
    using namespace oglplus;
    Context::Viewport(_uiFbo->size.x, _uiFbo->size.y);
    glClearColor(0, 0, 0, 0);
    Context::Clear().ColorBuffer();

    _program->Bind();
    glBindTexture(GL_TEXTURE_2D, overlayTexture);
    _plane->Use();
    _plane->Draw();
    Q_ASSERT(0 == glGetError());
});
#endif    

#endif