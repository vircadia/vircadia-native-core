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
    glGenBuffers(1, &_transform._transformObjectBuffer);
    glGenBuffers(1, &_transform._transformCameraBuffer);
    size_t cameraSize = sizeof(TransformCamera);
    while (_transform._cameraUboSize < cameraSize) {
        _transform._cameraUboSize += _uboAlignment;
    }
    size_t objectSize = sizeof(TransformObject);
    while (_transform._objectUboSize < objectSize) {
        _transform._objectUboSize += _uboAlignment;
    }
}

void GLBackend::killTransform() {
    glDeleteBuffers(1, &_transform._transformObjectBuffer);
    glDeleteBuffers(1, &_transform._transformCameraBuffer);
}

void GLBackend::syncTransformStateCache() {
    _transform._invalidViewport = true;
    _transform._invalidProj = true;
    _transform._invalidView = true;
    _transform._invalidModel = true;

    glGetIntegerv(GL_VIEWPORT, (GLint*) &_transform._viewport);

    Mat4 modelView;
    auto modelViewInv = glm::inverse(modelView);
    _transform._view.evalFromRawMatrix(modelViewInv);
    _transform._model.setIdentity();
}

void GLBackend::preUpdateTransform() {
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

    if (_transform._invalidView || _transform._invalidProj || _transform._invalidViewport) {
        _transform._cameraOffsets.push_back(TransformStageState::Pair(_commandIndex, _transform._cameraUboSize * _transform._cameraTransforms.size()));
        _transform._cameraTransforms.push_back(_transform._transformCamera);
    }

    if (_transform._invalidModel) {
        _transform._objectOffsets.push_back(TransformStageState::Pair(_commandIndex, _transform._objectUboSize * _transform._objectTransforms.size()));
        _transform._objectTransforms.push_back(_transform._transformObject);
    }

    // Flags are clean
    _transform._invalidView = _transform._invalidProj = _transform._invalidModel = _transform._invalidViewport = false;
}

void GLBackend::updateTransform() {
    int offset = -1;
    while (!_transform._objectOffsets.empty() && _commandIndex >= _transform._objectOffsets.front().first) {
        offset = _transform._objectOffsets.front().second;
        _transform._objectOffsets.pop_front();
    }
    if (offset >= 0) {
        glBindBufferRange(GL_UNIFORM_BUFFER, TRANSFORM_OBJECT_SLOT, 
            _transform._transformObjectBuffer,
            offset, sizeof(Backend::TransformObject));
    }

    offset = -1;
    while (!_transform._cameraOffsets.empty() && _commandIndex >= _transform._cameraOffsets.front().first) {
        offset = _transform._cameraOffsets.front().second;
        _transform._cameraOffsets.pop_front();
    }
    if (offset >= 0) {
        glBindBufferRange(GL_UNIFORM_BUFFER, TRANSFORM_CAMERA_SLOT, 
            _transform._transformCameraBuffer,
            offset, sizeof(Backend::TransformObject));
    }

    (void)CHECK_GL_ERROR();
}

void GLBackend::resetTransformStage() {
    
}
