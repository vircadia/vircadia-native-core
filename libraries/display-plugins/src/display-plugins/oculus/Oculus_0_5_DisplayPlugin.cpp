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
#include <OglplusHelpers.h>

#include "plugins/PluginContainer.h"
#include "OculusHelpers.h"

using namespace oglplus;


#define RIFT_SDK_DISTORTION 0

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
    _hmdWindow->setGeometry(_hmd->WindowsPos.x, _hmd->WindowsPos.y, _hmd->Resolution.w, _hmd->Resolution.h);
    _hmdWindow->show();
    _hmdWindow->installEventFilter(this);
    _hmdWindow->makeCurrent();
    ovrRenderAPIConfig config; memset(&config, 0, sizeof(ovrRenderAPIConfig));
    config.Header.API = ovrRenderAPI_OpenGL;
    config.Header.BackBufferSize = _hmd->Resolution;
    config.Header.Multisample = 1;
    int distortionCaps = 0
    | ovrDistortionCap_Vignette
    | ovrDistortionCap_Overdrive
    | ovrDistortionCap_TimeWarp
    ;
#if RIFT_SDK_DISTORTION
    ovrBool result = ovrHmd_ConfigureRendering(_hmd, &config, 0, _eyeFovs, nullptr);
    Q_ASSERT(result);
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
#else
    ovr_for_each_eye([&](ovrEyeType eye) {
        ovrDistortionMesh meshData;
        ovrHmd_CreateDistortionMesh(_hmd, eye, _eyeFovs[eye], distortionCaps, &meshData);
        ovrHmd_DestroyDistortionMesh(&meshData);
    });

    
    _program = loadDefaultShader();
    auto attribs = _program->ActiveAttribs();
    for(size_t i = 0; i < attribs.Size(); ++i) {
        auto attrib = attribs.At(i);
        if (String("Position") == attrib.Name()) {
            _positionAttribute = attrib.Index();
        } else if (String("TexCoord") == attrib.Name()) {
            _texCoordAttribute = attrib.Index();
        }
        qDebug() << attrib.Name().c_str();
    }
    _vertexBuffer.reset(new oglplus::Buffer());
    _vertexBuffer->Bind(Buffer::Target::Array);
    _vertexBuffer->Data(Buffer::Target::Array, BufferData(PLANE_VERTICES));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    
#endif
    
    
    
}

void Oculus_0_5_DisplayPlugin::deactivate() {
    _hmdWindow->deleteLater();
    _hmdWindow = nullptr;

    OculusBaseDisplayPlugin::deactivate();

    ovrHmd_Destroy(_hmd);
    _hmd = nullptr;
    ovr_Shutdown();
}

void Oculus_0_5_DisplayPlugin::preRender() {
#if RIFT_SDK_DISTORTION
    ovrHmd_BeginFrame(_hmd, _frameIndex);
#else
    ovrHmd_BeginFrameTiming(_hmd, _frameIndex);
#endif
}

void Oculus_0_5_DisplayPlugin::preDisplay() {
    _hmdWindow->makeCurrent();

}

void Oculus_0_5_DisplayPlugin::display(GLuint finalTexture, const glm::uvec2& sceneSize) {
    ++_frameIndex;
//    OpenGLDisplayPlugin::display(finalTexture, sceneSize);

#if RIFT_SDK_DISTORTION
    ovr_for_each_eye([&](ovrEyeType eye) {
        reinterpret_cast<ovrGLTexture&>(_eyeTextures[eye]).OGL.TexId = finalTexture;
    });
    ovrHmd_EndFrame(_hmd, _eyePoses, _eyeTextures);
#else
#endif
    
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
    _hmdWindow->swapBuffers();
    _hmdWindow->doneCurrent();
};

#endif
