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
#include <gpu/gl/GLShared.h>

using namespace gpu;
using namespace gpu::gl45;

void GL45Backend::resetInputStage() {
    Parent::resetInputStage();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    for (uint32_t i = 0; i < _input._attributeActivation.size(); i++) {
        glDisableVertexAttribArray(i);
    }
    for (uint32_t i = 0; i < _input._attribBindingBuffers.size(); i++) {
        glBindVertexBuffer(i, 0, 0, 0);
    }
}

void GL45Backend::updateInput() {
    bool isStereoNow = isStereo();
    // track stereo state change potentially happening without changing the input format
    // this is a rare case requesting to invalid the format
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
    _input._invalidFormat |= (isStereoNow != _input._lastUpdateStereoState);
#endif
    _input._lastUpdateStereoState = isStereoNow;

    bool hasColorAttribute = _input._hasColorAttribute;

    if (_input._invalidFormat) {
        InputStageState::ActivationCache newActivation;

        // Assign the vertex format required
        auto format = acquire(_input._format);
        if (format) {
            _input._attribBindingBuffers.reset();

            const auto& attributes = format->getAttributes();
            const auto& inputChannels = format->getChannels();
            for (auto& channelIt : inputChannels) {
                auto bufferChannelNum = (channelIt).first;
                const Stream::Format::ChannelMap::value_type::second_type& channel = (channelIt).second;
                _input._attribBindingBuffers.set(bufferChannelNum);

                GLuint frequency = 0;
                for (unsigned int i = 0; i < channel._slots.size(); i++) {
                    const Stream::Attribute& attrib = attributes.at(channel._slots[i]);

                    GLuint slot = attrib._slot;
                    GLuint count = attrib._element.getLocationScalarCount();
                    uint8_t locationCount = attrib._element.getLocationCount();
                    GLenum type = gl::ELEMENT_TYPE_TO_GL[attrib._element.getType()];

                    GLuint offset = (GLuint)attrib._offset;
                    GLboolean isNormalized = attrib._element.isNormalized();

                    GLenum perLocationSize = attrib._element.getLocationSize();

                    hasColorAttribute |= slot == Stream::COLOR;

                    for (GLuint locNum = 0; locNum < locationCount; ++locNum) {
                        GLuint attriNum = (GLuint)(slot + locNum);
                        newActivation.set(attriNum);
                        if (!_input._attributeActivation[attriNum]) {
                            _input._attributeActivation.set(attriNum);
                            glEnableVertexAttribArray(attriNum);
                        }
                        if (attrib._element.isInteger()) {
                            glVertexAttribIFormat(attriNum, count, type, offset + locNum * perLocationSize);
                        } else {
                            glVertexAttribFormat(attriNum, count, type, isNormalized, offset + locNum * perLocationSize);
                        }
                        glVertexAttribBinding(attriNum, attrib._channel);
                    }

                    if (i == 0) {
                        frequency = attrib._frequency;
                    } else {
                        assert(frequency == attrib._frequency);
                    }


                    (void)CHECK_GL_ERROR();
                }
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
                glVertexBindingDivisor(bufferChannelNum, frequency * (isStereoNow ? 2 : 1));
#else
                glVertexBindingDivisor(bufferChannelNum, frequency);
#endif
            }

            if (!hasColorAttribute && _input._hadColorAttribute) {
                // The previous input stage had a color attribute but this one doesn't, so reset the color to pure white.
                _input._colorAttribute = glm::vec4(1.0f);
                glVertexAttrib4fv(Stream::COLOR, &_input._colorAttribute.r);
            }
        }

        // Manage Activation what was and what is expected now
        // This should only disable VertexAttribs since the one needed by the vertex format (if it exists) have been enabled above
        for (GLuint i = 0; i < (GLuint)newActivation.size(); i++) {
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
        (void)CHECK_GL_ERROR();

        _input._invalidFormat = false;
        _stats._ISNumFormatChanges++;
    }

    _input._hadColorAttribute = hasColorAttribute;
    _input._hasColorAttribute = false;

    if (_input._invalidBuffers.any()) {
        auto vbo = _input._bufferVBOs.data();
        auto offset = _input._bufferOffsets.data();
        auto stride = _input._bufferStrides.data();

        // Profile the count of buffers to update and use it to short cut the for loop
        int numInvalids = (int) _input._invalidBuffers.count();
        _stats._ISNumInputBufferChanges += numInvalids;

        auto numBuffers = _input._buffers.size();
        for (GLuint buffer = 0; buffer < numBuffers; buffer++, vbo++, offset++, stride++) {
            if (_input._invalidBuffers.test(buffer)) {
                glBindVertexBuffer(buffer, (*vbo), (*offset), (GLsizei)(*stride));
                numInvalids--;
                if (numInvalids <= 0) {
                    break;
                }
            }
        }

        _input._invalidBuffers.reset();
        (void)CHECK_GL_ERROR();
    }
}
