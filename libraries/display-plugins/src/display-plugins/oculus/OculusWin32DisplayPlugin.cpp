//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusWin32DisplayPlugin.h"

#include <memory>

#include <QMainWindow>
#include <QGLWidget>
#include <GLMHelpers.h>
#include <GlWindow.h>
#include <QEvent>
#include <QResizeEvent>

#include <OVR_CAPI_GL.h>

#include "plugins/PluginContainer.h"

#include "OculusHelpers.h"

#include "../OglplusHelpers.h"

using oglplus::Framebuffer;
using oglplus::DefaultFramebuffer;

// A basic wrapper for constructing a framebuffer with a renderbuffer
// for the depth attachment and an undefined type for the color attachement
// This allows us to reuse the basic framebuffer code for both the Mirror
// FBO as well as the Oculus swap textures we will use to render the scene
// Though we don't really need depth at all for the mirror FBO, or even an
// FBO, but using one means I can just use a glBlitFramebuffer to get it onto
// the screen.
template <typename C = GLuint, typename D = GLuint>
struct FramebufferWrapper {
    uvec2       size;
    Framebuffer fbo;
    C           color{ 0 };
    D           depth{ 0 };

    virtual ~FramebufferWrapper() {
    }

    FramebufferWrapper() {}

    virtual void Init(const uvec2 & size) {
        this->size = size;
        initColor();
        initDepth();
        initDone();
    }

    template <typename F>
    void Bound(F f) {
        Bound(GL_FRAMEBUFFER, f);
    }

    template <typename F>
    void Bound(GLenum target, F f) {
        glBindFramebuffer(target, oglplus::GetName(fbo));
        onBind(target);
        f();
        onUnbind(target);
        glBindFramebuffer(target, 0);
    }

    void Viewport() {
        glViewport(0, 0, size.x, size.y);
    }

protected:
    virtual void onBind(GLenum target) {}
    virtual void onUnbind(GLenum target) {}


    virtual void initDepth() {
        glGenRenderbuffers(1, &depth);
        assert(depth);
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, size.x, size.y);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    virtual void initColor() = 0;
    virtual void initDone() = 0;
};



// A base class for FBO wrappers that need to use the Oculus C
// API to manage textures via ovrHmd_CreateSwapTextureSetGL,
// ovrHmd_CreateMirrorTextureGL, etc
template <typename C>
struct RiftFramebufferWrapper : public FramebufferWrapper<C> {
    ovrHmd hmd;
    RiftFramebufferWrapper(const ovrHmd & hmd) : hmd(hmd) {};

    void Resize(const uvec2 & size) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        this->size = size;
        initColor();
        initDone();
    }

protected:
    virtual void initDepth() override {
    }
};

// A wrapper for constructing and using a swap texture set,
// where each frame you draw to a texture via the FBO,
// then submit it and increment to the next texture.
// The Oculus SDK manages the creation and destruction of
// the textures
struct SwapFramebufferWrapper : public RiftFramebufferWrapper<ovrSwapTextureSet*> {
    SwapFramebufferWrapper(const ovrHmd & hmd)
        : RiftFramebufferWrapper(hmd) {
    }
    ~SwapFramebufferWrapper() {
        if (color) {
            ovrHmd_DestroySwapTextureSet(hmd, color);
            color = nullptr;
        }
    }

    void Increment() {
        ++color->CurrentIndex;
        color->CurrentIndex %= color->TextureCount;
    }

protected:
    virtual void initColor() override {
        if (color) {
            ovrHmd_DestroySwapTextureSet(hmd, color);
            color = nullptr;
        }

        if (!OVR_SUCCESS(ovrHmd_CreateSwapTextureSetGL(hmd, GL_RGBA, size.x, size.y, &color))) {
            qFatal("Unable to create swap textures");
        }

        for (int i = 0; i < color->TextureCount; ++i) {
            ovrGLTexture& ovrTex = (ovrGLTexture&)color->Textures[i];
            glBindTexture(GL_TEXTURE_2D, ovrTex.OGL.TexId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    virtual void initDone() override {
    }

    virtual void onBind(GLenum target) {
        ovrGLTexture& tex = (ovrGLTexture&)(color->Textures[color->CurrentIndex]);
        glFramebufferTexture2D(target, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.OGL.TexId, 0);
    }

    virtual void onUnbind(GLenum target) {
        glFramebufferTexture2D(target, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    }
};


// We use a FBO to wrap the mirror texture because it makes it easier to
// render to the screen via glBlitFramebuffer
struct MirrorFramebufferWrapper : public RiftFramebufferWrapper<ovrGLTexture*> {
    float                   targetAspect;
    MirrorFramebufferWrapper(const ovrHmd & hmd)
        : RiftFramebufferWrapper(hmd) {
    }
    ~MirrorFramebufferWrapper() {
        if (color) {
            ovrHmd_DestroyMirrorTexture(hmd, (ovrTexture*)color);
            color = nullptr;
        }
    }

private:
    void initColor() override {
        if (color) {
            ovrHmd_DestroyMirrorTexture(hmd, (ovrTexture*)color);
            color = nullptr;
        }
        ovrResult result = ovrHmd_CreateMirrorTextureGL(hmd, GL_RGBA, size.x, size.y, (ovrTexture**)&color);
        Q_ASSERT(OVR_SUCCESS(result));
    }

    void initDone() override {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color->OGL.TexId, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

};

const QString OculusWin32DisplayPlugin::NAME("Oculus Rift");

const QString & OculusWin32DisplayPlugin::getName() {
    return NAME;
}

bool OculusWin32DisplayPlugin::isSupported() {
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        return false;
    }
    bool result = false;
    if (ovrHmd_Detect() > 0) {
        result = true;
    }
    ovr_Shutdown();
    return result;
}

ovrLayerEyeFov& OculusWin32DisplayPlugin::getSceneLayer() {
    return _layers[0]->EyeFov;
}

ovrLayerQuad& OculusWin32DisplayPlugin::getUiLayer() {
    return _layers[1]->Quad;
}


void OculusWin32DisplayPlugin::activate(PluginContainer * container) {
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        Q_ASSERT(false);
        qFatal("Failed to Initialize SDK");
    }
    if (!OVR_SUCCESS(ovrHmd_Create(0, &_hmd))) {
        Q_ASSERT(false);
        qFatal("Failed to acquire HMD");
    }
    // Parent class relies on our _hmd intialization, so it must come after that.
    _layers.push_back(new ovrLayer_Union());
    _layers.push_back(new ovrLayer_Union());

    ovrLayerEyeFov& sceneLayer = getSceneLayer();
    memset(&sceneLayer, 0, sizeof(ovrLayerEyeFov));
    sceneLayer.Header.Type = ovrLayerType_EyeFov;
    sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    ovr_for_each_eye([&](ovrEyeType eye) {
        ovrFovPort & fov = sceneLayer.Fov[eye] = _eyeRenderDescs[eye].Fov;
        ovrSizei & size = sceneLayer.Viewport[eye].Size = ovrHmd_GetFovTextureSize(_hmd, eye, fov, 1.0f);
        sceneLayer.Viewport[eye].Pos = { eye == ovrEye_Left ? 0 : size.w, 0 };
    });

    ovrLayerQuad& uiLayer = getUiLayer();
    memset(&uiLayer, 0, sizeof(ovrLayerQuad));
    uiLayer.Header.Type = ovrLayerType_QuadInWorld;
    uiLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft | ovrLayerFlag_HighQuality;
    uiLayer.QuadPoseCenter.Orientation = { 0, 0, 0, 1 };
    uiLayer.QuadPoseCenter.Position = { 0, 0, -1 };

    OculusBaseDisplayPlugin::activate(container);
}

void OculusWin32DisplayPlugin::customizeContext(PluginContainer * container) {
    OculusBaseDisplayPlugin::customizeContext(container);
    wglSwapIntervalEXT(0);
    uvec2 mirrorSize = toGlm(_widget->geometry().size());
    _mirrorFbo = MirrorFboPtr(new MirrorFramebufferWrapper(_hmd));
    _mirrorFbo->Init(mirrorSize);
    _sceneFbo = SwapFboPtr(new SwapFramebufferWrapper(_hmd));
    uvec2 swapSize = toGlm(getRecommendedFramebufferSize());
    _sceneFbo->Init(swapSize);

    ovrLayerEyeFov& sceneLayer = getSceneLayer();
    sceneLayer.ColorTexture[0] = _sceneFbo->color;
    sceneLayer.ColorTexture[1] = nullptr;

    _uiFbo = SwapFboPtr(new SwapFramebufferWrapper(_hmd));
    _uiFbo->Init(getCanvasSize());
}

void OculusWin32DisplayPlugin::deactivate() {
    makeCurrent();
    _sceneFbo.reset();
    _uiFbo.reset();
    _mirrorFbo.reset();
//    doneCurrent();
    OculusBaseDisplayPlugin::deactivate();
    ovrHmd_Destroy(_hmd);
    _hmd = nullptr;
    ovr_Shutdown();
}

void OculusWin32DisplayPlugin::display(
    GLuint sceneTexture, const glm::uvec2& sceneSize,
    GLuint overlayTexture, const glm::uvec2& overlaySize) {
    using namespace oglplus;

    //Context::Viewport(_mirrorFbo->size.x, _mirrorFbo->size.y);
    //glClearColor(0, 0, 1, 1);
    //Context::Clear().ColorBuffer().DepthBuffer();
    //_program->Bind();
    //Mat4Uniform(*_program, "ModelView").Set(mat4());
    //Mat4Uniform(*_program, "Projection").Set(mat4());
    //glBindTexture(GL_TEXTURE_2D, sceneTexture);
    //_plane->Use();
    //_plane->Draw();

    _sceneFbo->Bound([&] {
        Q_ASSERT(0 == glGetError());
        using namespace oglplus;
        Context::Viewport(_sceneFbo->size.x, _sceneFbo->size.y);
        glClearColor(0, 0, 1, 1);
        Context::Clear().ColorBuffer();

        _program->Bind();
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        _plane->Use();
        _plane->Draw();
        Q_ASSERT(0 == glGetError());
    });

    ovrLayerQuad& uiLayer = _layers[1]->Quad;
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

    ovrLayerEyeFov& sceneLayer = _layers[0]->EyeFov;
    ovr_for_each_eye([&](ovrEyeType eye) {
        sceneLayer.RenderPose[eye] = _eyePoses[eye];
    });

    ovrLayerHeader* layers[2];
    layers[0] = &_layers[0]->Header;
    layers[1] = &_layers[1]->Header;

    ovrResult result = ovrHmd_SubmitFrame(_hmd, _frameIndex, nullptr, layers, 2);
    _sceneFbo->Increment();
    _uiFbo->Increment();
    /*
    _swapFbo->Bound(GL_READ_FRAMEBUFFER, [&] {
    glBlitFramebuffer(
    0, 0, _swapFbo->size.x, _swapFbo->size.y,
    0, 0, _mirrorFbo->size.x, _mirrorFbo->size.y,
    GL_COLOR_BUFFER_BIT, GL_NEAREST);
    });


    auto mirrorSize = toGlm(getDeviceSize());
    Context::Viewport(mirrorSize.x, mirrorSize.y);
    _mirrorFbo->Bound(GL_READ_FRAMEBUFFER, [&] {
        glBlitFramebuffer(
            0, _mirrorFbo->size.y, _mirrorFbo->size.x, 0,
            0, 0, _mirrorFbo->size.x, _mirrorFbo->size.y,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    });
    */
}


// Pressing Alt (and Meta) key alone activates the menubar because its style inherits the
// SHMenuBarAltKeyNavigation from QWindowsStyle. This makes it impossible for a scripts to
// receive keyPress events for the Alt (and Meta) key in a reliable manner.
//
// This filter catches events before QMenuBar can steal the keyboard focus.
// The idea was borrowed from
// http://www.archivum.info/qt-interest@trolltech.com/2006-09/00053/Re-(Qt4)-Alt-key-focus-QMenuBar-(solved).html

// Pass input events on to the application
bool OculusWin32DisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
    if (event->type() == QEvent::Resize) {
        QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
        qDebug() << resizeEvent->size().width() << " x " << resizeEvent->size().height();
        auto newSize = toGlm(resizeEvent->size());
        makeCurrent();
        _mirrorFbo->Resize(newSize);
        doneCurrent();
    }
    return OculusBaseDisplayPlugin::eventFilter(receiver, event);
}

void OculusWin32DisplayPlugin::swapBuffers() {
}

void OculusWin32DisplayPlugin::finishFrame() {
    doneCurrent();
};
