//
//  GLBackendTransform.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackendShared.h"

#include "Format.h"

using namespace gpu;

// Transform Stage
void GLBackend::do_setModelTransform(Batch& batch, uint32 paramOffset) {
    _transform._model = batch._transforms.get(batch._params[paramOffset]._uint);
    _transform._invalidModel = true;
}

void GLBackend::do_setViewTransform(Batch& batch, uint32 paramOffset) {
    _transform._view = batch._transforms.get(batch._params[paramOffset]._uint);
    _transform._invalidView = true;
}

void GLBackend::do_setProjectionTransform(Batch& batch, uint32 paramOffset) {
    memcpy(&_transform._projection, batch.editData(batch._params[paramOffset]._uint), sizeof(Mat4));
    _transform._invalidProj = true;
}

void GLBackend::do_setViewportTransform(Batch& batch, uint32 paramOffset) {
    memcpy(&_transform._viewport, batch.editData(batch._params[paramOffset]._uint), sizeof(Vec4i));

    // Where we assign the GL viewport
    glViewport(_transform._viewport.x, _transform._viewport.y, _transform._viewport.z, _transform._viewport.w);

    // The Viewport is tagged invalid because the CameraTransformUBO is not up to date and willl need update on next drawcall
    _transform._invalidViewport = true;
}

void GLBackend::initTransform() {
 #if (GPU_TRANSFORM_PROFILE == GPU_CORE)
    glGenBuffers(1, &_transform._transformObjectBuffer);
    glGenBuffers(1, &_transform._transformCameraBuffer);

    glBindBuffer(GL_UNIFORM_BUFFER, _transform._transformObjectBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(_transform._transformObject), (const void*) &_transform._transformObject, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, _transform._transformCameraBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(_transform._transformCamera), (const void*) &_transform._transformCamera, GL_DYNAMIC_DRAW);


    glBindBuffer(GL_UNIFORM_BUFFER, 0);
#else
#endif
}

void GLBackend::killTransform() {
 #if (GPU_TRANSFORM_PROFILE == GPU_CORE)
    glDeleteBuffers(1, &_transform._transformObjectBuffer);
    glDeleteBuffers(1, &_transform._transformCameraBuffer);
#else
#endif
}

void GLBackend::syncTransformStateCache() {
    _transform._invalidViewport = true;
    _transform._invalidProj = true;
    _transform._invalidView = true;
    _transform._invalidModel = true;

    glGetIntegerv(GL_VIEWPORT, (GLint*) &_transform._viewport);

    GLint currentMode;
    glGetIntegerv(GL_MATRIX_MODE, &currentMode);
    _transform._lastMode = currentMode;

    glGetFloatv(GL_PROJECTION_MATRIX, (float*) &_transform._projection);

    Mat4 modelView;
    glGetFloatv(GL_MODELVIEW_MATRIX, (float*) &modelView);
    auto modelViewInv = glm::inverse(modelView);
    _transform._view.evalFromRawMatrix(modelViewInv);
    _transform._model.setIdentity();
}

void GLBackend::updateTransform() {
    // Check all the dirty flags and update the state accordingly
    if (_transform._invalidViewport) {
        _transform._transformCamera._viewport = glm::vec4(_transform._viewport);
    }

    if (_transform._invalidProj) {
        _transform._transformCamera._projection = _transform._projection;
        _transform._transformCamera._projectionInverse = glm::inverse(_transform._projection);
    }

    if (_transform._invalidView) {
        _transform._view.getInverseMatrix(_transform._transformCamera._view);
        _transform._view.getMatrix(_transform._transformCamera._viewInverse);
    }

    if (_transform._invalidModel) {
        _transform._model.getMatrix(_transform._transformObject._model);
        _transform._model.getInverseMatrix(_transform._transformObject._modelInverse);
    }

    if (_transform._invalidView || _transform._invalidProj) {
        Mat4 viewUntranslated = _transform._transformCamera._view;
        viewUntranslated[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        _transform._transformCamera._projectionViewUntranslated = _transform._transformCamera._projection * viewUntranslated;
    }
 
 #if (GPU_TRANSFORM_PROFILE == GPU_CORE)
    if (_transform._invalidView || _transform._invalidProj || _transform._invalidViewport) {
        glBindBufferBase(GL_UNIFORM_BUFFER, TRANSFORM_CAMERA_SLOT, 0);
        glBindBuffer(GL_ARRAY_BUFFER, _transform._transformCameraBuffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(_transform._transformCamera), (const void*)&_transform._transformCamera);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();
    }

    if (_transform._invalidModel) {
        glBindBufferBase(GL_UNIFORM_BUFFER, TRANSFORM_OBJECT_SLOT, 0);
        glBindBuffer(GL_ARRAY_BUFFER, _transform._transformObjectBuffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(_transform._transformObject), (const void*) &_transform._transformObject);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, TRANSFORM_OBJECT_SLOT, _transform._transformObjectBuffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, TRANSFORM_CAMERA_SLOT, _transform._transformCameraBuffer);
    CHECK_GL_ERROR();
#endif


#if (GPU_TRANSFORM_PROFILE == GPU_LEGACY)
    // Do it again for fixed pipeline until we can get rid of it
     GLint originalMatrixMode;
     glGetIntegerv(GL_MATRIX_MODE, &originalMatrixMode);
    
    if (_transform._invalidProj) {
        if (_transform._lastMode != GL_PROJECTION) {
            glMatrixMode(GL_PROJECTION);
            _transform._lastMode = GL_PROJECTION;
        }
        glLoadMatrixf(reinterpret_cast< const GLfloat* >(&_transform._projection));

        (void) CHECK_GL_ERROR();
    }


    if (_transform._invalidModel || _transform._invalidView) {
        if (_transform._lastMode != GL_MODELVIEW) {
            glMatrixMode(GL_MODELVIEW);
            _transform._lastMode = GL_MODELVIEW;
        }
        if (!_transform._model.isIdentity()) {
            Transform::Mat4 modelView;
            if (!_transform._view.isIdentity()) {
                Transform mvx;
                Transform::inverseMult(mvx, _transform._view, _transform._model);
                mvx.getMatrix(modelView);
            } else {
                _transform._model.getMatrix(modelView);
            }
            glLoadMatrixf(reinterpret_cast< const GLfloat* >(&modelView));
        } else if (!_transform._view.isIdentity()) {
            Transform::Mat4 modelView;
            _transform._view.getInverseMatrix(modelView);
            glLoadMatrixf(reinterpret_cast< const GLfloat* >(&modelView));
        } else {
            glLoadIdentity();
        }
        (void) CHECK_GL_ERROR();
    }

    glMatrixMode(originalMatrixMode);
#endif

    // Flags are clean
    _transform._invalidView = _transform._invalidProj = _transform._invalidModel = _transform._invalidViewport = false;
}


