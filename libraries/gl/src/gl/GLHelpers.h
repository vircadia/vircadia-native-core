//
//  Created by Bradley Austin Davis 2015/05/29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_GLHelpers_h
#define hifi_GLHelpers_h

#include <functional>
#include <QJsonObject>

#include "GLLogging.h"

// 16 bits of depth precision
#define DEFAULT_GL_DEPTH_BUFFER_BITS 16
// 8 bits of stencil buffer (typically you really only need 1 bit for functionality
// but GL implementations usually just come with buffer sizes in multiples of 8)
#define DEFAULT_GL_STENCIL_BUFFER_BITS 8

class QObject;
class QOpenGLDebugMessage;
class QSurfaceFormat;
class QGLFormat;

size_t evalGLFormatSwapchainPixelSize(const QSurfaceFormat& format);

const QSurfaceFormat& getDefaultOpenGLSurfaceFormat();
int glVersionToInteger(QString glVersion);

bool isRenderThread();

#define GL_MAKE_VERSION(major, minor) ((major << 8) | minor)
#define GL_GET_MINOR_VERSION(glversion) (glversion & 0x00FF)
#define GL_GET_MAJOR_VERSION(glversion) ((glversion & 0xFF00) >> 8)

namespace gl {
    struct ContextInfo {
        std::string version;
        std::string shadingLanguageVersion;
        std::string vendor;
        std::string renderer;
        std::vector<std::string> extensions;

        ContextInfo& init();
        operator bool() const { return !version.empty(); }

        static const ContextInfo& get(bool init = false);
    };

#define LOG_GL_CONTEXT_INFO(category, contextInfo) \
    qCDebug(category) << "GL Version: " << contextInfo.version.c_str(); \
    qCDebug(category) << "GL Shader Language Version: " << contextInfo.shadingLanguageVersion.c_str(); \
    qCDebug(category) << "GL Vendor: " << contextInfo.vendor.c_str(); \
    qCDebug(category) << "GL Renderer: " << contextInfo.renderer.c_str();


    void globalLock();
    void globalRelease(bool finish = true);

    bool debugContextEnabled();

    bool khrDebugEnabled();

    bool extDebugMarkerEnabled();

    void withSavedContext(const std::function<void()>& f);

    bool checkGLError(const char* name);

    bool checkGLErrorDebug(const char* name);

    bool disableGl45();
    void setDisableGl45(bool disable);

    uint16_t getTargetVersion();

    uint16_t getAvailableVersion();

    uint16_t getRequiredVersion();
} // namespace gl

#define CHECK_GL_ERROR() ::gl::checkGLErrorDebug(__FUNCTION__)

#endif
