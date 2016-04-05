//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OculusHelpers.h"

#include <atomic>

#include <QtCore/QLoggingCategory>
#include <QtCore/QFile>
#include <QtCore/QDir>

using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;

Q_DECLARE_LOGGING_CATEGORY(oculus)
Q_LOGGING_CATEGORY(oculus, "hifi.plugins.oculus")

static std::atomic<uint32_t> refCount { 0 };
static ovrSession session { nullptr };

inline ovrErrorInfo getError() {
    ovrErrorInfo error;
    ovr_GetLastErrorInfo(&error);
    return error;
}

void logWarning(const char* what) {
    qWarning(oculus) << what << ":" << getError().ErrorString;
}

void logFatal(const char* what) {
    std::string error("[oculus] ");
    error += what;
    error += ": ";
    error += getError().ErrorString;
    qFatal(error.c_str());
}

static const QString OCULUS_RUNTIME_PATH { "C:\\Program Files (x86)\\Oculus\\Support\\oculus-runtime" };
static const QString GOOD_OCULUS_RUNTIME_FILE { OCULUS_RUNTIME_PATH + "\\LibOVRRT64_1.dll" };

bool oculusAvailable() {
    ovrDetectResult detect = ovr_Detect(0);
    if (!detect.IsOculusServiceRunning || !detect.IsOculusHMDConnected) {
        return false;
    }

    // HACK Explicitly check for the presence of the 1.0 runtime DLL, and fail if it 
    // doesn't exist
    if (!QFile(GOOD_OCULUS_RUNTIME_FILE).exists()) {
        qCWarning(oculus) << "Oculus Runtime detected, but no 1.x DLL present: \"" + GOOD_OCULUS_RUNTIME_FILE + "\"";
        return false;
    }

    return true;
}

ovrSession acquireOculusSession() {
    if (!session && !oculusAvailable()) {
        qCDebug(oculus) << "oculus: no runtime or HMD present";
        return session;
    }

    if (!session) {
        ovrInitParams init = {};
        init.Flags = 0;
        init.ConnectionTimeoutMS = 0;
        init.LogCallback = nullptr;
        if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
            logWarning("Failed to initialize Oculus SDK");
            return session;
        }

        Q_ASSERT(0 == refCount);
        ovrGraphicsLuid luid;
        if (!OVR_SUCCESS(ovr_Create(&session, &luid))) {
            logWarning("Failed to acquire Oculus session");
            return session;
        }
    }

    ++refCount;
    return session;
}

void releaseOculusSession() {
    Q_ASSERT(refCount > 0 && session);
    // HACK the Oculus runtime doesn't seem to play well with repeated shutdown / restart.
    // So for now we'll just hold on to the session
#if 0
    if (!--refCount) {
        qCDebug(oculus) << "oculus: zero refcount, shutdown SDK and session";
        ovr_Destroy(session);
        ovr_Shutdown();
        session = nullptr;
    }
#endif
}


// A wrapper for constructing and using a swap texture set,
// where each frame you draw to a texture via the FBO,
// then submit it and increment to the next texture.
// The Oculus SDK manages the creation and destruction of
// the textures

SwapFramebufferWrapper::SwapFramebufferWrapper(const ovrSession& session) 
    : _session(session) {
    color = nullptr;
    depth = nullptr;
}

SwapFramebufferWrapper::~SwapFramebufferWrapper() {
    destroyColor();
}

void SwapFramebufferWrapper::Commit() {
    auto result = ovr_CommitTextureSwapChain(_session, color);
    Q_ASSERT(OVR_SUCCESS(result));
}

void SwapFramebufferWrapper::Resize(const uvec2 & size) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    this->size = size;
    initColor();
    initDone();
}

void SwapFramebufferWrapper::destroyColor() {
    if (color) {
        ovr_DestroyTextureSwapChain(_session, color);
        color = nullptr;
    }
}

void SwapFramebufferWrapper::initColor() {
    destroyColor();

    ovrTextureSwapChainDesc desc = {};
    desc.Type = ovrTexture_2D;
    desc.ArraySize = 1;
    desc.Width = size.x;
    desc.Height = size.y;
    desc.MipLevels = 1;
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.SampleCount = 1;
    desc.StaticImage = ovrFalse;

    ovrResult result = ovr_CreateTextureSwapChainGL(_session, &desc, &color);
    if (!OVR_SUCCESS(result)) {
        logFatal("Failed to create swap textures");
    }

    int length = 0;
    result = ovr_GetTextureSwapChainLength(_session, color, &length);
    if (!OVR_SUCCESS(result) || !length) {
        qFatal("Unable to count swap chain textures");
    }
    for (int i = 0; i < length; ++i) {
        GLuint chainTexId;
        ovr_GetTextureSwapChainBufferGL(_session, color, i, &chainTexId);
        glBindTexture(GL_TEXTURE_2D, chainTexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void SwapFramebufferWrapper::initDone() {
}

void SwapFramebufferWrapper::onBind(oglplus::Framebuffer::Target target) {
    int curIndex;
    ovr_GetTextureSwapChainCurrentIndex(_session, color, &curIndex);
    GLuint curTexId;
    ovr_GetTextureSwapChainBufferGL(_session, color, curIndex, &curTexId);
    glFramebufferTexture2D(toEnum(target), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
}

void SwapFramebufferWrapper::onUnbind(oglplus::Framebuffer::Target target) {
    glFramebufferTexture2D(toEnum(target), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
}
