//
//  GL41BackendInput.cpp
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


void GL41Backend::resetInputStage() {
    Parent::resetInputStage();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    for (uint32_t i = 0; i < _input._attributeActivation.size(); i++) {
        glDisableVertexAttribArray(i);
        glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, 0, 0);
    }
}

void GL41Backend::updateInput() {
    bool isStereoNow = isStereo();
    // track stereo state change potentially happening wihtout changing the input format
    // this is a rare case requesting to invalid the format
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
    _input._invalidFormat |= (isStereoNow != _input._lastUpdateStereoState);
#endif
    _input._lastUpdateStereoState = isStereoNow;

    bool hasColorAttribute = _input._hasColorAttribute;

    if (_input._invalidFormat || _input._invalidBuffers.any()) {

        auto format = acquire(_input._format);
        if (_input._invalidFormat) {
            InputStageState::ActivationCache newActivation;

            _stats._ISNumFormatChanges++;

            // Check expected activation
            if (format) {
                for (auto& it : format->getAttributes()) {
                    const Stream::Attribute& attrib = (it).second;
                    uint8_t locationCount = attrib._element.getLocationCount();
                    for (int i = 0; i < locationCount; ++i) {
                        newActivation.set(attrib._slot + i);
                    }
                }
            }

            // Manage Activation what was and what is expected now
            for (unsigned int i = 0; i < newActivation.size(); i++) {
                bool newState = newActivation[i];
                if (newState != _input._attributeActivation[i]) {

                    if (newState) {
                        glEnableVertexAttribArray(i);
                    } else {
                        glDisableVertexAttribArray(i);
                    }
                    (void)CHECK_GL_ERROR();

                    _input._attributeActivation.flip(i);
                }
            }
        }

        // now we need to bind the buffers and assign the attrib pointers
        if (format) {
            const auto& buffers = _input._buffers;
            const auto& offsets = _input._bufferOffsets;
            const auto& strides = _input._bufferStrides;

            const auto& attributes = format->getAttributes();
            const auto& inputChannels = format->getChannels();
            int numInvalids = (int)_input._invalidBuffers.count();
            _stats._ISNumInputBufferChanges += numInvalids;
            
            GLuint boundVBO = 0;
            for (auto& channelIt : inputChannels) {
                const Stream::Format::ChannelMap::value_type::second_type& channel = (channelIt).second;
                if ((channelIt).first < buffers.size()) {
                    int bufferNum = (channelIt).first;

                    if (_input._invalidBuffers.test(bufferNum) || _input._invalidFormat) {
                        //  GLuint vbo = gpu::GL41Backend::getBufferID((*buffers[bufferNum]));
                        GLuint vbo = _input._bufferVBOs[bufferNum];
                        if (boundVBO != vbo) {
                            glBindBuffer(GL_ARRAY_BUFFER, vbo);
                            (void)CHECK_GL_ERROR();
                            boundVBO = vbo;
                        }
                        _input._invalidBuffers[bufferNum] = false;

                        for (unsigned int i = 0; i < channel._slots.size(); i++) {
                            const Stream::Attribute& attrib = attributes.at(channel._slots[i]);
                            GLuint slot = attrib._slot;
                            GLuint count = attrib._element.getLocationScalarCount();
                            uint8_t locationCount = attrib._element.getLocationCount();
                            GLenum type = gl::ELEMENT_TYPE_TO_GL[attrib._element.getType()];
                            // GLenum perLocationStride = strides[bufferNum];
                            GLenum perLocationStride = attrib._element.getLocationSize();
                            GLuint stride = (GLuint)strides[bufferNum];
                            uintptr_t pointer = (uintptr_t)(attrib._offset + offsets[bufferNum]);
                            GLboolean isNormalized = attrib._element.isNormalized();

                            hasColorAttribute |= slot == Stream::COLOR;

                            for (size_t locNum = 0; locNum < locationCount; ++locNum) {
                                if (attrib._element.isInteger()) {
                                    glVertexAttribIPointer(slot + (GLuint)locNum, count, type, stride,
                                        reinterpret_cast<GLvoid*>(pointer + perLocationStride * (GLuint)locNum));
                                } else {
                                    glVertexAttribPointer(slot + (GLuint)locNum, count, type, isNormalized, stride,
                                        reinterpret_cast<GLvoid*>(pointer + perLocationStride * (GLuint)locNum));
                                }
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
                                glVertexAttribDivisor(slot + (GLuint)locNum, attrib._frequency * (isStereoNow ? 2 : 1));
#else
                                glVertexAttribDivisor(slot + (GLuint)locNum, attrib._frequency);
#endif
                            }
                            (void)CHECK_GL_ERROR();
                        }
                    }
                }
            }

            if (!hasColorAttribute && _input._hadColorAttribute) {
                // The previous input stage had a color attribute but this one doesn't, so reset the color to pure white.
                _input._colorAttribute = glm::vec4(1.0f);
                glVertexAttrib4fv(Stream::COLOR, &_input._colorAttribute.r);
            }
        }
        // everything format related should be in sync now
        _input._invalidFormat = false;
    }

    _input._hadColorAttribute = hasColorAttribute;
    _input._hasColorAttribute = false;
}
