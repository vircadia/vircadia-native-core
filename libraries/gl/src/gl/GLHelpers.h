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

#include <QJsonObject>

// 16 bits of depth precision
#define DEFAULT_GL_DEPTH_BUFFER_BITS 16
// 8 bits of stencil buffer (typically you really only need 1 bit for functionality
// but GL implementations usually just come with buffer sizes in multiples of 8)
#define DEFAULT_GL_STENCIL_BUFFER_BITS 8

class QSurfaceFormat;
class QGLFormat;

template<class F>
void setGLFormatVersion(F& format, int major = 4, int minor = 5) { format.setVersion(major, minor); }

const QSurfaceFormat& getDefaultOpenGLSurfaceFormat();
const QGLFormat& getDefaultGLFormat();
QJsonObject getGLContextData();
int glVersionToInteger(QString glVersion);

#endif
