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
#include "../gl/GLShared.h"

using namespace gpu;
using namespace gpu::gl45;

void GL45Backend::resetInputStage() {
    Parent::resetInputStage();

#ifdef GPU_USE_SEPARATE_VERTEX_FORMAT
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    for (uint32_t i = 0; i < _input._attributeActivation.size(); i++) {
        glDisableVertexAttribArray(i);
        glVertexAttribFormat(i, 4, GL_FLOAT, GL_FALSE, 0);
    }
    for (uint32_t i = 0; i < _input._attribBindingBuffers.size(); i++) {
        glBindVertexBuffer(i, 0, 0, 0);
    }
#else
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    for (uint32_t i = 0; i < _input._attributeActivation.size(); i++) {
        glDisableVertexAttribArray(i);
        glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, 0, 0);
    }
#endif
}

void GL45Backend::updateInput() {
#ifdef GPU_USE_SEPARATE_VERTEX_FORMAT
    if (_input._invalidFormat) {
        InputStageState::ActivationCache newActivation;

        // Assign the vertex format required
        if (_input._format) {
            _input._attribBindingBuffers.reset();

            const Stream::Format::AttributeMap& attributes = _input._format->getAttributes();
            auto& inputChannels = _input._format->getChannels();
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

                    GLuint offset = (GLuint)attrib._offset;;
                    GLboolean isNormalized = attrib._element.isNormalized();

                    GLenum perLocationSize = attrib._element.getLocationSize();
                    for (GLuint locNum = 0; locNum < locationCount; ++locNum) {
                        GLuint attriNum = (GLuint)(slot + locNum);
                        newActivation.set(attriNum);
                        if (!_input._attributeActivation[attriNum]) {
                            _input._attributeActivation.set(attriNum);
                            glEnableVertexAttribArray(attriNum);
                        }
                        glVertexAttribFormat(attriNum, count, type, isNormalized, offset + locNum * perLocationSize);
                        // TODO: Support properly the IAttrib version
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
                glVertexBindingDivisor(bufferChannelNum, frequency * (isStereo() ? 2 : 1));
#else
                glVertexBindingDivisor(bufferChannelNum, frequency);
#endif
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

    if (_input._invalidBuffers.any()) {
        auto vbo = _input._bufferVBOs.data();
        auto offset = _input._bufferOffsets.data();
        auto stride = _input._bufferStrides.data();

        for (GLuint buffer = 0; buffer < _input._buffers.size(); buffer++, vbo++, offset++, stride++) {
            if (_input._invalidBuffers.test(buffer)) {
                glBindVertexBuffer(buffer, (*vbo), (*offset), (GLsizei)(*stride));
            }
        }

        _input._invalidBuffers.reset();
        (void)CHECK_GL_ERROR();
    }
#else
    if (_input._invalidFormat || _input._invalidBuffers.any()) {

        if (_input._invalidFormat) {
            InputStageState::ActivationCache newActivation;

            _stats._ISNumFormatChanges++;

            // Check expected activation
            if (_input._format) {
                for (auto& it : _input._format->getAttributes()) {
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
        if (_input._format) {
            const Buffers& buffers = _input._buffers;
            const Offsets& offsets = _input._bufferOffsets;
            const Offsets& strides = _input._bufferStrides;

            const Stream::Format::AttributeMap& attributes = _input._format->getAttributes();
            auto& inputChannels = _input._format->getChannels();
            _stats._ISNumInputBufferChanges++;

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
                            GLuint pointer = (GLuint)(attrib._offset + offsets[bufferNum]);
                            GLboolean isNormalized = attrib._element.isNormalized();

                            for (size_t locNum = 0; locNum < locationCount; ++locNum) {
                                glVertexAttribPointer(slot + (GLuint)locNum, count, type, isNormalized, stride,
                                    reinterpret_cast<GLvoid*>(pointer + perLocationStride * (GLuint)locNum));
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
                                glVertexAttribDivisor(slot + (GLuint)locNum, attrib._frequency * (isStereo() ? 2 : 1));
#else
                                glVertexAttribDivisor(slot + (GLuint)locNum, attrib._frequency);
#endif
                            }

                            // TODO: Support properly the IAttrib version

                            (void)CHECK_GL_ERROR();
                        }
                    }
                }
            }
        }
        // everything format related should be in sync now
        _input._invalidFormat = false;
    }
#endif
}
