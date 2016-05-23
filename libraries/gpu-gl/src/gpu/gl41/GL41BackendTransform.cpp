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
        auto byteSize = batch._objects.size() * sizeof(Batch::TransformObject);
        bufferData.resize(byteSize);
        memcpy(bufferData.data(), batch._objects.data(), byteSize);

#ifdef GPU_SSBO_DRAW_CALL_INFO
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _transform._objectBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, bufferData.size(), bufferData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#else
        glBindBuffer(GL_TEXTURE_BUFFER, _transform._objectBuffer);
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
            _transform._drawCallInfoOffsets[data.first] = (GLvoid*)currentSize;
        }

        glBindBuffer(GL_ARRAY_BUFFER, _transform._drawCallInfoBuffer);
        glBufferData(GL_ARRAY_BUFFER, bufferData.size(), bufferData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

#ifdef GPU_SSBO_DRAW_CALL_INFO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, TRANSFORM_OBJECT_SLOT, _transform._objectBuffer);
#else
    glActiveTexture(GL_TEXTURE0 + TRANSFORM_OBJECT_SLOT);
    glBindTexture(GL_TEXTURE_BUFFER, _transform._objectBufferTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _transform._objectBuffer);
#endif

    CHECK_GL_ERROR();

    // Make sure the current Camera offset is unknown before render Draw
    _transform._currentCameraOffset = INVALID_OFFSET;
}
