//
//  GL45BackendQuery.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 7/7/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"

#include "../gl/GLQuery.h"

namespace gpu { namespace gl45 { 

class GL45Query : public gpu::gl::GLQuery {
    using Parent = gpu::gl::GLQuery;
public:
    static GLuint allocateQuery() {
        GLuint result;
        glCreateQueries(GL_TIMESTAMP, 1, &result);
        return result;
    }

    GL45Query(const Query& query)
        : Parent(query, allocateQuery(), allocateQuery()){}
};

gl::GLQuery* GL45Backend::syncGPUObject(const Query& query) {
    return GL45Query::sync<GL45Query>(query);
}

GLuint GL45Backend::getQueryID(const QueryPointer& query) {
    return GL45Query::getId<GL45Query>(query);
}

} }