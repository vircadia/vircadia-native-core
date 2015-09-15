//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusDisplayPlugin.h"

#include <memory>

#include <QMainWindow>
#include <QGLWidget>
#include <GLMHelpers.h>
#include <GlWindow.h>
#include <QEvent>
#include <QResizeEvent>
#include <QThread>

#include <OglplusHelpers.h>
#include <oglplus/opt/list_init.hpp>
#include <oglplus/shapes/vector.hpp>
#include <oglplus/opt/list_init.hpp>


#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

#include <oglplus/shapes/obj_mesh.hpp>

#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic pop
#endif


#include <PerfStat.h>
#include <plugins/PluginContainer.h>
#include <ViewFrustum.h>

#include "OculusHelpers.h"

#if (OVR_MAJOR_VERSION == 6) 
#define ovr_Create ovrHmd_Create
#define ovr_CreateSwapTextureSetGL ovrHmd_CreateSwapTextureSetGL
#define ovr_CreateMirrorTextureGL ovrHmd_CreateMirrorTextureGL 
#define ovr_Destroy ovrHmd_Destroy
#define ovr_DestroySwapTextureSet ovrHmd_DestroySwapTextureSet
#define ovr_DestroyMirrorTexture ovrHmd_DestroyMirrorTexture
#define ovr_GetFloat ovrHmd_GetFloat
#define ovr_GetFovTextureSize ovrHmd_GetFovTextureSize
#define ovr_GetFrameTiming ovrHmd_GetFrameTiming
#define ovr_GetTrackingState ovrHmd_GetTrackingState
#define ovr_GetRenderDesc ovrHmd_GetRenderDesc
#define ovr_RecenterPose ovrHmd_RecenterPose
#define ovr_SubmitFrame ovrHmd_SubmitFrame
#define ovr_ConfigureTracking ovrHmd_ConfigureTracking

#define ovr_GetHmdDesc(X) *X
#endif

#if (OVR_MAJOR_VERSION >= 6)

// A base class for FBO wrappers that need to use the Oculus C
// API to manage textures via ovr_CreateSwapTextureSetGL,
// ovr_CreateMirrorTextureGL, etc
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
            ovr_DestroySwapTextureSet(hmd, color);
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
            ovr_DestroySwapTextureSet(hmd, color);
            color = nullptr;
        }

        if (!OVR_SUCCESS(ovr_CreateSwapTextureSetGL(hmd, GL_RGBA, size.x, size.y, &color))) {
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
            ovr_DestroyMirrorTexture(hmd, (ovrTexture*)color);
            color = nullptr;
        }
    }

private:
    void initColor() override {
        if (color) {
            ovr_DestroyMirrorTexture(hmd, (ovrTexture*)color);
            color = nullptr;
        }
        ovrResult result = ovr_CreateMirrorTextureGL(hmd, GL_RGBA, size.x, size.y, (ovrTexture**)&color);
        Q_ASSERT(OVR_SUCCESS(result));
    }

    void initDone() override {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color->OGL.TexId, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
};


#endif

const QString OculusDisplayPlugin::NAME("Oculus Rift");

uvec2 OculusDisplayPlugin::getRecommendedRenderSize() const {
    return _desiredFramebufferSize;
}

void OculusDisplayPlugin::preRender() {
#if (OVR_MAJOR_VERSION >= 6)
    ovrFrameTiming ftiming = ovr_GetFrameTiming(_hmd, _frameIndex);
    _trackingState = ovr_GetTrackingState(_hmd, ftiming.DisplayMidpointSeconds);
    ovr_CalcEyePoses(_trackingState.HeadPose.ThePose, _eyeOffsets, _eyePoses);
#endif
}

glm::mat4 OculusDisplayPlugin::getProjection(Eye eye, const glm::mat4& baseProjection) const {
    return _eyeProjections[eye];
}

void OculusDisplayPlugin::resetSensors() {
#if (OVR_MAJOR_VERSION >= 6)
    ovr_RecenterPose(_hmd);
#endif
}

glm::mat4 OculusDisplayPlugin::getEyePose(Eye eye) const {
    return toGlm(_eyePoses[eye]);
}

glm::mat4 OculusDisplayPlugin::getHeadPose() const {
    return toGlm(_trackingState.HeadPose.ThePose);
}

const QString & OculusDisplayPlugin::getName() const {
    return NAME;
}

bool OculusDisplayPlugin::isSupported() const {
#if (OVR_MAJOR_VERSION >= 6)
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        return false;
    }
    bool result = false;
    if (ovrHmd_Detect() > 0) {
        result = true;
    }
    ovr_Shutdown();
    return result;
#else
    return false;
#endif
}

void OculusDisplayPlugin::init() {
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        qFatal("Could not init OVR");
    }
}

void OculusDisplayPlugin::deinit() {
    ovr_Shutdown();
}

#if (OVR_MAJOR_VERSION >= 6)
ovrLayerEyeFov& OculusDisplayPlugin::getSceneLayer() {
    return _sceneLayer;
}
#endif

//static gpu::TexturePointer _texture;

void OculusDisplayPlugin::activate() {
#if (OVR_MAJOR_VERSION >= 6)
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        Q_ASSERT(false);
        qFatal("Failed to Initialize SDK");
    }

//    CONTAINER->getPrimarySurface()->makeCurrent();
#if (OVR_MAJOR_VERSION == 6)
    if (!OVR_SUCCESS(ovr_Create(0, &_hmd))) {
#elif (OVR_MAJOR_VERSION == 7)
    if (!OVR_SUCCESS(ovr_Create(&_hmd, &_luid))) {
#endif
        Q_ASSERT(false);
        qFatal("Failed to acquire HMD");
    }

    _hmdDesc = ovr_GetHmdDesc(_hmd);

    _ipd = ovr_GetFloat(_hmd, OVR_KEY_IPD, _ipd);

    glm::uvec2 eyeSizes[2];
    ovr_for_each_eye([&](ovrEyeType eye) {
        _eyeFovs[eye] = _hmdDesc.DefaultEyeFov[eye];
        ovrEyeRenderDesc& erd = _eyeRenderDescs[eye] = ovr_GetRenderDesc(_hmd, eye, _eyeFovs[eye]);
        ovrMatrix4f ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded);
        _eyeProjections[eye] = toGlm(ovrPerspectiveProjection);

        ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, 0.001f, 10.0f, ovrProjection_RightHanded);
        _compositeEyeProjections[eye] = toGlm(ovrPerspectiveProjection);

        _eyeOffsets[eye] = erd.HmdToEyeViewOffset;
        eyeSizes[eye] = toGlm(ovr_GetFovTextureSize(_hmd, eye, erd.Fov, 1.0f));
    });
    ovrFovPort combined = _eyeFovs[Left];
    combined.LeftTan = std::max(_eyeFovs[Left].LeftTan, _eyeFovs[Right].LeftTan);
    combined.RightTan = std::max(_eyeFovs[Left].RightTan, _eyeFovs[Right].RightTan);
    ovrMatrix4f ovrPerspectiveProjection =
        ovrMatrix4f_Projection(combined, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded);
    _eyeProjections[Mono] = toGlm(ovrPerspectiveProjection);



    _desiredFramebufferSize = uvec2(
        eyeSizes[0].x + eyeSizes[1].x,
        std::max(eyeSizes[0].y, eyeSizes[1].y));

    _frameIndex = 0;

    if (!OVR_SUCCESS(ovr_ConfigureTracking(_hmd,
        ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0))) {
        qFatal("Could not attach to sensor device");
    }

    WindowOpenGLDisplayPlugin::activate();

    // Parent class relies on our _hmd intialization, so it must come after that.
    ovrLayerEyeFov& sceneLayer = getSceneLayer();
    memset(&sceneLayer, 0, sizeof(ovrLayerEyeFov));
    sceneLayer.Header.Type = ovrLayerType_EyeFov;
    sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    ovr_for_each_eye([&](ovrEyeType eye) {
        ovrFovPort & fov = sceneLayer.Fov[eye] = _eyeRenderDescs[eye].Fov;
        ovrSizei & size = sceneLayer.Viewport[eye].Size = ovr_GetFovTextureSize(_hmd, eye, fov, 1.0f);
        sceneLayer.Viewport[eye].Pos = { eye == ovrEye_Left ? 0 : size.w, 0 };
    });
    // We're rendering both eyes to the same texture, so only one of the 
    // pointers is populated
    sceneLayer.ColorTexture[0] = _sceneFbo->color;
    // not needed since the structure was zeroed on init, but explicit
    sceneLayer.ColorTexture[1] = nullptr;

    if (!OVR_SUCCESS(ovr_ConfigureTracking(_hmd,
        ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0))) {
        qFatal("Could not attach to sensor device");
    }
#endif
}

void OculusDisplayPlugin::customizeContext() {
    WindowOpenGLDisplayPlugin::customizeContext();
#if (OVR_MAJOR_VERSION >= 6)
    _sceneFbo = SwapFboPtr(new SwapFramebufferWrapper(_hmd));
    _sceneFbo->Init(getRecommendedRenderSize());
#endif
    enableVsync(false);
    // Only enable mirroring if we know vsync is disabled
    _enableMirror = !isVsyncEnabled();
}

void OculusDisplayPlugin::deactivate() {
#if (OVR_MAJOR_VERSION >= 6)
    makeCurrent();
    _sceneFbo.reset();
    doneCurrent();

    WindowOpenGLDisplayPlugin::deactivate();

    ovr_Destroy(_hmd);
    _hmd = nullptr;
    ovr_Shutdown();
#endif
}

void OculusDisplayPlugin::display(GLuint finalTexture, const glm::uvec2& sceneSize) {
#if (OVR_MAJOR_VERSION >= 6)
    using namespace oglplus;
    // Need to make sure only the display plugin is responsible for 
    // controlling vsync
    wglSwapIntervalEXT(0);

    // screen mirroring
    if (_enableMirror) {
        auto windowSize = toGlm(_window->size());
        Context::Viewport(windowSize.x, windowSize.y);
        glBindTexture(GL_TEXTURE_2D, finalTexture);
        GLenum err = glGetError();
        Q_ASSERT(0 == err);
        drawUnitQuad();
    }

    _sceneFbo->Bound([&] {
        auto size = _sceneFbo->size;
        Context::Viewport(size.x, size.y);
        glBindTexture(GL_TEXTURE_2D, finalTexture);
        GLenum err = glGetError();
        drawUnitQuad();
    });

    ovrLayerEyeFov& sceneLayer = getSceneLayer(); 
    ovr_for_each_eye([&](ovrEyeType eye) {
        sceneLayer.RenderPose[eye] = _eyePoses[eye];
    });

    auto windowSize = toGlm(_window->size());
    {
        ovrViewScaleDesc viewScaleDesc;
        viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
        viewScaleDesc.HmdToEyeViewOffset[0] = _eyeOffsets[0];
        viewScaleDesc.HmdToEyeViewOffset[1] = _eyeOffsets[1];

        ovrLayerHeader* layers = &sceneLayer.Header;
        ovrResult result = ovr_SubmitFrame(_hmd, 0, &viewScaleDesc, &layers, 1);
        if (!OVR_SUCCESS(result)) {
            qDebug() << result;
        }
    }
    _sceneFbo->Increment();

    ++_frameIndex;
#endif
}

// Pass input events on to the application
bool OculusDisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
    return WindowOpenGLDisplayPlugin::eventFilter(receiver, event);
}

/*
    The swapbuffer call here is only required if we want to mirror the content to the screen.
    However, it should only be done if we can reliably disable v-sync on the mirror surface, 
    otherwise the swapbuffer delay will interefere with the framerate of the headset
*/
void OculusDisplayPlugin::finishFrame() {
    if (_enableMirror) {
        swapBuffers();
    }
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
