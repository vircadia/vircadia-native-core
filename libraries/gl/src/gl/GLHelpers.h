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

template<class F>
// https://bugreports.qt.io/browse/QTBUG-64703 prevents us from using "defined(QT_OPENGL_ES_3_1)"
#if defined(USE_GLES)
void setGLFormatVersion(F& format, int major = 3, int minor = 2)
#else
void setGLFormatVersion(F& format, int major = 4, int minor = 5) 
#endif
    { format.setVersion(major, minor);  }

size_t evalGLFormatSwapchainPixelSize(const QSurfaceFormat& format);

const QSurfaceFormat& getDefaultOpenGLSurfaceFormat();
QJsonObject getGLContextData();
int glVersionToInteger(QString glVersion);

bool isRenderThread();

namespace gl {
    void withSavedContext(const std::function<void()>& f);

    bool checkGLError(const char* name);

    bool checkGLErrorDebug(const char* name);

} // namespace gl

#define CHECK_GL_ERROR() ::gl::checkGLErrorDebug(__FUNCTION__)

#endif
