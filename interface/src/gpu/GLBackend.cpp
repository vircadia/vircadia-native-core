//
//  GLBackend.cpp
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"

#include <QDebug>

#include "gpu/Batch.h"

using namespace gpu;

GLBackend::CommandCall GLBackend::_commandCalls[Batch::NUM_COMMANDS] = 
{
    (&::gpu::GLBackend::do_draw),
    (&::gpu::GLBackend::do_drawIndexed),
    (&::gpu::GLBackend::do_drawInstanced),
    (&::gpu::GLBackend::do_drawIndexedInstanced),

    (&::gpu::GLBackend::do_setInputFormat),

    (&::gpu::GLBackend::do_setInputBuffer),

    (&::gpu::GLBackend::do_setIndexBuffer),

    (&::gpu::GLBackend::do_glEnable),
    (&::gpu::GLBackend::do_glDisable),

    (&::gpu::GLBackend::do_glEnableClientState),
    (&::gpu::GLBackend::do_glDisableClientState),

    (&::gpu::GLBackend::do_glCullFace),
    (&::gpu::GLBackend::do_glAlphaFunc),

    (&::gpu::GLBackend::do_glDepthFunc),
    (&::gpu::GLBackend::do_glDepthMask),
    (&::gpu::GLBackend::do_glDepthRange),

    (&::gpu::GLBackend::do_glBindBuffer),

    (&::gpu::GLBackend::do_glBindTexture),
    (&::gpu::GLBackend::do_glActiveTexture),

    (&::gpu::GLBackend::do_glDrawBuffers),

    (&::gpu::GLBackend::do_glUseProgram),
    (&::gpu::GLBackend::do_glUniform1f),
    (&::gpu::GLBackend::do_glUniformMatrix4fv),

    (&::gpu::GLBackend::do_glMatrixMode),
    (&::gpu::GLBackend::do_glPushMatrix),
    (&::gpu::GLBackend::do_glPopMatrix),
    (&::gpu::GLBackend::do_glMultMatrixf),
    (&::gpu::GLBackend::do_glLoadMatrixf),
    (&::gpu::GLBackend::do_glLoadIdentity),
    (&::gpu::GLBackend::do_glRotatef),
    (&::gpu::GLBackend::do_glScalef),
    (&::gpu::GLBackend::do_glTranslatef),

    (&::gpu::GLBackend::do_glDrawArrays),
    (&::gpu::GLBackend::do_glDrawRangeElements),

    (&::gpu::GLBackend::do_glColorPointer),
    (&::gpu::GLBackend::do_glNormalPointer),
    (&::gpu::GLBackend::do_glTexCoordPointer),
    (&::gpu::GLBackend::do_glVertexPointer),

    (&::gpu::GLBackend::do_glVertexAttribPointer),
    (&::gpu::GLBackend::do_glEnableVertexAttribArray),
    (&::gpu::GLBackend::do_glDisableVertexAttribArray),

    (&::gpu::GLBackend::do_glColor4f),

    (&::gpu::GLBackend::do_glMaterialf),
    (&::gpu::GLBackend::do_glMaterialfv),
};

static const GLenum _primitiveToGLmode[NUM_PRIMITIVES] = {
    GL_POINTS,
    GL_LINES,
    GL_LINE_STRIP,
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_QUADS,
};

static const GLenum _elementTypeToGLType[NUM_TYPES]= {
    GL_FLOAT,
    GL_INT,
    GL_UNSIGNED_INT,
    GL_HALF_FLOAT,
    GL_SHORT,
    GL_UNSIGNED_SHORT,
    GL_BYTE,
    GL_UNSIGNED_BYTE,
    GL_FLOAT,
    GL_INT,
    GL_UNSIGNED_INT,
    GL_HALF_FLOAT,
    GL_SHORT,
    GL_UNSIGNED_SHORT,
    GL_BYTE,
    GL_UNSIGNED_BYTE
};


GLBackend::GLBackend() :

    _inputFormat(0),
    _inputAttributeActivation(0),
    _needInputFormatUpdate(true),

    _inputBuffersState(0),
    _inputBuffers(_inputBuffersState.size(), BufferPointer(0)),
    _inputBufferOffsets(_inputBuffersState.size(), 0),
    _inputBufferStrides(_inputBuffersState.size(), 0),

    _indexBuffer(0),
    _indexBufferOffset(0)
{

}

GLBackend::~GLBackend() {

}

void GLBackend::renderBatch(Batch& batch) {
    uint32 numCommands = batch.getCommands().size();
    const Batch::Commands::value_type* command = batch.getCommands().data();
    const Batch::CommandOffsets::value_type* offset = batch.getCommandOffsets().data();

    GLBackend backend;

    for (int i = 0; i < numCommands; i++) {
        CommandCall call = _commandCalls[(*command)];
        (backend.*(call))(batch, *offset);
        command++;
        offset++;
    }
}

void GLBackend::checkGLError() {
    GLenum error = glGetError();
    if (!error) {
        return;
    }
    else {
        switch (error) {
        case GL_INVALID_ENUM:
            qDebug() << "An unacceptable value is specified for an enumerated argument.The offending command is ignored and has no other side effect than to set the error flag.";
            break;
        case GL_INVALID_VALUE:
            qDebug() << "A numeric argument is out of range.The offending command is ignored and has no other side effect than to set the error flag";
            break;
        case GL_INVALID_OPERATION:
            qDebug() << "The specified operation is not allowed in the current state.The offending command is ignored and has no other side effect than to set the error flag..";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            qDebug() << "The framebuffer object is not complete.The offending command is ignored and has no other side effect than to set the error flag.";
            break;
        case GL_OUT_OF_MEMORY:
            qDebug() << "There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
            break;
        case GL_STACK_UNDERFLOW:
            qDebug() << "An attempt has been made to perform an operation that would cause an internal stack to underflow.";
            break;
        case GL_STACK_OVERFLOW:
            qDebug() << "An attempt has been made to perform an operation that would cause an internal stack to overflow.";
            break;
        }
    }
}

#define CHECK_GL_ERROR() ::gpu::GLBackend::checkGLError()
//#define CHECK_GL_ERROR()

void GLBackend::do_draw(Batch& batch, uint32 paramOffset) {
    updateInput();

    Primitive primitiveType = (Primitive)batch._params[paramOffset + 2]._uint;
    GLenum mode = _primitiveToGLmode[primitiveType];
    uint32 numVertices = batch._params[paramOffset + 1]._uint;
    uint32 startVertex = batch._params[paramOffset + 0]._uint;

    glDrawArrays(mode, startVertex, numVertices);
    CHECK_GL_ERROR();
}

void GLBackend::do_drawIndexed(Batch& batch, uint32 paramOffset) {
    updateInput();

    Primitive primitiveType = (Primitive)batch._params[paramOffset + 2]._uint;
    GLenum mode = _primitiveToGLmode[primitiveType];
    uint32 numIndices = batch._params[paramOffset + 1]._uint;
    uint32 startIndex = batch._params[paramOffset + 0]._uint;

    GLenum glType = _elementTypeToGLType[_indexBufferType];

    glDrawElements(mode, numIndices, glType, (GLvoid*)(startIndex + _indexBufferOffset));
    CHECK_GL_ERROR();
}

void GLBackend::do_drawInstanced(Batch& batch, uint32 paramOffset) {
    CHECK_GL_ERROR();
}

void GLBackend::do_drawIndexedInstanced(Batch& batch, uint32 paramOffset) {
    CHECK_GL_ERROR();
}

void GLBackend::do_setInputFormat(Batch& batch, uint32 paramOffset) {
    Stream::FormatPointer format = batch._streamFormats.get(batch._params[paramOffset]._uint);

    if (format != _inputFormat) {
        _inputFormat = format;
        _needInputFormatUpdate = true;
    }
}

void GLBackend::do_setInputBuffer(Batch& batch, uint32 paramOffset) {
    Offset stride = batch._params[paramOffset + 0]._uint;
    Offset offset = batch._params[paramOffset + 1]._uint;
    BufferPointer buffer = batch._buffers.get(batch._params[paramOffset + 2]._uint);
    uint32 channel = batch._params[paramOffset + 3]._uint;

    if (channel < getNumInputBuffers()) {
        _inputBuffers[channel] = buffer;
        _inputBufferOffsets[channel] = offset;
        _inputBufferStrides[channel] = stride;
        _inputBuffersState.set(channel);
    }
}

#define SUPPORT_LEGACY_OPENGL
#if defined(SUPPORT_LEGACY_OPENGL)
static const int NUM_CLASSIC_ATTRIBS = Stream::INPUT_SLOT_TANGENT;
static const GLenum attributeSlotToClassicAttribName[NUM_CLASSIC_ATTRIBS] = {
    GL_VERTEX_ARRAY,
    GL_NORMAL_ARRAY,
    GL_COLOR_ARRAY,
    GL_TEXTURE_COORD_ARRAY
};
#endif

void GLBackend::updateInput() {
    if (_needInputFormatUpdate || _inputBuffersState.any()) {

        if (_needInputFormatUpdate) {
            InputActivationCache newActivation;

            // Check expected activation
            if (_inputFormat) {
                const Stream::Format::AttributeMap& attributes = _inputFormat->getAttributes();
                for (Stream::Format::AttributeMap::const_iterator it = attributes.begin(); it != attributes.end(); it++) {
                    const Stream::Attribute& attrib = (*it).second;
                    newActivation.set(attrib._slot);
                }
            }

            // Manage Activation what was and what is expected now
            for (int i = 0; i < newActivation.size(); i++) {
                bool newState = newActivation[i];
                if (newState != _inputAttributeActivation[i]) {
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
                    CHECK_GL_ERROR();

                    _inputAttributeActivation.flip(i);
                }
            }
        }

        // now we need to bind the buffers and assign the attrib pointers
        if (_inputFormat) {
            const Buffers& buffers = _inputBuffers;
            const Offsets& offsets = _inputBufferOffsets;
            const Offsets& strides = _inputBufferStrides;

            const Stream::Format::AttributeMap& attributes = _inputFormat->getAttributes();

            for (Stream::Format::ChannelMap::const_iterator channelIt = _inputFormat->getChannels().begin();
                    channelIt != _inputFormat->getChannels().end();
                    channelIt++) {
                const Stream::Format::ChannelMap::value_type::second_type& channel = (*channelIt).second;
                if ((*channelIt).first < buffers.size()) {
                    int bufferNum = (*channelIt).first;

                    if (_inputBuffersState.at(bufferNum) || _needInputFormatUpdate) {
                        GLuint vbo = gpu::GLBackend::getBufferID((*buffers[bufferNum]));
                        glBindBuffer(GL_ARRAY_BUFFER, vbo);
                        CHECK_GL_ERROR();
                        _inputBuffersState[bufferNum] = false;

                        for (int i = 0; i < channel._slots.size(); i++) {
                            const Stream::Attribute& attrib = attributes.at(channel._slots[i]);
                            GLuint slot = attrib._slot;
                            GLuint count = attrib._element.getDimensionCount();
                            GLenum type = _elementTypeToGLType[attrib._element.getType()];
                            GLuint stride = strides[bufferNum];
                            GLuint pointer = attrib._offset + offsets[bufferNum];
                            #if defined(SUPPORT_LEGACY_OPENGL)
                            if (slot < NUM_CLASSIC_ATTRIBS) {
                                switch (slot) {
                                case Stream::INPUT_SLOT_POSITION:
                                    glVertexPointer(count, type, stride, (GLvoid*)pointer);
                                    break;
                                case Stream::INPUT_SLOT_NORMAL:
                                    glNormalPointer(type, stride, (GLvoid*)pointer);
                                    break;
                                case Stream::INPUT_SLOT_COLOR:
                                    glColorPointer(count, type, stride, (GLvoid*)pointer);
                                    break;
                                case Stream::INPUT_SLOT_TEXCOORD:
                                    glTexCoordPointer(count, type, stride, (GLvoid*)pointer); 
                                    break;
                                };
                            } else {
                            #else 
                            {
                            #endif
                                GLboolean isNormalized = attrib._element.isNormalized();
                                glVertexAttribPointer(slot, count, type, isNormalized, stride, (GLvoid*)pointer);
                            }
                            CHECK_GL_ERROR();
                        }
                    }
                }
            }
        }
        // everything format related should be in sync now
        _needInputFormatUpdate = false;
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
    _indexBufferType = (Type) batch._params[paramOffset + 2]._uint;
    BufferPointer indexBuffer = batch._buffers.get(batch._params[paramOffset + 1]._uint);
    _indexBufferOffset = batch._params[paramOffset + 0]._uint;
    _indexBuffer = indexBuffer;
    if (indexBuffer) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, getBufferID(*indexBuffer));
    } else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    CHECK_GL_ERROR();
}

// TODO: As long as we have gl calls explicitely issued from interface
// code, we need to be able to record and batch these calls. THe long 
// term strategy is to get rid of any GL calls in favor of the HIFI GPU API

#define ADD_COMMAND_GL(call) _commands.push_back(COMMAND_##call); _commandOffsets.push_back(_params.size());

//#define DO_IT_NOW(call, offset) runLastCommand();
#define DO_IT_NOW(call, offset) 


void Batch::_glEnable(GLenum cap) {
    ADD_COMMAND_GL(glEnable);

    _params.push_back(cap);

    DO_IT_NOW(_glEnable, 1);
}
void GLBackend::do_glEnable(Batch& batch, uint32 paramOffset) {
    glEnable(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDisable(GLenum cap) {
    ADD_COMMAND_GL(glDisable);

    _params.push_back(cap);

    DO_IT_NOW(_glDisable, 1);
}
void GLBackend::do_glDisable(Batch& batch, uint32 paramOffset) {
    glDisable(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glEnableClientState(GLenum array) {
    ADD_COMMAND_GL(glEnableClientState);

    _params.push_back(array);

    DO_IT_NOW(_glEnableClientState, 1);
}
void GLBackend::do_glEnableClientState(Batch& batch, uint32 paramOffset) {
    glEnableClientState(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDisableClientState(GLenum array) {
    ADD_COMMAND_GL(glDisableClientState);

    _params.push_back(array);

    DO_IT_NOW(_glDisableClientState, 1);
}
void GLBackend::do_glDisableClientState(Batch& batch, uint32 paramOffset) {
    glDisableClientState(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glCullFace(GLenum mode) {
    ADD_COMMAND_GL(glCullFace);

    _params.push_back(mode);

    DO_IT_NOW(_glCullFace, 1);
}
void GLBackend::do_glCullFace(Batch& batch, uint32 paramOffset) {
    glCullFace(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glAlphaFunc(GLenum func, GLclampf ref) {
    ADD_COMMAND_GL(glAlphaFunc);

    _params.push_back(ref);
    _params.push_back(func);

    DO_IT_NOW(_glAlphaFunc, 2);
}
void GLBackend::do_glAlphaFunc(Batch& batch, uint32 paramOffset) {
    glAlphaFunc(
        batch._params[paramOffset + 1]._uint,
        batch._params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glDepthFunc(GLenum func) {
    ADD_COMMAND_GL(glDepthFunc);

    _params.push_back(func);

    DO_IT_NOW(_glDepthFunc, 1);
}
void GLBackend::do_glDepthFunc(Batch& batch, uint32 paramOffset) {
    glDepthFunc(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDepthMask(GLboolean flag) {
    ADD_COMMAND_GL(glDepthMask);

    _params.push_back(flag);

    DO_IT_NOW(_glDepthMask, 1);
}
void GLBackend::do_glDepthMask(Batch& batch, uint32 paramOffset) {
    glDepthMask(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDepthRange(GLclampd zNear, GLclampd zFar) {
    ADD_COMMAND_GL(glDepthRange);

    _params.push_back(zFar);
    _params.push_back(zNear);

    DO_IT_NOW(_glDepthRange, 2);
}
void GLBackend::do_glDepthRange(Batch& batch, uint32 paramOffset) {
    glDepthRange(
        batch._params[paramOffset + 1]._double,
        batch._params[paramOffset + 0]._double);
    CHECK_GL_ERROR();
}

void Batch::_glBindBuffer(GLenum target, GLuint buffer) {
    ADD_COMMAND_GL(glBindBuffer);

    _params.push_back(buffer);
    _params.push_back(target);

    DO_IT_NOW(_glBindBuffer, 2);
}
void GLBackend::do_glBindBuffer(Batch& batch, uint32 paramOffset) {
    glBindBuffer(
        batch._params[paramOffset + 1]._uint,
        batch._params[paramOffset + 0]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glBindTexture(GLenum target, GLuint texture) {
    ADD_COMMAND_GL(glBindTexture);

    _params.push_back(texture);
    _params.push_back(target);

    DO_IT_NOW(_glBindTexture, 2);
}
void GLBackend::do_glBindTexture(Batch& batch, uint32 paramOffset) {
    glBindTexture(
        batch._params[paramOffset + 1]._uint,
        batch._params[paramOffset + 0]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glActiveTexture(GLenum texture) {
    ADD_COMMAND_GL(glActiveTexture);

    _params.push_back(texture);

    DO_IT_NOW(_glActiveTexture, 1);
}
void GLBackend::do_glActiveTexture(Batch& batch, uint32 paramOffset) {
    glActiveTexture(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDrawBuffers(GLsizei n, const GLenum* bufs) {
    ADD_COMMAND_GL(glDrawBuffers);

    _params.push_back(cacheData(n * sizeof(GLenum), bufs));
    _params.push_back(n);

    DO_IT_NOW(_glDrawBuffers, 2);
}
void GLBackend::do_glDrawBuffers(Batch& batch, uint32 paramOffset) {
    glDrawBuffers(
        batch._params[paramOffset + 1]._uint,
        (const GLenum*)batch.editData(batch._params[paramOffset + 0]._uint));
    CHECK_GL_ERROR();
}

void Batch::_glUseProgram(GLuint program) {
    ADD_COMMAND_GL(glUseProgram);

    _params.push_back(program);

    DO_IT_NOW(_glUseProgram, 1);
}
void GLBackend::do_glUseProgram(Batch& batch, uint32 paramOffset) {
    glUseProgram(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glUniform1f(GLint location, GLfloat v0) {
    ADD_COMMAND_GL(glUniform1f);

    _params.push_back(v0);
    _params.push_back(location);

    DO_IT_NOW(_glUniform1f, 1);
}
void GLBackend::do_glUniform1f(Batch& batch, uint32 paramOffset) {
    glUniform1f(
        batch._params[paramOffset + 1]._int,
        batch._params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    ADD_COMMAND_GL(glUniformMatrix4fv);

    const int MATRIX4_SIZE = 16 * sizeof(float);
    _params.push_back(cacheData(count * MATRIX4_SIZE, value));
    _params.push_back(transpose);
    _params.push_back(count);
    _params.push_back(location);

    DO_IT_NOW(_glUniformMatrix4fv, 4);
}
void GLBackend::do_glUniformMatrix4fv(Batch& batch, uint32 paramOffset) {
    glUniformMatrix4fv(
        batch._params[paramOffset + 3]._int,
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._uint,
        (const GLfloat*)batch.editData(batch._params[paramOffset + 0]._uint));
    CHECK_GL_ERROR();
}

void Batch::_glMatrixMode(GLenum mode) {
    ADD_COMMAND_GL(glMatrixMode);

    _params.push_back(mode);

    DO_IT_NOW(_glMatrixMode, 1);
}
void GLBackend::do_glMatrixMode(Batch& batch, uint32 paramOffset) {
    glMatrixMode(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glPushMatrix() {
    ADD_COMMAND_GL(glPushMatrix);

    DO_IT_NOW(_glPushMatrix, 0);
}
void GLBackend::do_glPushMatrix(Batch& batch, uint32 paramOffset) {
    glPushMatrix();
    CHECK_GL_ERROR();
}

void Batch::_glPopMatrix() {
    ADD_COMMAND_GL(glPopMatrix);

    DO_IT_NOW(_glPopMatrix, 0);
}
void GLBackend::do_glPopMatrix(Batch& batch, uint32 paramOffset) {
    glPopMatrix();
    CHECK_GL_ERROR();
}

void Batch::_glMultMatrixf(const GLfloat *m) {
    ADD_COMMAND_GL(glMultMatrixf);

    const int MATRIX4_SIZE = 16 * sizeof(float);
    _params.push_back(cacheData(MATRIX4_SIZE, m));

    DO_IT_NOW(_glMultMatrixf, 1);
}
void GLBackend::do_glMultMatrixf(Batch& batch, uint32 paramOffset) {
    glMultMatrixf((const GLfloat*)batch.editData(batch._params[paramOffset]._uint));
    CHECK_GL_ERROR();
}

void Batch::_glLoadMatrixf(const GLfloat *m) {
    ADD_COMMAND_GL(glLoadMatrixf);

    const int MATRIX4_SIZE = 16 * sizeof(float);
    _params.push_back(cacheData(MATRIX4_SIZE, m));

    DO_IT_NOW(_glLoadMatrixf, 1);
}
void GLBackend::do_glLoadMatrixf(Batch& batch, uint32 paramOffset) {
    glLoadMatrixf((const GLfloat*)batch.editData(batch._params[paramOffset]._uint));
    CHECK_GL_ERROR();
}

void Batch::_glLoadIdentity(void) {
    ADD_COMMAND_GL(glLoadIdentity);

    DO_IT_NOW(_glLoadIdentity, 0);
}
void GLBackend::do_glLoadIdentity(Batch& batch, uint32 paramOffset) {
    glLoadIdentity();
    CHECK_GL_ERROR();
}

void Batch::_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    ADD_COMMAND_GL(glRotatef);

    _params.push_back(z);
    _params.push_back(y);
    _params.push_back(x);
    _params.push_back(angle);

    DO_IT_NOW(_glRotatef, 4);
}
void GLBackend::do_glRotatef(Batch& batch, uint32 paramOffset) {
    glRotatef(
        batch._params[paramOffset + 3]._float,
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glScalef(GLfloat x, GLfloat y, GLfloat z) {
    ADD_COMMAND_GL(glScalef);

    _params.push_back(z);
    _params.push_back(y);
    _params.push_back(x);

    DO_IT_NOW(_glScalef, 3);
}
void GLBackend::do_glScalef(Batch& batch, uint32 paramOffset) {
    glScalef(
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    ADD_COMMAND_GL(glTranslatef);

    _params.push_back(z);
    _params.push_back(y);
    _params.push_back(x);

    DO_IT_NOW(_glTranslatef, 3);
}
void GLBackend::do_glTranslatef(Batch& batch, uint32 paramOffset) {
    glTranslatef(
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    ADD_COMMAND_GL(glDrawArrays);

    _params.push_back(count);
    _params.push_back(first);
    _params.push_back(mode);

    DO_IT_NOW(_glDrawArrays, 3);
}
void GLBackend::do_glDrawArrays(Batch& batch, uint32 paramOffset) {
    glDrawArrays(
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._int,
        batch._params[paramOffset + 0]._int);
    CHECK_GL_ERROR();
}

void Batch::_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {
    ADD_COMMAND_GL(glDrawRangeElements);

    _params.push_back(cacheResource(indices));
    _params.push_back(type);
    _params.push_back(count);
    _params.push_back(end);
    _params.push_back(start);
    _params.push_back(mode);

    DO_IT_NOW(_glDrawRangeElements, 6);
}
void GLBackend::do_glDrawRangeElements(Batch& batch, uint32 paramOffset) {
    glDrawRangeElements(
        batch._params[paramOffset + 5]._uint,
        batch._params[paramOffset + 4]._uint,
        batch._params[paramOffset + 3]._uint,
        batch._params[paramOffset + 2]._int,
        batch._params[paramOffset + 1]._uint,
        batch.editResource(batch._params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}

void Batch::_glColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    ADD_COMMAND_GL(glColorPointer);

    _params.push_back(cacheResource(pointer));
    _params.push_back(stride);
    _params.push_back(type);
    _params.push_back(size);

    DO_IT_NOW(_glColorPointer, 4);
}
void GLBackend::do_glColorPointer(Batch& batch, uint32 paramOffset) {
    glColorPointer(
        batch._params[paramOffset + 3]._int,
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._int,
        batch.editResource(batch._params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}

void Batch::_glNormalPointer(GLenum type, GLsizei stride, const void *pointer) {
    ADD_COMMAND_GL(glNormalPointer);

    _params.push_back(cacheResource(pointer));
    _params.push_back(stride);
    _params.push_back(type);

    DO_IT_NOW(_glNormalPointer, 3);
}
void GLBackend::do_glNormalPointer(Batch& batch, uint32 paramOffset) {
    glNormalPointer(
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._int,
        batch.editResource(batch._params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}

void Batch::_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    ADD_COMMAND_GL(glTexCoordPointer);

    _params.push_back(cacheResource(pointer));
    _params.push_back(stride);
    _params.push_back(type);
    _params.push_back(size);

    DO_IT_NOW(_glTexCoordPointer, 4);
}
void GLBackend::do_glTexCoordPointer(Batch& batch, uint32 paramOffset) {
    glTexCoordPointer(
        batch._params[paramOffset + 3]._int,
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._int,
        batch.editResource(batch._params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}

void Batch::_glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    ADD_COMMAND_GL(glVertexPointer);

    _params.push_back(cacheResource(pointer));
    _params.push_back(stride);
    _params.push_back(type);
    _params.push_back(size);

    DO_IT_NOW(_glVertexPointer, 4);
}
void GLBackend::do_glVertexPointer(Batch& batch, uint32 paramOffset) {
    glVertexPointer(
        batch._params[paramOffset + 3]._int,
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._int,
        batch.editResource(batch._params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}


void Batch::_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    ADD_COMMAND_GL(glVertexAttribPointer);

    _params.push_back(cacheResource(pointer));
    _params.push_back(stride);
    _params.push_back(normalized);
    _params.push_back(type);
    _params.push_back(size);
    _params.push_back(index);

    DO_IT_NOW(_glVertexAttribPointer, 6);
}
void GLBackend::do_glVertexAttribPointer(Batch& batch, uint32 paramOffset) {
    glVertexAttribPointer(
        batch._params[paramOffset + 5]._uint,
        batch._params[paramOffset + 4]._int,
        batch._params[paramOffset + 3]._uint,
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._int,
        batch.editResource(batch._params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}

void Batch::_glEnableVertexAttribArray(GLint location) {
    ADD_COMMAND_GL(glEnableVertexAttribArray);

    _params.push_back(location);

    DO_IT_NOW(_glEnableVertexAttribArray, 1);
}
void GLBackend::do_glEnableVertexAttribArray(Batch& batch, uint32 paramOffset) {
    glEnableVertexAttribArray(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDisableVertexAttribArray(GLint location) {
    ADD_COMMAND_GL(glDisableVertexAttribArray);

    _params.push_back(location);

    DO_IT_NOW(_glDisableVertexAttribArray, 1);
}
void GLBackend::do_glDisableVertexAttribArray(Batch& batch, uint32 paramOffset) {
    glDisableVertexAttribArray(batch._params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    ADD_COMMAND_GL(glColor4f);

    _params.push_back(alpha);
    _params.push_back(blue);
    _params.push_back(green);
    _params.push_back(red);

    DO_IT_NOW(_glColor4f, 4);
}
void GLBackend::do_glColor4f(Batch& batch, uint32 paramOffset) {
    glColor4f(
        batch._params[paramOffset + 3]._float,
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glMaterialf(GLenum face, GLenum pname, GLfloat param) {
    ADD_COMMAND_GL(glMaterialf);

    _params.push_back(param);
    _params.push_back(pname);
    _params.push_back(face);

    DO_IT_NOW(_glMaterialf, 3);
}
void GLBackend::do_glMaterialf(Batch& batch, uint32 paramOffset) {
    glMaterialf(
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._uint,
        batch._params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
    ADD_COMMAND_GL(glMaterialfv);

    _params.push_back(cacheData(4 * sizeof(float), params));
    _params.push_back(pname);
    _params.push_back(face);

    DO_IT_NOW(_glMaterialfv, 3);
}
void GLBackend::do_glMaterialfv(Batch& batch, uint32 paramOffset) {
    glMaterialfv(
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._uint,
        (const GLfloat*)batch.editData(batch._params[paramOffset + 0]._uint));
    CHECK_GL_ERROR();
}

GLBackend::GLBuffer::GLBuffer() :
    _stamp(0),
    _buffer(0),
    _size(0)
{}

GLBackend::GLBuffer::~GLBuffer() {
    if (_buffer != 0) {
        glDeleteBuffers(1, &_buffer);
    }
}

void GLBackend::syncGPUObject(const Buffer& buffer) {
    GLBuffer* object = Backend::getGPUObject<GLBackend::GLBuffer>(buffer);

    if (object && (object->_stamp == buffer.getSysmem().getStamp())) {
        return;
    }

    // need to have a gpu object?
    if (!object) {
        object = new GLBuffer();
        glGenBuffers(1, &object->_buffer);
        CHECK_GL_ERROR();
        Backend::setGPUObject(buffer, object);
    }

    // Now let's update the content of the bo with the sysmem version
    // TODO: in the future, be smarter about when to actually upload the glBO version based on the data that did change
    //if () {
    glBindBuffer(GL_ARRAY_BUFFER, object->_buffer);
    glBufferData(GL_ARRAY_BUFFER, buffer.getSysmem().getSize(), buffer.getSysmem().readData(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    object->_stamp = buffer.getSysmem().getStamp();
    object->_size = buffer.getSysmem().getSize();
    //}
    CHECK_GL_ERROR();
}



GLuint GLBackend::getBufferID(const Buffer& buffer) {
    GLBackend::syncGPUObject(buffer);
    return Backend::getGPUObject<GLBackend::GLBuffer>(buffer)->_buffer;
}
