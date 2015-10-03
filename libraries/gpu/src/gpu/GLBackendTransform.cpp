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

    ivec4& vp = _transform._viewport;

    // Where we assign the GL viewport
    if (_stereo._enable) {
        vp.z /= 2;
        if (_stereo._pass) {
            vp.x += vp.z;
        }
    } 

    glViewport(vp.x, vp.y, vp.z, vp.w);

    // The Viewport is tagged invalid because the CameraTransformUBO is not up to date and willl need update on next drawcall
    _transform._invalidViewport = true;
}

void GLBackend::initTransform() {
    glGenBuffers(1, &_transform._objectBuffer);
    glGenBuffers(1, &_transform._cameraBuffer);
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
    glDeleteBuffers(1, &_transform._objectBuffer);
    glDeleteBuffers(1, &_transform._cameraBuffer);
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

void GLBackend::TransformStageState::preUpdate(size_t commandIndex, const StereoState& stereo) {
    // Check all the dirty flags and update the state accordingly
    if (_invalidViewport) {
        _camera._viewport = glm::vec4(_viewport);
    }

    if (_invalidProj) {
        _camera._projection = _projection;
    }

    if (_invalidView) {
        _view.getInverseMatrix(_camera._view);
    }

    if (_invalidModel) {
        _model.getMatrix(_object._model);

        // FIXME - we don't want to be using glm::inverse() here but it fixes the flickering issue we are 
        // seeing with planky blocks in toybox. Our implementation of getInverseMatrix() is buggy in cases
        // of non-uniform scale. We need to fix that. In the mean time, glm::inverse() works.
        //_model.getInverseMatrix(_object._modelInverse);
        _object._modelInverse = glm::inverse(_object._model);
    }

    if (_invalidView || _invalidProj || _invalidViewport) {
        size_t offset = _cameraUboSize * _cameras.size();
        if (stereo._enable) {
            _cameraOffsets.push_back(TransformStageState::Pair(commandIndex, offset));
            for (int i = 0; i < 2; ++i) {
                _cameras.push_back(_camera.getEyeCamera(i, stereo));
            }
        } else {
            _cameraOffsets.push_back(TransformStageState::Pair(commandIndex, offset));
            _cameras.push_back(_camera.recomputeDerived());
        }
    }

    if (_invalidModel) {
        size_t offset = _objectUboSize * _objects.size();
        _objectOffsets.push_back(TransformStageState::Pair(commandIndex, offset));
        _objects.push_back(_object);
    }

    // Flags are clean
    _invalidView = _invalidProj = _invalidModel = _invalidViewport = false;
}

void GLBackend::TransformStageState::transfer() const {
    static QByteArray bufferData;
    if (!_cameras.empty()) {
        glBindBuffer(GL_UNIFORM_BUFFER, _cameraBuffer);
        bufferData.resize(_cameraUboSize * _cameras.size());
        for (size_t i = 0; i < _cameras.size(); ++i) {
            memcpy(bufferData.data() + (_cameraUboSize * i), &_cameras[i], sizeof(TransformCamera));
        }
        glBufferData(GL_UNIFORM_BUFFER, bufferData.size(), bufferData.data(), GL_DYNAMIC_DRAW);
    }

    if (!_objects.empty()) {
        glBindBuffer(GL_UNIFORM_BUFFER, _objectBuffer);
        bufferData.resize(_objectUboSize * _objects.size());
        for (size_t i = 0; i < _objects.size(); ++i) {
            memcpy(bufferData.data() + (_objectUboSize * i), &_objects[i], sizeof(TransformObject));
        }
        glBufferData(GL_UNIFORM_BUFFER, bufferData.size(), bufferData.data(), GL_DYNAMIC_DRAW);
    }

    if (!_cameras.empty() || !_objects.empty()) {
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    CHECK_GL_ERROR();
}

void GLBackend::TransformStageState::update(size_t commandIndex, const StereoState& stereo) const {
    int offset = -1;
    while ((_objectsItr != _objectOffsets.end()) && (commandIndex >= (*_objectsItr).first)) {
        offset = (*_objectsItr).second;
        ++_objectsItr;
    }
    if (offset >= 0) {
        glBindBufferRange(GL_UNIFORM_BUFFER, TRANSFORM_OBJECT_SLOT,
            _objectBuffer, offset, sizeof(Backend::TransformObject));
    }

    offset = -1;
    while ((_camerasItr != _cameraOffsets.end()) && (commandIndex >= (*_camerasItr).first)) {
        offset = (*_camerasItr).second;
        ++_camerasItr;
    }
    if (offset >= 0) {
        // We include both camera offsets for stereo
        if (stereo._enable && stereo._pass) {
            offset += _cameraUboSize;
        }
        glBindBufferRange(GL_UNIFORM_BUFFER, TRANSFORM_CAMERA_SLOT,
            _cameraBuffer, offset, sizeof(Backend::TransformCamera));
    }

    (void)CHECK_GL_ERROR();
}

void GLBackend::updateTransform() const {
    _transform.update(_commandIndex, _stereo);
}

void GLBackend::resetTransformStage() {
    
}
