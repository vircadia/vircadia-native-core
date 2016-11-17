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

void GLBackend::do_setInputFormat(const Batch& batch, size_t paramOffset) {
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

void GLBackend::do_setInputBuffer(const Batch& batch, size_t paramOffset) {
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

void GLBackend::do_setIndexBuffer(const Batch& batch, size_t paramOffset) {
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

void GLBackend::do_setIndirectBuffer(const Batch& batch, size_t paramOffset) {
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

