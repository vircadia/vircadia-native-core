//
//  GLESBackendQuery.cpp
//  libraries/gpu-gl-android/src/gpu/gles
//
//  Created by Gabriel Calero & Cristian Duarte on 9/27/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLESBackend.h"

#include <gpu/gl/GLQuery.h>

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gles; 

class GLESQuery : public GLQuery {
    using Parent = GLQuery;
public:
    static GLuint allocateQuery() {
        GLuint result;
        glGenQueries(1, &result);
        return result;
    }

    GLESQuery(const std::weak_ptr<GLBackend>& backend, const Query& query)
        : Parent(backend, query, allocateQuery(), allocateQuery()) { }
};

GLQuery* GLESBackend::syncGPUObject(const Query& query) {
    return GLESQuery::sync<GLESQuery>(*this, query);
}

GLuint GLESBackend::getQueryID(const QueryPointer& query) {
    return GLESQuery::getId<GLESQuery>(*this, query);
}
