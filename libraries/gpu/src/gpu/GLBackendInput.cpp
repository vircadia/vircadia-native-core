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
#include "GLBackendShared.h"

using namespace gpu;

void GLBackend::do_setInputFormat(Batch& batch, uint32 paramOffset) {
    Stream::FormatPointer format = batch._streamFormats.get(batch._params[paramOffset]._uint);

    if (format != _input._format) {
        _input._format = format;
        _input._invalidFormat = true;
    }
}

void GLBackend::do_setInputBuffer(Batch& batch, uint32 paramOffset) {
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
            _input._buffersState.set(channel);
        }
    }
}

#define NOT_SUPPORT_VAO
#if defined(SUPPORT_VAO)
#else

#define SUPPORT_LEGACY_OPENGL
#if defined(SUPPORT_LEGACY_OPENGL)
static const int NUM_CLASSIC_ATTRIBS = Stream::TANGENT;
static const GLenum attributeSlotToClassicAttribName[NUM_CLASSIC_ATTRIBS] = {
    GL_VERTEX_ARRAY,
    GL_NORMAL_ARRAY,
    GL_COLOR_ARRAY,
    GL_TEXTURE_COORD_ARRAY
};
#endif
#endif

void GLBackend::initInput() {
#if defined(SUPPORT_VAO)
    if(!_input._defaultVAO) {
        glGenVertexArrays(1, &_input._defaultVAO);
    }
    glBindVertexArray(_input._defaultVAO);
    (void) CHECK_GL_ERROR();
#endif
}

void GLBackend::killInput() {
#if defined(SUPPORT_VAO)
    glBindVertexArray(0);
    if(_input._defaultVAO) {
        glDeleteVertexArrays(1, &_input._defaultVAO);
    }
    (void) CHECK_GL_ERROR();
#endif
}

void GLBackend::syncInputStateCache() {
#if defined(SUPPORT_VAO)
    for (int i = 0; i < NUM_CLASSIC_ATTRIBS; i++) {
        _input._attributeActivation[i] = glIsEnabled(attributeSlotToClassicAttribName[i]);
    }
    //_input._defaultVAO
    glBindVertexArray(_input._defaultVAO);
#else
    size_t i = 0;
#if defined(SUPPORT_LEGACY_OPENGL)
    for (; i < NUM_CLASSIC_ATTRIBS; i++) {
        _input._attributeActivation[i] = glIsEnabled(attributeSlotToClassicAttribName[i]);
    }
#endif
    for (; i < _input._attributeActivation.size(); i++) {
        GLint active = 0;
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &active);
        _input._attributeActivation[i] = active;
    }
#endif
}

void GLBackend::updateInput() {
#if defined(SUPPORT_VAO)
    if (_input._invalidFormat) {

        InputStageState::ActivationCache newActivation;

        // Assign the vertex format required
        if (_input._format) {
            for (auto& it : _input._format->getAttributes()) {
                const Stream::Attribute& attrib = (it).second;
                newActivation.set(attrib._slot);
                glVertexAttribFormat(
                    attrib._slot,
                    attrib._element.getDimensionCount(),
                    _elementTypeToGLType[attrib._element.getType()],
                    attrib._element.isNormalized(),
                    attrib._offset);
            }
            (void) CHECK_GL_ERROR();
        }

        // Manage Activation what was and what is expected now
        for (int i = 0; i < newActivation.size(); i++) {
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

    if (_input._buffersState.any()) {
        int numBuffers = _input._buffers.size();
        auto buffer = _input._buffers.data();
        auto vbo = _input._bufferVBOs.data();
        auto offset = _input._bufferOffsets.data();
        auto stride = _input._bufferStrides.data();

        for (int bufferNum = 0; bufferNum < numBuffers; bufferNum++) {
            if (_input._buffersState.test(bufferNum)) {
                glBindVertexBuffer(bufferNum, (*vbo), (*offset), (*stride));
            }
            buffer++;
            vbo++;
            offset++;
            stride++;
        }
        _input._buffersState.reset();
        (void) CHECK_GL_ERROR();
    }
#else
    if (_input._invalidFormat || _input._buffersState.any()) {

        if (_input._invalidFormat) {
            InputStageState::ActivationCache newActivation;

            _stats._ISNumFormatChanges++;

            // Check expected activation
            if (_input._format) {
                for (auto& it : _input._format->getAttributes()) {
                    const Stream::Attribute& attrib = (it).second;
                    newActivation.set(attrib._slot);
                }
            }
            
            // Manage Activation what was and what is expected now
            for (unsigned int i = 0; i < newActivation.size(); i++) {
                bool newState = newActivation[i];
                if (newState != _input._attributeActivation[i]) {
#if defined(SUPPORT_LEGACY_OPENGL)
                    if (i < NUM_CLASSIC_ATTRIBS) {
                        if (newState) {
                            glEnableClientState(attributeSlotToClassicAttribName[i]);
                        } else {
                            glDisableClientState(attributeSlotToClassicAttribName[i]);
                        }
                    } else
#endif
                    {
                        if (newState) {
                            glEnableVertexAttribArray(i);
                        } else {
                            glDisableVertexAttribArray(i);
                        }
                    }
                    (void) CHECK_GL_ERROR();
                    
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

                    if (_input._buffersState.test(bufferNum) || _input._invalidFormat) {
                      //  GLuint vbo = gpu::GLBackend::getBufferID((*buffers[bufferNum]));
                        GLuint vbo = _input._bufferVBOs[bufferNum];
                        if (boundVBO != vbo) {
                            glBindBuffer(GL_ARRAY_BUFFER, vbo);
                            (void) CHECK_GL_ERROR();
                            boundVBO = vbo;
                        }
                        _input._buffersState[bufferNum] = false;

                        for (unsigned int i = 0; i < channel._slots.size(); i++) {
                            const Stream::Attribute& attrib = attributes.at(channel._slots[i]);
                            GLuint slot = attrib._slot;
                            GLuint count = attrib._element.getDimensionCount();
                            GLenum type = _elementTypeToGLType[attrib._element.getType()];
                            GLuint stride = strides[bufferNum];
                            GLuint pointer = attrib._offset + offsets[bufferNum];
#if defined(SUPPORT_LEGACY_OPENGL)
                            const bool useClientState = slot < NUM_CLASSIC_ATTRIBS;
                            if (useClientState) {
                                switch (slot) {
                                    case Stream::POSITION:
                                        glVertexPointer(count, type, stride, reinterpret_cast<GLvoid*>(pointer));
                                        break;
                                    case Stream::NORMAL:
                                        glNormalPointer(type, stride, reinterpret_cast<GLvoid*>(pointer));
                                        break;
                                    case Stream::COLOR:
                                        glColorPointer(count, type, stride, reinterpret_cast<GLvoid*>(pointer));
                                        break;
                                    case Stream::TEXCOORD:
                                        glTexCoordPointer(count, type, stride, reinterpret_cast<GLvoid*>(pointer));
                                        break;
                                };
                            } else 
#endif                  
                            {
                                GLboolean isNormalized = attrib._element.isNormalized();
                                glVertexAttribPointer(slot, count, type, isNormalized, stride,
                                                      reinterpret_cast<GLvoid*>(pointer));
                            }
                            (void) CHECK_GL_ERROR();
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


void GLBackend::do_setIndexBuffer(Batch& batch, uint32 paramOffset) {
    _input._indexBufferType = (Type) batch._params[paramOffset + 2]._uint;
    BufferPointer indexBuffer = batch._buffers.get(batch._params[paramOffset + 1]._uint);
    _input._indexBufferOffset = batch._params[paramOffset + 0]._uint;
    _input._indexBuffer = indexBuffer;
    if (indexBuffer) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, getBufferID(*indexBuffer));
    } else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    (void) CHECK_GL_ERROR();
}

template <typename V>
void popParam(Batch::Params& params, uint32& paramOffset, V& v) {
    for (size_t i = 0; i < v.length(); ++i) {
        v[i] = params[paramOffset++]._float;
    }
}
