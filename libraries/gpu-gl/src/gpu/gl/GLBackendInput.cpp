//
//  GLBackendInput.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"
#include "GLShared.h"
#include "GLInputFormat.h"

using namespace gpu;
using namespace gpu::gl;

void GLBackend::do_setInputFormat(Batch& batch, size_t paramOffset) {
    Stream::FormatPointer format = batch._streamFormats.get(batch._params[paramOffset]._uint);
    if (format != _input._format) {
        _input._format = format;
        if (format) {
            auto inputFormat = GLInputFormat::sync((*format));
            assert(inputFormat);
            if (_input._formatKey != inputFormat->key) {
                _input._formatKey = inputFormat->key;
                _input._invalidFormat = true;
            }
        } else {
            _input._formatKey.clear();
            _input._invalidFormat = true;
        }
    }
}

void GLBackend::do_setInputBuffer(Batch& batch, size_t paramOffset) {
    Offset stride = batch._params[paramOffset + 0]._uint;
    Offset offset = batch._params[paramOffset + 1]._uint;
    BufferPointer buffer = batch._buffers.get(batch._params[paramOffset + 2]._uint);
    uint32 channel = batch._params[paramOffset + 3]._uint;

    if (channel < getNumInputBuffers()) {
        bool isModified = false;
        if (_input._buffers[channel] != buffer) {
            _input._buffers[channel] = buffer;
         
            GLuint vbo = 0;
            if (buffer) {
                vbo = getBufferID((*buffer));
            }
            _input._bufferVBOs[channel] = vbo;

            isModified = true;
        }

        if (_input._bufferOffsets[channel] != offset) {
            _input._bufferOffsets[channel] = offset;
            isModified = true;
        }

        if (_input._bufferStrides[channel] != stride) {
            _input._bufferStrides[channel] = stride;
            isModified = true;
        }

        if (isModified) {
            _input._invalidBuffers.set(channel);
        }
    }
}

void GLBackend::initInput() {
    if(!_input._defaultVAO) {
        glGenVertexArrays(1, &_input._defaultVAO);
    }
    glBindVertexArray(_input._defaultVAO);
    (void) CHECK_GL_ERROR();
}

void GLBackend::killInput() {
    glBindVertexArray(0);
    if(_input._defaultVAO) {
        glDeleteVertexArrays(1, &_input._defaultVAO);
    }
    (void) CHECK_GL_ERROR();
}

void GLBackend::syncInputStateCache() {
    for (uint32_t i = 0; i < _input._attributeActivation.size(); i++) {
        GLint active = 0;
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &active);
        _input._attributeActivation[i] = active;
    }
    //_input._defaultVAO
    glBindVertexArray(_input._defaultVAO);
}


void GLBackend::updateInput() {
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
                                glVertexAttribDivisor(slot + (GLuint)locNum, attrib._frequency);
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
}

void GLBackend::resetInputStage() {
    // Reset index buffer
    _input._indexBufferType = UINT32;
    _input._indexBufferOffset = 0;
    _input._indexBuffer.reset();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    (void) CHECK_GL_ERROR();

    // Reset vertex buffer and format
    _input._format.reset();
    _input._formatKey.clear();
    _input._invalidFormat = false;
    _input._attributeActivation.reset();

    for (uint32_t i = 0; i < _input._buffers.size(); i++) {
        _input._buffers[i].reset();
        _input._bufferOffsets[i] = 0;
        _input._bufferStrides[i] = 0;
        _input._bufferVBOs[i] = 0;
    }
    _input._invalidBuffers.reset();

    // THe vertex array binding MUST be reset in the specific Backend versions as they use different techniques
}

void GLBackend::do_setIndexBuffer(Batch& batch, size_t paramOffset) {
    _input._indexBufferType = (Type)batch._params[paramOffset + 2]._uint;
    _input._indexBufferOffset = batch._params[paramOffset + 0]._uint;

    BufferPointer indexBuffer = batch._buffers.get(batch._params[paramOffset + 1]._uint);
    if (indexBuffer != _input._indexBuffer) {
        _input._indexBuffer = indexBuffer;
        if (indexBuffer) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, getBufferID(*indexBuffer));
        } else {
            // FIXME do we really need this?  Is there ever a draw call where we care that the element buffer is null?
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
    }
    (void) CHECK_GL_ERROR();
}

void GLBackend::do_setIndirectBuffer(Batch& batch, size_t paramOffset) {
    _input._indirectBufferOffset = batch._params[paramOffset + 1]._uint;
    _input._indirectBufferStride = batch._params[paramOffset + 2]._uint;

    BufferPointer buffer = batch._buffers.get(batch._params[paramOffset]._uint);
    if (buffer != _input._indirectBuffer) {
        _input._indirectBuffer = buffer;
        if (buffer) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, getBufferID(*buffer));
        } else {
            // FIXME do we really need this?  Is there ever a draw call where we care that the element buffer is null?
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        }
    }

    (void)CHECK_GL_ERROR();
}
