//
//  GL45BackendInput.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"

using namespace gpu;
using namespace gpu::gl45;

void GL45Backend::updateInput() {
    if (_input._invalidFormat) {

        InputStageState::ActivationCache newActivation;

        // Assign the vertex format required
        if (_input._format) {
            for (auto& it : _input._format->getAttributes()) {
                const Stream::Attribute& attrib = (it).second;

                GLuint slot = attrib._slot;
                GLuint count = attrib._element.getLocationScalarCount();
                uint8_t locationCount = attrib._element.getLocationCount();
                GLenum type = gl::ELEMENT_TYPE_TO_GL[attrib._element.getType()];
                GLuint offset = (GLuint)attrib._offset;;
                GLboolean isNormalized = attrib._element.isNormalized();

                GLenum perLocationSize = attrib._element.getLocationSize();

                for (uint8_t locNum = 0; locNum < locationCount; ++locNum) {
                    newActivation.set(slot + locNum);
                    glVertexAttribFormat(slot + locNum, count, type, isNormalized, offset + locNum * perLocationSize);
                    glVertexAttribBinding(slot + locNum, attrib._channel);
                }
                glVertexBindingDivisor(attrib._channel, attrib._frequency);
            }
            (void) CHECK_GL_ERROR();
        }

        // Manage Activation what was and what is expected now
        for (uint32_t i = 0; i < newActivation.size(); i++) {
            bool newState = newActivation[i];
            if (newState != _input._attributeActivation[i]) {
                if (newState) {
                    glEnableVertexAttribArray(i);
                } else {
                    glDisableVertexAttribArray(i);
                }
                _input._attributeActivation.flip(i);
            }
        }
        (void) CHECK_GL_ERROR();

        _input._invalidFormat = false;
        _stats._ISNumFormatChanges++;
    }

    if (_input._invalidBuffers.any()) {
        auto numBuffers = _input._buffers.size();
        auto buffer = _input._buffers.data();
        auto vbo = _input._bufferVBOs.data();
        auto offset = _input._bufferOffsets.data();
        auto stride = _input._bufferStrides.data();

        for (uint32_t bufferNum = 0; bufferNum < numBuffers; bufferNum++) {
            if (_input._invalidBuffers.test(bufferNum)) {
                glBindVertexBuffer(bufferNum, (*vbo), (GLuint)(*offset), (GLuint)(*stride));
            }
            buffer++;
            vbo++;
            offset++;
            stride++;
        }
        _input._invalidBuffers.reset();
        (void) CHECK_GL_ERROR();
    }
}

