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
void GLBackend::do_setModelTransform(Batch& batch, size_t paramOffset) {
}

void GLBackend::do_setViewTransform(Batch& batch, size_t paramOffset) {
    _transform._view = batch._transforms.get(batch._params[paramOffset]._uint);
    _transform._invalidView = true;
}

void GLBackend::do_setProjectionTransform(Batch& batch, size_t paramOffset) {
    memcpy(&_transform._projection, batch.editData(batch._params[paramOffset]._uint), sizeof(Mat4));
    _transform._invalidProj = true;
}

void GLBackend::do_setViewportTransform(Batch& batch, size_t paramOffset) {
    memcpy(&_transform._viewport, batch.editData(batch._params[paramOffset]._uint), sizeof(Vec4i));

    if (!_inRenderTransferPass && !isStereo()) {
        ivec4& vp = _transform._viewport;
        glViewport(vp.x, vp.y, vp.z, vp.w);
    }

    // The Viewport is tagged invalid because the CameraTransformUBO is not up to date and will need update on next drawcall
    _transform._invalidViewport = true;
}

void GLBackend::do_setDepthRangeTransform(Batch& batch, size_t paramOffset) {

    Vec2 depthRange(batch._params[paramOffset + 1]._float, batch._params[paramOffset + 0]._float);

    if ((depthRange.x != _transform._depthRange.x) || (depthRange.y != _transform._depthRange.y)) {
        _transform._depthRange = depthRange;
        
        glDepthRangef(depthRange.x, depthRange.y);
    }
}

void GLBackend::initTransform() {
    glGenBuffers(1, &_transform._objectBuffer);
    glGenBuffers(1, &_transform._cameraBuffer);
    glGenBuffers(1, &_transform._drawCallInfoBuffer);
#ifndef GPU_SSBO_DRAW_CALL_INFO
    glGenTextures(1, &_transform._objectBufferTexture);
#endif
    size_t cameraSize = sizeof(TransformStageState::CameraBufferElement);
    while (_transform._cameraUboSize < cameraSize) {
        _transform._cameraUboSize += _uboAlignment;
    }
}

void GLBackend::killTransform() {
    glDeleteBuffers(1, &_transform._objectBuffer);
    glDeleteBuffers(1, &_transform._cameraBuffer);
    glDeleteBuffers(1, &_transform._drawCallInfoBuffer);
#ifndef GPU_SSBO_DRAW_CALL_INFO
    glDeleteTextures(1, &_transform._objectBufferTexture);
#endif
}

void GLBackend::syncTransformStateCache() {
    _transform._invalidViewport = true;
    _transform._invalidProj = true;
    _transform._invalidView = true;

    glGetIntegerv(GL_VIEWPORT, (GLint*) &_transform._viewport);

    glGetFloatv(GL_DEPTH_RANGE, (GLfloat*)&_transform._depthRange);

    Mat4 modelView;
    auto modelViewInv = glm::inverse(modelView);
    _transform._view.evalFromRawMatrix(modelViewInv);

    glDisableVertexAttribArray(gpu::Stream::DRAW_CALL_INFO);
    _transform._enabledDrawcallInfoBuffer = false;
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
        // This is when the _view matrix gets assigned
        _view.getInverseMatrix(_camera._view);
    }

    if (_invalidView || _invalidProj || _invalidViewport) {
        size_t offset = _cameraUboSize * _cameras.size();
        _cameraOffsets.push_back(TransformStageState::Pair(commandIndex, offset));
        if (stereo._enable) {
            _cameras.push_back((_camera.getEyeCamera(0, stereo, _view)));
            _cameras.push_back((_camera.getEyeCamera(1, stereo, _view)));
        } else {
            _cameras.push_back((_camera.recomputeDerived(_view)));
        }

    }

    // Flags are clean
    _invalidView = _invalidProj = _invalidViewport = false;
}

void GLBackend::TransformStageState::transfer(const Batch& batch) const {
    // FIXME not thread safe
    static std::vector<uint8_t> bufferData;
    if (!_cameras.empty()) {
        bufferData.resize(_cameraUboSize * _cameras.size());
        for (size_t i = 0; i < _cameras.size(); ++i) {
            memcpy(bufferData.data() + (_cameraUboSize * i), &_cameras[i], sizeof(CameraBufferElement));
        }
        glBindBuffer(GL_UNIFORM_BUFFER, _cameraBuffer);
        glBufferData(GL_UNIFORM_BUFFER, bufferData.size(), bufferData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    if (!batch._objects.empty()) {
        auto byteSize = batch._objects.size() * sizeof(Batch::TransformObject);
        bufferData.resize(byteSize);
        memcpy(bufferData.data(), batch._objects.data(), byteSize);

#ifdef GPU_SSBO_DRAW_CALL_INFO
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _objectBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, bufferData.size(), bufferData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#else
        glBindBuffer(GL_TEXTURE_BUFFER, _objectBuffer);
        glBufferData(GL_TEXTURE_BUFFER, bufferData.size(), bufferData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
#endif
    }

    if (!batch._namedData.empty()) {
        bufferData.clear();
        for (auto& data : batch._namedData) {
            auto currentSize = bufferData.size();
            auto bytesToCopy = data.second.drawCallInfos.size() * sizeof(Batch::DrawCallInfo);
            bufferData.resize(currentSize + bytesToCopy);
            memcpy(bufferData.data() + currentSize, data.second.drawCallInfos.data(), bytesToCopy);
            _drawCallInfoOffsets[data.first] = (GLvoid*)currentSize;
        }

        glBindBuffer(GL_ARRAY_BUFFER, _drawCallInfoBuffer);
        glBufferData(GL_ARRAY_BUFFER, bufferData.size(), bufferData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

#ifdef GPU_SSBO_DRAW_CALL_INFO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, TRANSFORM_OBJECT_SLOT, _objectBuffer);
#else
    glActiveTexture(GL_TEXTURE0 + TRANSFORM_OBJECT_SLOT);
    glBindTexture(GL_TEXTURE_BUFFER, _objectBufferTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _objectBuffer);
#endif

    CHECK_GL_ERROR();

    // Make sure the current Camera offset is unknown before render Draw
    _currentCameraOffset = INVALID_OFFSET;
}

void GLBackend::TransformStageState::update(size_t commandIndex, const StereoState& stereo) const {
    size_t offset = INVALID_OFFSET;
    while ((_camerasItr != _cameraOffsets.end()) && (commandIndex >= (*_camerasItr).first)) {
        offset = (*_camerasItr).second;
        _currentCameraOffset = offset;
        ++_camerasItr;
    }

    if (offset != INVALID_OFFSET) {
        if (!stereo._enable) {
            bindCurrentCamera(0);
        }
    }
    (void)CHECK_GL_ERROR();
}

void GLBackend::TransformStageState::bindCurrentCamera(int eye) const {
    if (_currentCameraOffset != INVALID_OFFSET) {
        glBindBufferRange(GL_UNIFORM_BUFFER, TRANSFORM_CAMERA_SLOT, _cameraBuffer, _currentCameraOffset + eye * _cameraUboSize, sizeof(CameraBufferElement));
    }
}

void GLBackend::updateTransform(const Batch& batch) {
    _transform.update(_commandIndex, _stereo);

    auto& drawCallInfoBuffer = batch.getDrawCallInfoBuffer();
    if (batch._currentNamedCall.empty()) {
        auto& drawCallInfo = drawCallInfoBuffer[_currentDraw];
        if (_transform._enabledDrawcallInfoBuffer) {
            glDisableVertexAttribArray(gpu::Stream::DRAW_CALL_INFO); // Make sure attrib array is disabled
            _transform._enabledDrawcallInfoBuffer = false;
        }
        glVertexAttribI2i(gpu::Stream::DRAW_CALL_INFO, drawCallInfo.index, drawCallInfo.unused);
    } else {
#if defined(SUPPORT_VERTEX_ATTRIB_FORMAT)
        if (!_transform._enabledDrawcallInfoBuffer) {
            glEnableVertexAttribArray(gpu::Stream::DRAW_CALL_INFO); // Make sure attrib array is enabled
            glVertexAttribIFormat(gpu::Stream::DRAW_CALL_INFO, 2, GL_UNSIGNED_SHORT, 0);
            glVertexAttribBinding(gpu::Stream::DRAW_CALL_INFO, gpu::Stream::DRAW_CALL_INFO);
            glVertexBindingDivisor(gpu::Stream::DRAW_CALL_INFO, 1);
            _transform._enabledDrawcallInfoBuffer = true;
        }
        // NOTE: A stride of zero in BindVertexBuffer signifies that all elements are sourced from the same location,
        //       so we must provide a stride.
        //       This is in contrast to VertexAttrib*Pointer, where a zero signifies tightly-packed elements.
        glBindVertexBuffer(gpu::Stream::DRAW_CALL_INFO, _transform._drawCallInfoBuffer, (GLintptr)_transform._drawCallInfoOffsets[batch._currentNamedCall], 2 * sizeof(GLushort));
#else 
        if (!_transform._enabledDrawcallInfoBuffer) {
            glEnableVertexAttribArray(gpu::Stream::DRAW_CALL_INFO); // Make sure attrib array is enabled
            glBindBuffer(GL_ARRAY_BUFFER, _transform._drawCallInfoBuffer);
            glVertexAttribDivisor(gpu::Stream::DRAW_CALL_INFO, 1);
            _transform._enabledDrawcallInfoBuffer = true;
        }
        glVertexAttribIPointer(gpu::Stream::DRAW_CALL_INFO, 2, GL_UNSIGNED_SHORT, 0, _transform._drawCallInfoOffsets[batch._currentNamedCall]);
#endif
    }
    
    (void)CHECK_GL_ERROR();
}

void GLBackend::resetTransformStage() {
    glDisableVertexAttribArray(gpu::Stream::DRAW_CALL_INFO);
    _transform._enabledDrawcallInfoBuffer = false;
}
