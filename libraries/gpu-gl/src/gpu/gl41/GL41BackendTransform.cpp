//
//  GL41BackendTransform.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL41Backend.h"

using namespace gpu;
using namespace gpu::gl41;

void GL41Backend::initTransform() {
    glGenBuffers(1, &_transform._objectBuffer);
    glGenBuffers(1, &_transform._cameraBuffer);
    glGenBuffers(1, &_transform._drawCallInfoBuffer);
    glGenTextures(1, &_transform._objectBufferTexture);
    size_t cameraSize = sizeof(TransformStageState::CameraBufferElement);
    while (_transform._cameraUboSize < cameraSize) {
        _transform._cameraUboSize += _uboAlignment;
    }
}

void GL41Backend::transferTransformState(const Batch& batch) const {
    // FIXME not thread safe
    static std::vector<uint8_t> bufferData;
    if (!_transform._cameras.empty()) {
        bufferData.resize(_transform._cameraUboSize * _transform._cameras.size());
        for (size_t i = 0; i < _transform._cameras.size(); ++i) {
            memcpy(bufferData.data() + (_transform._cameraUboSize * i), &_transform._cameras[i], sizeof(TransformStageState::CameraBufferElement));
        }
        glBindBuffer(GL_UNIFORM_BUFFER, _transform._cameraBuffer);
        glBufferData(GL_UNIFORM_BUFFER, bufferData.size(), bufferData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    if (!batch._objects.empty()) {
        glBindBuffer(GL_TEXTURE_BUFFER, _transform._objectBuffer);
        glBufferData(GL_TEXTURE_BUFFER, batch._objects.size() * sizeof(Batch::TransformObject), batch._objects.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }

    if (!batch._namedData.empty()) {
        bufferData.clear();
        for (auto& data : batch._namedData) {
            auto currentSize = bufferData.size();
            auto bytesToCopy = data.second.drawCallInfos.size() * sizeof(Batch::DrawCallInfo);
            bufferData.resize(currentSize + bytesToCopy);
            memcpy(bufferData.data() + currentSize, data.second.drawCallInfos.data(), bytesToCopy);
            _transform._drawCallInfoOffsets[data.first] = (GLvoid*)currentSize;
        }

        glBindBuffer(GL_ARRAY_BUFFER, _transform._drawCallInfoBuffer);
        glBufferData(GL_ARRAY_BUFFER, bufferData.size(), bufferData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    glActiveTexture(GL_TEXTURE0 +  GL41Backend::TRANSFORM_OBJECT_SLOT);
    glBindTexture(GL_TEXTURE_BUFFER, _transform._objectBufferTexture);
    if (!batch._objects.empty()) {
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _transform._objectBuffer);
    }

    CHECK_GL_ERROR();

    // Make sure the current Camera offset is unknown before render Draw
    _transform._currentCameraOffset = INVALID_OFFSET;
}


void GL41Backend::updateTransform(const Batch& batch) {
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
        if (!_transform._enabledDrawcallInfoBuffer) {
            glEnableVertexAttribArray(gpu::Stream::DRAW_CALL_INFO); // Make sure attrib array is enabled
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
            glVertexAttribDivisor(gpu::Stream::DRAW_CALL_INFO, (isStereo() ? 2 : 1));
#else
            glVertexAttribDivisor(gpu::Stream::DRAW_CALL_INFO, 1);
#endif
            _transform._enabledDrawcallInfoBuffer = true;
        }
        glBindBuffer(GL_ARRAY_BUFFER, _transform._drawCallInfoBuffer);
        glVertexAttribIPointer(gpu::Stream::DRAW_CALL_INFO, 2, GL_UNSIGNED_SHORT, 0, _transform._drawCallInfoOffsets[batch._currentNamedCall]);
    }

    (void)CHECK_GL_ERROR();
}