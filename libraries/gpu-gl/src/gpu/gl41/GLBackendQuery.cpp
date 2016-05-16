//
//  GLBackendQuery.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 7/7/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"

#include "../gl/GLQuery.h"

using namespace gpu;
using namespace gpu::gl41; 

class GLQuery : public gpu::gl::GLQuery {
    using Parent = gpu::gl::GLBuffer;
public:
    static GLuint allocateQuery() {
        GLuint result;
        glGenQueries(1, &result);
        return result;
    }

    GLQuery(const Query& query) 
        : gl::GLQuery(query, allocateQuery()) { }
};

gl::GLQuery* GLBackend::syncGPUObject(const Query& query) {
    return GLQuery::sync<GLQuery>(query);
}

GLuint GLBackend::getQueryID(const QueryPointer& query) {
    return GLQuery::getId<GLQuery>(query);
}
