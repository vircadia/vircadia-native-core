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
        _input._buffers[channel] = buffer;
        _input._bufferOffsets[channel] = offset;
        _input._bufferStrides[channel] = stride;
        _input._buffersState.set(channel);
    }
}

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

void GLBackend::updateInput() {
    if (_input._invalidFormat || _input._buffersState.any()) {

        if (_input._invalidFormat) {
            InputStageState::ActivationCache newActivation;

            // Check expected activation
            if (_input._format) {
                const Stream::Format::AttributeMap& attributes = _input._format->getAttributes();
                for (Stream::Format::AttributeMap::const_iterator it = attributes.begin(); it != attributes.end(); it++) {
                    const Stream::Attribute& attrib = (*it).second;
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
                        }
                        else {
                            glDisableClientState(attributeSlotToClassicAttribName[i]);
                        }
                    } else {
#else 
                    {
#endif
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

            for (Stream::Format::ChannelMap::const_iterator channelIt = _input._format->getChannels().begin();
                    channelIt != _input._format->getChannels().end();
                    channelIt++) {
                const Stream::Format::ChannelMap::value_type::second_type& channel = (*channelIt).second;
                if ((*channelIt).first < buffers.size()) {
                    int bufferNum = (*channelIt).first;

                    if (_input._buffersState.test(bufferNum) || _input._invalidFormat) {
                        GLuint vbo = gpu::GLBackend::getBufferID((*buffers[bufferNum]));
                        glBindBuffer(GL_ARRAY_BUFFER, vbo);
                        (void) CHECK_GL_ERROR();
                        _input._buffersState[bufferNum] = false;

                        for (unsigned int i = 0; i < channel._slots.size(); i++) {
                            const Stream::Attribute& attrib = attributes.at(channel._slots[i]);
                            GLuint slot = attrib._slot;
                            GLuint count = attrib._element.getDimensionCount();
                            GLenum type = _elementTypeToGLType[attrib._element.getType()];
                            GLuint stride = strides[bufferNum];
                            GLuint pointer = attrib._offset + offsets[bufferNum];
                            #if defined(SUPPORT_LEGACY_OPENGL)
                            if (slot < NUM_CLASSIC_ATTRIBS) {
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
                            } else {
                            #else 
                            {
                            #endif
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

/* TODO: Fancy version GL4.4
    if (_needInputFormatUpdate) {

        InputActivationCache newActivation;

        // Assign the vertex format required
        if (_inputFormat) {
            const StreamFormat::AttributeMap& attributes = _inputFormat->getAttributes();
            for (StreamFormat::AttributeMap::const_iterator it = attributes.begin(); it != attributes.end(); it++) {
                const StreamFormat::Attribute& attrib = (*it).second;
                newActivation.set(attrib._slot);
                glVertexAttribFormat(
                    attrib._slot,
                    attrib._element.getDimensionCount(),
                    _elementTypeToGLType[attrib._element.getType()],
                    attrib._element.isNormalized(),
                    attrib._stride);
            }
            CHECK_GL_ERROR();
        }

        // Manage Activation what was and what is expected now
        for (int i = 0; i < newActivation.size(); i++) {
            bool newState = newActivation[i];
            if (newState != _inputAttributeActivation[i]) {
                if (newState) {
                    glEnableVertexAttribArray(i);
                } else {
                    glDisableVertexAttribArray(i);
                }
                _inputAttributeActivation.flip(i);
            }
        }
        CHECK_GL_ERROR();

        _needInputFormatUpdate = false;
    }

    if (_needInputStreamUpdate) {
        if (_inputStream) {
            const Stream::Buffers& buffers = _inputStream->getBuffers();
            const Stream::Offsets& offsets = _inputStream->getOffsets();
            const Stream::Strides& strides = _inputStream->getStrides();

            for (int i = 0; i < buffers.size(); i++) {
                GLuint vbo = gpu::GLBackend::getBufferID((*buffers[i]));
                glBindVertexBuffer(i, vbo, offsets[i], strides[i]);
            }

            CHECK_GL_ERROR();
        }
        _needInputStreamUpdate = false;
    }
*/
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
