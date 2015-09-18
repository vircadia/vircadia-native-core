//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusDisplayPlugin.h"

#include <QGLWidget>

#include "OculusHelpers.h"

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

const QString & OculusDisplayPlugin::getName() const {
    return NAME;
}

void OculusDisplayPlugin::customizeContext() {
    WindowOpenGLDisplayPlugin::customizeContext();
#if (OVR_MAJOR_VERSION >= 6)
    _sceneFbo = SwapFboPtr(new SwapFramebufferWrapper(_hmd));
    _sceneFbo->Init(getRecommendedRenderSize());

    // We're rendering both eyes to the same texture, so only one of the 
    // pointers is populated
    _sceneLayer.ColorTexture[0] = _sceneFbo->color;
    // not needed since the structure was zeroed on init, but explicit
    _sceneLayer.ColorTexture[1] = nullptr;
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
#endif

    OculusBaseDisplayPlugin::deactivate();
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

    ovr_for_each_eye([&](ovrEyeType eye) {
        _sceneLayer.RenderPose[eye] = _eyePoses[eye];
    });

    auto windowSize = toGlm(_window->size());
    {
        ovrViewScaleDesc viewScaleDesc;
        viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
        viewScaleDesc.HmdToEyeViewOffset[0] = _eyeOffsets[0];
        viewScaleDesc.HmdToEyeViewOffset[1] = _eyeOffsets[1];

        ovrLayerHeader* layers = &_sceneLayer.Header;
        ovrResult result = ovr_SubmitFrame(_hmd, 0, &viewScaleDesc, &layers, 1);
        if (!OVR_SUCCESS(result)) {
            qDebug() << result;
        }
    }
    _sceneFbo->Increment();

    ++_frameIndex;
#endif
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
