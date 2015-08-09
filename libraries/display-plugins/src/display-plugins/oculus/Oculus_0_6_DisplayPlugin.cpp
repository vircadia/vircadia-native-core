//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Oculus_0_6_DisplayPlugin.h"

#include <memory>

#include <QMainWindow>
#include <QGLWidget>
#include <GLMHelpers.h>
#include <GlWindow.h>
#include <QEvent>
#include <QResizeEvent>

#include <OVR_CAPI_GL.h>


#include <OglplusHelpers.h>
#include <oglplus/opt/list_init.hpp>
#include <oglplus/shapes/vector.hpp>
#include <oglplus/opt/list_init.hpp>
#include <oglplus/shapes/obj_mesh.hpp>

#include <PerfStat.h>
#include <plugins/PluginContainer.h>

#include "OculusHelpers.h"

using namespace Oculus;
#if (OVR_MAJOR_VERSION == 6)
SwapFboPtr          _sceneFbo;
MirrorFboPtr        _mirrorFbo;
ovrLayerEyeFov      _sceneLayer;
#endif

// A base class for FBO wrappers that need to use the Oculus C
// API to manage textures via ovrHmd_CreateSwapTextureSetGL,
// ovrHmd_CreateMirrorTextureGL, etc
template <typename C>
struct RiftFramebufferWrapper : public FramebufferWrapper<C, char> {
    ovrHmd hmd;
    RiftFramebufferWrapper(const ovrHmd & hmd) : hmd(hmd) {
        color = 0;
        depth = 0;
    };

    void Resize(const uvec2 & size) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        this->size = size;
        initColor();
        initDone();
    }

protected:
    virtual void initDepth() override final {
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

    virtual void onBind(oglplus::Framebuffer::Target target) override {
        ovrGLTexture& tex = (ovrGLTexture&)(color->Textures[color->CurrentIndex]);
        glFramebufferTexture2D(toEnum(target), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.OGL.TexId, 0);
    }

    virtual void onUnbind(oglplus::Framebuffer::Target target) override {
        glFramebufferTexture2D(toEnum(target), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    }
};


// We use a FBO to wrap the mirror texture because it makes it easier to
// render to the screen via glBlitFramebuffer
struct MirrorFramebufferWrapper : public RiftFramebufferWrapper<ovrGLTexture*> {
    MirrorFramebufferWrapper(const ovrHmd & hmd)
        : RiftFramebufferWrapper(hmd) { }

    virtual ~MirrorFramebufferWrapper() {
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

const QString Oculus_0_6_DisplayPlugin::NAME("Oculus Rift");

const QString & Oculus_0_6_DisplayPlugin::getName() const {
    return NAME;
}

bool Oculus_0_6_DisplayPlugin::isSupported() const {
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

ovrLayerEyeFov& getSceneLayer() {
    return _sceneLayer;
}

//static gpu::TexturePointer _texture;

void Oculus_0_6_DisplayPlugin::activate() {
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        Q_ASSERT(false);
        qFatal("Failed to Initialize SDK");
    }
    if (!OVR_SUCCESS(ovrHmd_Create(0, &_hmd))) {
        Q_ASSERT(false);
        qFatal("Failed to acquire HMD");
    }

    OculusBaseDisplayPlugin::activate();

    // Parent class relies on our _hmd intialization, so it must come after that.
    ovrLayerEyeFov& sceneLayer = getSceneLayer();
    memset(&sceneLayer, 0, sizeof(ovrLayerEyeFov));
    sceneLayer.Header.Type = ovrLayerType_EyeFov;
    sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    ovr_for_each_eye([&](ovrEyeType eye) {
        ovrFovPort & fov = sceneLayer.Fov[eye] = _eyeRenderDescs[eye].Fov;
        ovrSizei & size = sceneLayer.Viewport[eye].Size = ovrHmd_GetFovTextureSize(_hmd, eye, fov, 1.0f);
        sceneLayer.Viewport[eye].Pos = { eye == ovrEye_Left ? 0 : size.w, 0 };
    });
    // We're rendering both eyes to the same texture, so only one of the 
    // pointers is populated
    sceneLayer.ColorTexture[0] = _sceneFbo->color;
    // not needed since the structure was zeroed on init, but explicit
    sceneLayer.ColorTexture[1] = nullptr;

    PerformanceTimer::setActive(true);

    if (!OVR_SUCCESS(ovrHmd_ConfigureTracking(_hmd,
        ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0))) {
        qFatal("Could not attach to sensor device");
    }

}

void Oculus_0_6_DisplayPlugin::customizeContext() {
    OculusBaseDisplayPlugin::customizeContext();
    
    //_texture = DependencyManager::get<TextureCache>()->
    //    getImageTexture(PathUtils::resourcesPath() + "/images/cube_texture.png");
    uvec2 mirrorSize = toGlm(_window->geometry().size());
    _mirrorFbo = MirrorFboPtr(new MirrorFramebufferWrapper(_hmd));
    _mirrorFbo->Init(mirrorSize);

    _sceneFbo = SwapFboPtr(new SwapFramebufferWrapper(_hmd));
    _sceneFbo->Init(getRecommendedRenderSize());
}

void Oculus_0_6_DisplayPlugin::deactivate() {
    makeCurrent();
    _sceneFbo.reset();
    _mirrorFbo.reset();
    doneCurrent();
    PerformanceTimer::setActive(false);

    OculusBaseDisplayPlugin::deactivate();

    ovrHmd_Destroy(_hmd);
    _hmd = nullptr;
    ovr_Shutdown();
}

void Oculus_0_6_DisplayPlugin::display(GLuint finalTexture, const glm::uvec2& sceneSize) {
    using namespace oglplus;
    // Need to make sure only the display plugin is responsible for 
    // controlling vsync
    wglSwapIntervalEXT(0);

    _sceneFbo->Bound([&] {
        auto size = _sceneFbo->size;
        Context::Viewport(size.x, size.y);
        glBindTexture(GL_TEXTURE_2D, finalTexture);
        drawUnitQuad();
    });

    ovrLayerEyeFov& sceneLayer = getSceneLayer(); 
    ovr_for_each_eye([&](ovrEyeType eye) {
        sceneLayer.RenderPose[eye] = _eyePoses[eye];
    });

    auto windowSize = toGlm(_window->size());

    /* 
       Two alternatives for mirroring to the screen, the first is to copy our own composited
       scene to the window framebuffer, before distortion.  Note this only works if we're doing
       ui compositing ourselves, and not relying on the Oculus SDK compositor (or we don't want 
       the UI visible in the output window (unlikely).  This should be done before 
       _sceneFbo->Increment or we're be using the wrong texture
    */
    //_sceneFbo->Bound(GL_READ_FRAMEBUFFER, [&] {
    //    glBlitFramebuffer(
    //        0, 0, _sceneFbo->size.x, _sceneFbo->size.y,
    //        0, 0, windowSize.x, _mirrorFbo.y,
    //        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    //});

    {
        PerformanceTimer("OculusSubmit");
        ovrLayerHeader* layers = &sceneLayer.Header;
        ovrResult result = ovrHmd_SubmitFrame(_hmd, _frameIndex, nullptr, &layers, 1);
    }
    _sceneFbo->Increment();

    /* 
        The other alternative for mirroring is to use the Oculus mirror texture support, which 
        will contain the post-distorted and fully composited scene regardless of how many layers 
        we send.
    */
    auto mirrorSize = _mirrorFbo->size;
    _mirrorFbo->Bound(Framebuffer::Target::Read, [&] {
        Context::BlitFramebuffer(
            0, mirrorSize.y, mirrorSize.x, 0,
            0, 0, windowSize.x, windowSize.y,
            BufferSelectBit::ColorBuffer, BlitFilter::Nearest);
    });

    ++_frameIndex;
}

// Pass input events on to the application
bool Oculus_0_6_DisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
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

/*
    The swapbuffer call here is only required if we want to mirror the content to the screen.
    However, it should only be done if we can reliably disable v-sync on the mirror surface, 
    otherwise the swapbuffer delay will interefere with the framerate of the headset
*/
void Oculus_0_6_DisplayPlugin::finishFrame() {
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
