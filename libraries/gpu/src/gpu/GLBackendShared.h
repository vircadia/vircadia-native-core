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

static const GLenum _primitiveToGLmode[gpu::NUM_PRIMITIVES] = {
    GL_POINTS,
    GL_LINES,
    GL_LINE_STRIP,
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
    GL_QUADS,
    GL_QUAD_STRIP,
};

static const GLenum _elementTypeToGLType[gpu::NUM_TYPES] = {
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

// Stupid preprocessor trick to turn the line macro into a string
#define CHECK_GL_ERROR_HELPER(x) #x
// FIXME doesn't build on Linux or Mac.  Hmmmm
// #define CHECK_GL_ERROR() gpu::GLBackend::checkGLErrorDebug(__FUNCTION__ ":" CHECK_GL_ERROR_HELPER(__LINE__))
#define CHECK_GL_ERROR() gpu::GLBackend::checkGLErrorDebug(__FUNCTION__)

#endif
