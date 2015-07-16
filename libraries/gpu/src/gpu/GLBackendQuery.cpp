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
#include "GPULogging.h"
#include "GLBackendShared.h"


using namespace gpu;

GLBackend::GLQuery::GLQuery() {}

GLBackend::GLQuery::~GLQuery() {
    if (_qo != 0) {
        glDeleteQueries(1, &_qo);
    }
}

GLBackend::GLQuery* GLBackend::syncGPUObject(const Query& query) {
    GLQuery* object = Backend::getGPUObject<GLBackend::GLQuery>(query);

    // If GPU object already created and in sync
    if (object) {
        return object;
    }

    // need to have a gpu object?
    if (!object) {
        GLuint qo;
        glGenQueries(1, &qo);
        (void) CHECK_GL_ERROR();
        GLuint64 result = -1;

        // All is green, assign the gpuobject to the Query
        object = new GLQuery();
        object->_qo = qo;
        object->_result = result;
        Backend::setGPUObject(query, object);
    }

    return object;
}



GLuint GLBackend::getQueryID(const QueryPointer& query) {
    if (!query) {
        return 0;
    }
    GLQuery* object = GLBackend::syncGPUObject(*query);
    if (object) {
        return object->_qo;
    } else {
        return 0;
    }
}

void GLBackend::do_beginQuery(Batch& batch, uint32 paramOffset) {
    auto query = batch._queries.get(batch._params[paramOffset]._uint);
    GLQuery* glquery = syncGPUObject(*query);
    if (glquery) {
        #if (GPU_FEATURE_PROFILE == GPU_LEGACY)
            // (EXT_TIMER_QUERY)
            glBeginQuery(GL_TIME_ELAPSED_EXT, glquery->_qo);
        #else
            glBeginQuery(GL_TIME_ELAPSED, glquery->_qo);
        #endif
        (void)CHECK_GL_ERROR();
    }
}

void GLBackend::do_endQuery(Batch& batch, uint32 paramOffset) {
    auto query = batch._queries.get(batch._params[paramOffset]._uint);
    GLQuery* glquery = syncGPUObject(*query);
    if (glquery) {
        #if (GPU_FEATURE_PROFILE == GPU_LEGACY)
            // (EXT_TIMER_QUERY)
            glEndQuery(GL_TIME_ELAPSED_EXT);
        #else
            glEndQuery(GL_TIME_ELAPSED);
        #endif
        (void)CHECK_GL_ERROR();
    }
}

void GLBackend::do_getQuery(Batch& batch, uint32 paramOffset) {
    auto query = batch._queries.get(batch._params[paramOffset]._uint);
    GLQuery* glquery = syncGPUObject(*query);
    if (glquery) { 
        #if (GPU_FEATURE_PROFILE == GPU_LEGACY)
            // (EXT_TIMER_QUERY)
            #if !defined(Q_OS_LINUX)
            glGetQueryObjectui64vEXT(glquery->_qo, GL_QUERY_RESULT, &glquery->_result);
            #endif
        #else
            glGetQueryObjectui64v(glquery->_qo, GL_QUERY_RESULT, &glquery->_result);
        #endif
        (void)CHECK_GL_ERROR();
    }
}
