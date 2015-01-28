//
//  GLBackendShared.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/22/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_GLBackend_Shared_h
#define hifi_gpu_GLBackend_Shared_h

#include "GLBackend.h"

#include <QDebug>

#include "Batch.h"

using namespace gpu;

static const GLenum _primitiveToGLmode[NUM_PRIMITIVES] = {
    GL_POINTS,
    GL_LINES,
    GL_LINE_STRIP,
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
    GL_QUADS,
    GL_QUAD_STRIP,
};

static const GLenum _elementTypeToGLType[NUM_TYPES]= {
    GL_FLOAT,
    GL_INT,
    GL_UNSIGNED_INT,
    GL_HALF_FLOAT,
    GL_SHORT,
    GL_UNSIGNED_SHORT,
    GL_BYTE,
    GL_UNSIGNED_BYTE,
    GL_FLOAT,
    GL_INT,
    GL_UNSIGNED_INT,
    GL_HALF_FLOAT,
    GL_SHORT,
    GL_UNSIGNED_SHORT,
    GL_BYTE,
    GL_UNSIGNED_BYTE
};

#define CHECK_GL_ERROR() ::gpu::GLBackend::checkGLError()
//#define CHECK_GL_ERROR()

#endif
