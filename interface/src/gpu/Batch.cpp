//
//  Batch.cpp
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/14/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Batch.h"

#include <QDebug>

#define ADD_COMMAND(call) _commands.push_back(COMMAND_##call); _commandCalls.push_back(&gpu::Batch::do_##call); _commandOffsets.push_back(_params.size());

//#define DO_IT_NOW(call, offset) runLastCommand();
#define DO_IT_NOW(call, offset) 

#define CHECK_GL_ERROR() ::gpu::backend::checkGLError()

using namespace gpu;

Batch::Batch() :
    _commands(),
    _commandCalls(),
    _commandOffsets(),
    _params(),
    _resources(),
    _data(){
}

Batch::~Batch() {
}

void Batch::clear() {
    _commands.clear();
    _commandCalls.clear();
    _commandOffsets.clear();
    _params.clear();
    _resources.clear();
    _data.clear();
}

uint32 Batch::cacheResource(Resource* res) {
    uint32 offset = _resources.size();
    _resources.push_back(ResourceCache(res));
    
    return offset;
}

uint32 Batch::cacheResource(const void* pointer) {
    uint32 offset = _resources.size();
    _resources.push_back(ResourceCache(pointer));

    return offset;
}

uint32 Batch::cacheData(uint32 size, const void* data) {
    uint32 offset = _data.size();
    uint32 nbBytes = size;
    _data.resize(offset + nbBytes);
    memcpy(_data.data() + offset, data, size);

    return offset;
}

#define CASE_COMMAND(call) case COMMAND_##call: { do_##call(offset); } break;

void Batch::runCommand(Command com, uint32 offset) {
    switch (com) {
        CASE_COMMAND(draw);
        CASE_COMMAND(drawIndexed);
        CASE_COMMAND(drawInstanced);
        CASE_COMMAND(drawIndexedInstanced);
        CASE_COMMAND(glEnable);
        CASE_COMMAND(glDisable);
        CASE_COMMAND(glEnableClientState);
        CASE_COMMAND(glDisableClientState);
        CASE_COMMAND(glCullFace);
        CASE_COMMAND(glAlphaFunc);
        CASE_COMMAND(glDepthFunc);
        CASE_COMMAND(glDepthMask);
        CASE_COMMAND(glDepthRange);
        CASE_COMMAND(glBindBuffer);
        CASE_COMMAND(glBindTexture);
        CASE_COMMAND(glActiveTexture);
        CASE_COMMAND(glDrawBuffers);
        CASE_COMMAND(glUseProgram);
        CASE_COMMAND(glUniform1f);
        CASE_COMMAND(glUniformMatrix4fv);
        CASE_COMMAND(glMatrixMode);
        CASE_COMMAND(glPushMatrix);
        CASE_COMMAND(glPopMatrix);
        CASE_COMMAND(glMultMatrixf);
        CASE_COMMAND(glLoadMatrixf);
        CASE_COMMAND(glLoadIdentity);
        CASE_COMMAND(glRotatef);
        CASE_COMMAND(glScalef);
        CASE_COMMAND(glTranslatef);
        CASE_COMMAND(glDrawArrays);
        CASE_COMMAND(glDrawRangeElements);
        CASE_COMMAND(glColorPointer);
        CASE_COMMAND(glNormalPointer);
        CASE_COMMAND(glTexCoordPointer);
        CASE_COMMAND(glVertexPointer);
        CASE_COMMAND(glVertexAttribPointer);
        CASE_COMMAND(glEnableVertexAttribArray);
        CASE_COMMAND(glDisableVertexAttribArray);
        CASE_COMMAND(glColor4f);
        CASE_COMMAND(glMaterialf);
        CASE_COMMAND(glMaterialfv);
    }
}

void Batch::draw(Primitive primitiveType, int nbVertices, int startVertex) {
    ADD_COMMAND(draw);

    _params.push_back(startVertex);
    _params.push_back(nbVertices);
    _params.push_back(primitiveType);
}

void Batch::drawIndexed(Primitive primitiveType, int nbIndices, int startIndex) {
    ADD_COMMAND(drawIndexed);

    _params.push_back(startIndex);
    _params.push_back(nbIndices);
    _params.push_back(primitiveType);
}

void Batch::drawInstanced(uint32 nbInstances, Primitive primitiveType, int nbVertices, int startVertex, int startInstance) {
    ADD_COMMAND(drawInstanced);

    _params.push_back(startInstance);
    _params.push_back(startVertex);
    _params.push_back(nbVertices);
    _params.push_back(primitiveType);
    _params.push_back(nbInstances);
}

void Batch::drawIndexedInstanced(uint32 nbInstances, Primitive primitiveType, int nbIndices, int startIndex, int startInstance) {
    ADD_COMMAND(drawIndexedInstanced);

    _params.push_back(startInstance);
    _params.push_back(startIndex);
    _params.push_back(nbIndices);
    _params.push_back(primitiveType);
    _params.push_back(nbInstances);
}

// TODO: As long as we have gl calls explicitely issued from interface
// code, we need to be able to record and batch these calls. THe long 
// term strategy is to get rid of any GL calls in favor of the HIFI GPU API

void Batch::_glEnable(GLenum cap) {
    ADD_COMMAND(glEnable);

    _params.push_back(cap);

    DO_IT_NOW(_glEnable, 1);
}
void Batch::do_glEnable(uint32 paramOffset) {
    glEnable(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDisable(GLenum cap) {
    ADD_COMMAND(glDisable);

    _params.push_back(cap);

    DO_IT_NOW(_glDisable, 1);
}
void Batch::do_glDisable(uint32 paramOffset) {
    glDisable(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glEnableClientState(GLenum array) {
    ADD_COMMAND(glEnableClientState);

    _params.push_back(array);

    DO_IT_NOW(_glEnableClientState, 1 );
}
void Batch::do_glEnableClientState(uint32 paramOffset) {
    glEnableClientState(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDisableClientState(GLenum array) {
    ADD_COMMAND(glDisableClientState);

    _params.push_back(array);

    DO_IT_NOW(_glDisableClientState, 1);
}
void Batch::do_glDisableClientState(uint32 paramOffset) {
    glDisableClientState(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glCullFace(GLenum mode) {
    ADD_COMMAND(glCullFace);

    _params.push_back(mode);

    DO_IT_NOW(_glCullFace, 1);
}
void Batch::do_glCullFace(uint32 paramOffset) {
    glCullFace(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glAlphaFunc(GLenum func, GLclampf ref) {
    ADD_COMMAND(glAlphaFunc);

    _params.push_back(ref);
    _params.push_back(func);

    DO_IT_NOW(_glAlphaFunc, 2);
}
void Batch::do_glAlphaFunc(uint32 paramOffset) {
    glAlphaFunc(
        _params[paramOffset + 1]._uint,
        _params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glDepthFunc(GLenum func) {
    ADD_COMMAND(glDepthFunc);

    _params.push_back(func);

    DO_IT_NOW(_glDepthFunc, 1);
}
void Batch::do_glDepthFunc(uint32 paramOffset) {
    glDepthFunc(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDepthMask(GLboolean flag) {
    ADD_COMMAND(glDepthMask);

    _params.push_back(flag);

    DO_IT_NOW(_glDepthMask, 1);
}
void Batch::do_glDepthMask(uint32 paramOffset) {
    glDepthMask(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDepthRange(GLclampd zNear, GLclampd zFar) {
    ADD_COMMAND(glDepthRange);

    _params.push_back(zFar);
    _params.push_back(zNear);

    DO_IT_NOW(_glDepthRange, 2);
}
void Batch::do_glDepthRange(uint32 paramOffset) {
    glDepthRange(
        _params[paramOffset + 1]._double,
        _params[paramOffset + 0]._double);
    CHECK_GL_ERROR();
}

void Batch::_glBindBuffer(GLenum target, GLuint buffer) {
    ADD_COMMAND(glBindBuffer);

    _params.push_back(buffer);
    _params.push_back(target);

    DO_IT_NOW(_glBindBuffer, 2);
}
void Batch::do_glBindBuffer(uint32 paramOffset) {
    glBindBuffer(
        _params[paramOffset + 1]._uint,
        _params[paramOffset + 0]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glBindTexture(GLenum target, GLuint texture) {
    ADD_COMMAND(glBindTexture);

    _params.push_back(texture);
    _params.push_back(target);

    DO_IT_NOW(_glBindTexture, 2);
}
void Batch::do_glBindTexture(uint32 paramOffset) {
    glBindTexture(
        _params[paramOffset + 1]._uint,
        _params[paramOffset + 0]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glActiveTexture(GLenum texture) {
    ADD_COMMAND(glActiveTexture);

    _params.push_back(texture);

    DO_IT_NOW(_glActiveTexture, 1);
}
void Batch::do_glActiveTexture(uint32 paramOffset) {
    glActiveTexture(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDrawBuffers(GLsizei n, const GLenum* bufs) {
    ADD_COMMAND(glDrawBuffers);

    _params.push_back(cacheData(n * sizeof(GLenum), bufs));
    _params.push_back(n);

    DO_IT_NOW(_glDrawBuffers, 2);
}
void Batch::do_glDrawBuffers(uint32 paramOffset) {
    glDrawBuffers(
        _params[paramOffset + 1]._uint,
        (const GLenum*) editData(_params[paramOffset + 0]._uint));
    CHECK_GL_ERROR();
}

void Batch::_glUseProgram(GLuint program) {
    ADD_COMMAND(glUseProgram);

    _params.push_back(program);

    DO_IT_NOW(_glUseProgram, 1);
}
void Batch::do_glUseProgram(uint32 paramOffset) {
    glUseProgram(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glUniform1f(GLint location, GLfloat v0) {
    ADD_COMMAND(glUniform1f);

    _params.push_back(v0);
    _params.push_back(location);

    DO_IT_NOW(_glUniform1f, 1);
}
void Batch::do_glUniform1f(uint32 paramOffset) {
    glUniform1f(
        _params[paramOffset + 1]._int,
        _params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    ADD_COMMAND(glUniformMatrix4fv);

    const int MATRIX4_SIZE = 16 * sizeof(float);
    _params.push_back(cacheData(count * MATRIX4_SIZE, value));
    _params.push_back(transpose);
    _params.push_back(count);
    _params.push_back(location);

    DO_IT_NOW(_glUniformMatrix4fv, 4);
}
void Batch::do_glUniformMatrix4fv(uint32 paramOffset) {
    glUniformMatrix4fv(
        _params[paramOffset + 3]._int,
        _params[paramOffset + 2]._uint,
        _params[paramOffset + 1]._uint,
        (const GLfloat*) editData(_params[paramOffset + 0]._uint));
    CHECK_GL_ERROR();
}

void Batch::_glMatrixMode(GLenum mode) {
    ADD_COMMAND(glMatrixMode);

    _params.push_back(mode);

    DO_IT_NOW(_glMatrixMode, 1);
}
void Batch::do_glMatrixMode(uint32 paramOffset) {
    glMatrixMode(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glPushMatrix() {
    ADD_COMMAND(glPushMatrix);

    DO_IT_NOW(_glPushMatrix, 0);
}
void Batch::do_glPushMatrix(uint32 paramOffset) {
    glPushMatrix();
    CHECK_GL_ERROR();
}

void Batch::_glPopMatrix() {
    ADD_COMMAND(glPopMatrix);

    DO_IT_NOW(_glPopMatrix, 0);
}
void Batch::do_glPopMatrix(uint32 paramOffset) {
    glPopMatrix();
    CHECK_GL_ERROR();
}

void Batch::_glMultMatrixf(const GLfloat *m) {
    ADD_COMMAND(glMultMatrixf);

    const int MATRIX4_SIZE = 16 * sizeof(float);
    _params.push_back(cacheData(MATRIX4_SIZE, m));

    DO_IT_NOW(_glMultMatrixf, 1);
}
void Batch::do_glMultMatrixf(uint32 paramOffset) {
    glMultMatrixf((const GLfloat*) editData(_params[paramOffset]._uint));
    CHECK_GL_ERROR();
}

void Batch::_glLoadMatrixf(const GLfloat *m) {
    ADD_COMMAND(glLoadMatrixf);

    const int MATRIX4_SIZE = 16 * sizeof(float);
    _params.push_back(cacheData(MATRIX4_SIZE, m));

    DO_IT_NOW(_glLoadMatrixf, 1);
}
void Batch::do_glLoadMatrixf(uint32 paramOffset) {
    glLoadMatrixf((const GLfloat*)editData(_params[paramOffset]._uint));
    CHECK_GL_ERROR();
}

void Batch::_glLoadIdentity(void) {
    ADD_COMMAND(glLoadIdentity);

    DO_IT_NOW(_glLoadIdentity, 0);
}
void Batch::do_glLoadIdentity(uint32 paramOffset) {
    glLoadIdentity();
    CHECK_GL_ERROR();
}

void Batch::_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    ADD_COMMAND(glRotatef);

    _params.push_back(z);
    _params.push_back(y);
    _params.push_back(x);
    _params.push_back(angle);

    DO_IT_NOW(_glRotatef, 4);
}
void Batch::do_glRotatef(uint32 paramOffset) {
    glRotatef(
        _params[paramOffset + 3]._float,
        _params[paramOffset + 2]._float,
        _params[paramOffset + 1]._float,
        _params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glScalef(GLfloat x, GLfloat y, GLfloat z) {
    ADD_COMMAND(glScalef);

    _params.push_back(z);
    _params.push_back(y);
    _params.push_back(x);

    DO_IT_NOW(_glScalef, 3);
}
void Batch::do_glScalef(uint32 paramOffset) {
    glScalef(
        _params[paramOffset + 2]._float,
        _params[paramOffset + 1]._float,
        _params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    ADD_COMMAND(glTranslatef);

    _params.push_back(z);
    _params.push_back(y);
    _params.push_back(x);

    DO_IT_NOW(_glTranslatef, 3);
}
void Batch::do_glTranslatef(uint32 paramOffset) {
    glTranslatef(
        _params[paramOffset + 2]._float,
        _params[paramOffset + 1]._float,
        _params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    ADD_COMMAND(glDrawArrays);

    _params.push_back(count);
    _params.push_back(first);
    _params.push_back(mode);

    DO_IT_NOW(_glDrawArrays, 3);
}
void Batch::do_glDrawArrays(uint32 paramOffset) {
    glDrawArrays(
        _params[paramOffset + 2]._uint,
        _params[paramOffset + 1]._int,
        _params[paramOffset + 0]._int);
    CHECK_GL_ERROR();
}

void Batch::_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {
    ADD_COMMAND(glDrawRangeElements);

    _params.push_back(cacheResource(indices));
    _params.push_back(type);
    _params.push_back(count);
    _params.push_back(end);
    _params.push_back(start);
    _params.push_back(mode);

    DO_IT_NOW(_glDrawRangeElements, 6);
}
void Batch::do_glDrawRangeElements(uint32 paramOffset) {
    glDrawRangeElements(
        _params[paramOffset + 5]._uint,
        _params[paramOffset + 4]._uint,
        _params[paramOffset + 3]._uint,
        _params[paramOffset + 2]._int,
        _params[paramOffset + 1]._uint,
        editResource(_params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}

void Batch::_glColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    ADD_COMMAND(glColorPointer);

    _params.push_back(cacheResource(pointer));
    _params.push_back(stride);
    _params.push_back(type);
    _params.push_back(size);

    DO_IT_NOW(_glColorPointer, 4);
}
void Batch::do_glColorPointer(uint32 paramOffset) {
    glColorPointer(
        _params[paramOffset + 3]._int,
        _params[paramOffset + 2]._uint,
        _params[paramOffset + 1]._int,
        editResource(_params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}

void Batch::_glNormalPointer(GLenum type, GLsizei stride, const void *pointer) {
    ADD_COMMAND(glNormalPointer);

    _params.push_back(cacheResource(pointer));
    _params.push_back(stride);
    _params.push_back(type);

    DO_IT_NOW(_glNormalPointer, 3);
}
void Batch::do_glNormalPointer(uint32 paramOffset) {
    glNormalPointer(
        _params[paramOffset + 2]._uint,
        _params[paramOffset + 1]._int,
        editResource(_params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}

void Batch::_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    ADD_COMMAND(glTexCoordPointer);

    _params.push_back(cacheResource(pointer));
    _params.push_back(stride);
    _params.push_back(type);
    _params.push_back(size);

    DO_IT_NOW(_glTexCoordPointer, 4);
}
void Batch::do_glTexCoordPointer(uint32 paramOffset) {
    glTexCoordPointer(
        _params[paramOffset + 3]._int,
        _params[paramOffset + 2]._uint,
        _params[paramOffset + 1]._int,
        editResource(_params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}

void Batch::_glVertexPointer(GLint size, GLenum type, GLsizei stride,  const void *pointer) {
    ADD_COMMAND(glVertexPointer);

    _params.push_back(cacheResource(pointer));
    _params.push_back(stride);
    _params.push_back(type);
    _params.push_back(size);

    DO_IT_NOW(_glVertexPointer, 4);
}
void Batch::do_glVertexPointer(uint32 paramOffset) {
    glVertexPointer(
        _params[paramOffset + 3]._int,
        _params[paramOffset + 2]._uint,
        _params[paramOffset + 1]._int,
        editResource(_params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}


void Batch::_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    ADD_COMMAND(glVertexAttribPointer);

    _params.push_back(cacheResource(pointer));
    _params.push_back(stride);
    _params.push_back(normalized);
    _params.push_back(type);
    _params.push_back(size);
    _params.push_back(index);

    DO_IT_NOW(_glVertexAttribPointer, 6);
}
void Batch::do_glVertexAttribPointer(uint32 paramOffset) {
    glVertexAttribPointer(
        _params[paramOffset + 5]._uint,
        _params[paramOffset + 4]._int,
        _params[paramOffset + 3]._uint,
        _params[paramOffset + 2]._uint,
        _params[paramOffset + 1]._int,
        editResource(_params[paramOffset + 0]._uint)->_pointer);
    CHECK_GL_ERROR();
}

void Batch::_glEnableVertexAttribArray(GLint location) {
    ADD_COMMAND(glEnableVertexAttribArray);

    _params.push_back(location);

    DO_IT_NOW(_glEnableVertexAttribArray, 1);
}
void Batch::do_glEnableVertexAttribArray(uint32 paramOffset) {
    glEnableVertexAttribArray(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glDisableVertexAttribArray(GLint location) {
    ADD_COMMAND(glDisableVertexAttribArray);

    _params.push_back(location);

    DO_IT_NOW(_glDisableVertexAttribArray, 1);
}
void Batch::do_glDisableVertexAttribArray(uint32 paramOffset) {
    glDisableVertexAttribArray(_params[paramOffset]._uint);
    CHECK_GL_ERROR();
}

void Batch::_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    ADD_COMMAND(glColor4f);

    _params.push_back(alpha);
    _params.push_back(blue);
    _params.push_back(green);
    _params.push_back(red);

    DO_IT_NOW(_glColor4f, 4);
}
void Batch::do_glColor4f(uint32 paramOffset) {
    glColor4f(
        _params[paramOffset + 3]._float,
        _params[paramOffset + 2]._float,
        _params[paramOffset + 1]._float,
        _params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glMaterialf(GLenum face, GLenum pname, GLfloat param) {
    ADD_COMMAND(glMaterialf);

    _params.push_back(param);
    _params.push_back(pname);
    _params.push_back(face);

    DO_IT_NOW(_glMaterialf, 3);
}
void Batch::do_glMaterialf(uint32 paramOffset) {
    glMaterialf(
        _params[paramOffset + 2]._uint,
        _params[paramOffset + 1]._uint,
        _params[paramOffset + 0]._float);
    CHECK_GL_ERROR();
}

void Batch::_glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
    ADD_COMMAND(glMaterialfv);

    _params.push_back(cacheData(4 * sizeof(float), params));
    _params.push_back(pname);
    _params.push_back(face);

    DO_IT_NOW(_glMaterialfv, 3);
}
void Batch::do_glMaterialfv(uint32 paramOffset) {
    glMaterialfv(
        _params[paramOffset + 2]._uint,
        _params[paramOffset + 1]._uint,
        (const GLfloat*) editData(_params[paramOffset + 0]._uint));
    CHECK_GL_ERROR();
}



void backend::renderBatch(Batch& batch) {
    uint32 numCommands = batch._commands.size();
    Batch::CommandCall* call = batch._commandCalls.data();
    Batch::CommandOffsets::value_type* offset = batch._commandOffsets.data();

    for (int i = 0; i < numCommands; i++) {
        (batch.*(*call))(*offset);
        call++;
        offset++;
    }
}

void backend::checkGLError() {
    GLenum error = glGetError();
    if (!error) {
        return;
    } else {
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