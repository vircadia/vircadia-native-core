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

#define DO_IT_NOW(call, offset) int param = _params.size() - (offset); do##call(param);

using namespace gpu;

Batch::Batch() :
    _commands(),
    _params(),
    _resources(){
}

Batch::~Batch() {
}

void Batch::clear() {
    _commands.clear();
    _params.clear();
    _resources.clear();
}

void Batch::draw( Primitive primitiveType, int nbVertices, int startVertex) {
    _commands.push_back(COMMAND_DRAW);
    _params.push_back(startVertex);
    _params.push_back(nbVertices);
    _params.push_back(primitiveType);
}

void Batch::drawIndexed( Primitive primitiveType, int nbIndices, int startIndex) {
    _commands.push_back(COMMAND_DRAW_INDEXED);
    _params.push_back(startIndex);
    _params.push_back(nbIndices);
    _params.push_back(primitiveType);
}

void Batch::drawInstanced( uint32 nbInstances, Primitive primitiveType, int nbVertices, int startVertex, int startInstance) {
    _commands.push_back(COMMAND_DRAW_INSTANCED);
    _params.push_back(startInstance);
    _params.push_back(startVertex);
    _params.push_back(nbVertices);
    _params.push_back(primitiveType);
    _params.push_back(nbInstances);
}

void Batch::drawIndexedInstanced( uint32 nbInstances, Primitive primitiveType, int nbIndices, int startIndex, int startInstance) {
    _commands.push_back(COMMAND_DRAW_INDEXED_INSTANCED);
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
    _commands.push_back(COMMAND_glEnable);
    _params.push_back(cap);

    DO_IT_NOW(_glEnable, 1);
}
void Batch::do_glEnable(int &paramOffset) {
    glEnable(_params[paramOffset++]._uint);
}

void Batch::_glDisable(GLenum cap) {
    _commands.push_back(COMMAND_glDisable);
    _params.push_back(cap);

    DO_IT_NOW(_glDisable, 1);
}
void Batch::do_glDisable(int &paramOffset) {
    glDisable(_params[paramOffset++]._uint);
}

void Batch::_glEnableClientState(GLenum array) {
    _commands.push_back(COMMAND_glEnableClientState);
    _params.push_back(array);

    DO_IT_NOW(_glEnableClientState, 1 );
}
void Batch::do_glEnableClientState(int &paramOffset) {
    glEnableClientState(_params[paramOffset++]._uint);
}

void Batch::_glDisableClientState(GLenum array) {
    _commands.push_back(COMMAND_glDisableClientState);
    _params.push_back(array);

    DO_IT_NOW(_glDisableClientState, 1);
}
void Batch::do_glDisableClientState(int &paramOffset) {
    glDisableClientState(_params[paramOffset++]._uint);
}

void Batch::_glCullFace(GLenum mode) {
    _commands.push_back(COMMAND_glCullFace);
    _params.push_back(mode);

    DO_IT_NOW(_glCullFace, 1);
}
void Batch::do_glCullFace(int &paramOffset) {
    glCullFace(_params[paramOffset++]._uint);
}

void Batch::_glAlphaFunc(GLenum func, GLclampf ref) {
    _commands.push_back(COMMAND_glAlphaFunc);
    _params.push_back(ref);
    _params.push_back(func);

    DO_IT_NOW(_glAlphaFunc, 1);
}
void Batch::do_glAlphaFunc(int &paramOffset) {
    glAlphaFunc(_params[paramOffset++]._uint, _params[paramOffset++]._float);
}

void Batch::_glDepthFunc(GLenum func) {
    _commands.push_back(COMMAND_glDepthFunc);
    _params.push_back(func);

    DO_IT_NOW(_glDepthFunc, 1);
}
void Batch::do_glDepthFunc(int &paramOffset) {
    glDepthFunc(_params[paramOffset++]._uint);
}

void Batch::_glDepthMask(GLboolean flag) {
    _commands.push_back(COMMAND_glDepthMask);
    _params.push_back(flag);

    DO_IT_NOW(_glDepthMask, 1);
}
void Batch::do_glDepthMask(int &paramOffset) {
    glDepthMask(_params[paramOffset++]._uint);
}

void Batch::_glDepthRange(GLclampd zNear, GLclampd zFar) {
    _commands.push_back(COMMAND_glDepthRange);
    _params.push_back(zFar);
    _params.push_back(zNear);

    DO_IT_NOW(_glDepthRange, 2);
}
void Batch::do_glDepthRange(int &paramOffset) {
    glDepthRange(_params[paramOffset++]._double, _params[paramOffset++]._double);
}

void Batch::_glBindBuffer(GLenum target, GLuint buffer) {
    _commands.push_back(COMMAND_glBindBuffer);
    _params.push_back(buffer);
    _params.push_back(target);

    DO_IT_NOW(_glBindBuffer, 2);
}
void Batch::do_glBindBuffer(int &paramOffset) {
    glBindBuffer(_params[paramOffset++]._uint, _params[paramOffset++]._uint);
}

void Batch::_glBindTexture(GLenum target, GLuint texture) {
    _commands.push_back(COMMAND_glBindTexture);
    _params.push_back(texture);
    _params.push_back(target);

    DO_IT_NOW(_glBindTexture, 2);
}
void Batch::do_glBindTexture(int &paramOffset) {
    glBindTexture(_params[paramOffset++]._uint, _params[paramOffset++]._uint);
}

void Batch::_glActiveTexture(GLenum texture) {
    _commands.push_back(COMMAND_glActiveTexture);
    _params.push_back(texture);

    DO_IT_NOW(_glActiveTexture, 1);
}
void Batch::do_glActiveTexture(int &paramOffset) {
    glActiveTexture(_params[paramOffset++]._uint);
}

void Batch::_glDrawBuffers(GLsizei n, const GLenum* bufs) {
    _commands.push_back(COMMAND_glDrawBuffers);
    _params.push_back(bufs);
    _params.push_back(n);

    DO_IT_NOW(_glDrawBuffers, 2);
}
void Batch::do_glDrawBuffers(int &paramOffset) {
    glDrawBuffers(_params[paramOffset++]._uint, (const GLenum*) _params[paramOffset++]._constPointer);
}

void Batch::_glUseProgram(GLuint program) {
    _commands.push_back(COMMAND_glUseProgram);
    _params.push_back(program);

    DO_IT_NOW(_glUseProgram, 1);
}
void Batch::do_glUseProgram(int &paramOffset) {
    glUseProgram(_params[paramOffset++]._uint);
}

void Batch::_glUniform1f(GLint location, GLfloat v0) {
    _commands.push_back(COMMAND_glUniform1f);
    _params.push_back(v0);
    _params.push_back(location);

    DO_IT_NOW(_glUniform1f, 1);
}
void Batch::do_glUniform1f(int &paramOffset) {
    glUniform1f(_params[paramOffset++]._float);
}

void Batch::_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    _commands.push_back(COMMAND_glUniformMatrix4fv);
    _params.push_back(value);
    _params.push_back(transpose);
    _params.push_back(count);
    _params.push_back(location);

    DO_IT_NOW(_glUniformMatrix4fv, 4);
}
void Batch::do_glUniformMatrix4fv(int &paramOffset) {
    glUniformMatrix4fv(_params[paramOffset++]._int, _params[paramOffset++]._uint, _params[paramOffset++]._uint, _params[paramOffset++]._constPointer);
}

void Batch::_glMatrixMode(GLenum mode) {
    _commands.push_back(COMMAND_glMatrixMode);
    _params.push_back(mode);

    DO_IT_NOW(_glMatrixMode, 1);
}
void Batch::do_glMatrixMode(int &paramOffset) {
    glMatrixMode(_params[paramOffset++]._uint);
}

void Batch::_glPushMatrix() {
    _commands.push_back(COMMAND_glPushMatrix);

    DO_IT_NOW(_glPushMatrix, 0);
}
void Batch::do_glPushMatrix(int &paramOffset) {
    glPushMatrix();
}

void Batch::_glPopMatrix() {
    _commands.push_back(COMMAND_glPopMatrix);

    DO_IT_NOW(_glPopMatrix, 0);
}
void Batch::do_glPopMatrix(int &paramOffset) {
    glPopMatrix();
}

void Batch::_glMultMatrixf(const GLfloat *m) {
    _commands.push_back(COMMAND_glMultMatrixf);
    _params.push_back(m);

    DO_IT_NOW(_glMultMatrixf, 1);
}
void Batch::do_glMultMatrixf(int &paramOffset) {
    glMultMatrixf((const GLfloat*) _params[paramOffset++]._constPointer);
}

void Batch::_glLoadMatrixf(const GLfloat *m) {
    _commands.push_back(COMMAND_glLoadMatrixf);
    _params.push_back(m);

    DO_IT_NOW(_glLoadMatrixf, 1);
}
void Batch::do_glLoadMatrixf(int &paramOffset) {
    glLoadMatrixf((const GLfloat*)_params[paramOffset++]._constPointer);
}

void Batch::_glLoadIdentity(void) {
    _commands.push_back(COMMAND_glLoadIdentity);

    DO_IT_NOW(_glLoadIdentity, 0);
}
void Batch::do_glLoadIdentity(int &paramOffset) {
    glLoadIdentity();
}

void Batch::_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    _commands.push_back(COMMAND_glRotatef);
    _params.push_back(z);
    _params.push_back(y);
    _params.push_back(x);
    _params.push_back(angle);

    DO_IT_NOW(_glRotatef, 4);
}
void Batch::do_glRotatef(int &paramOffset) {
    glRotatef(_params[paramOffset++]._float, _params[paramOffset++]._float, _params[paramOffset++]._float, _params[paramOffset++]._float);
}

void Batch::_glScalef(GLfloat x, GLfloat y, GLfloat z) {
    _commands.push_back(COMMAND_glScalef);
    _params.push_back(z);
    _params.push_back(y);
    _params.push_back(x);

    DO_IT_NOW(_glScalef, 3);
}
void Batch::do_glScalef(int &paramOffset) {
    glScalef(_params[paramOffset++]._float, _params[paramOffset++]._float, _params[paramOffset++]._float);
}

void Batch::_glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    _commands.push_back(COMMAND_glTranslatef);
    _params.push_back(z);
    _params.push_back(y);
    _params.push_back(x);

    DO_IT_NOW(_glTranslatef, 3);
}
void Batch::do_glTranslatef(int &paramOffset) {
    glTranslatef(_params[paramOffset++]._float, _params[paramOffset++]._float, _params[paramOffset++]._float);
}

void Batch::_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    _commands.push_back(COMMAND_glDrawArrays);
    _params.push_back(count);
    _params.push_back(first);
    _params.push_back(mode);

    DO_IT_NOW(_glDrawArrays, 3);
}
void Batch::do_glDrawArrays(int &paramOffset) {
    glDrawArrays(_params[paramOffset++]._uint, _params[paramOffset++]._int, _params[paramOffset++]._int);
}

void Batch::_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {
    _commands.push_back(COMMAND_glDrawRangeElements);
    _params.push_back(indices);
    _params.push_back(type);
    _params.push_back(count);
    _params.push_back(end);
    _params.push_back(start);
    _params.push_back(mode);

    DO_IT_NOW(_glDrawRangeElements, 6);
}
void Batch::do_glDrawRangeElements(int &paramOffset) {
    glDrawRangeElements(_params[paramOffset++]._uint, _params[paramOffset++]._uint, _params[paramOffset++]._uint, _params[paramOffset++]._int, _params[paramOffset++]._uint, _params[paramOffset++]._constPointer);
}

void Batch::_glColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    _commands.push_back(COMMAND_glColorPointer);
    _params.push_back(pointer);
    _params.push_back(stride);
    _params.push_back(type);
    _params.push_back(size);

    DO_IT_NOW(_glColorPointer, 4);
}
void Batch::do_glColorPointer(int &paramOffset) {
    glColorPointer(_params[paramOffset++]._int, _params[paramOffset++]._uint, _params[paramOffset++]._int, _params[paramOffset++]._constPointer);
}

void Batch::_glNormalPointer(GLenum type, GLsizei stride, const void *pointer) {
    _commands.push_back(COMMAND_glNormalPointer);
    _params.push_back(pointer);
    _params.push_back(stride);
    _params.push_back(type);

    DO_IT_NOW(_glNormalPointer, 4);
}
void Batch::do_glNormalPointer(int &paramOffset) {
    glNormalPointer(_params[paramOffset++]._uint, _params[paramOffset++]._int, _params[paramOffset++]._constPointer);
}

void Batch::_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    _commands.push_back(COMMAND_glTexCoordPointer);
    _params.push_back(pointer);
    _params.push_back(stride);
    _params.push_back(type);
    _params.push_back(size);
}
void Batch::do_glCullFace(int &paramOffset) {
    glCullFace(_params[paramOffset++]._uint);
}

void Batch::_glVertexPointer(GLint size, GLenum type, GLsizei stride,  const void *pointer) {
    _commands.push_back(COMMAND_glVertexPointer);
    _params.push_back(pointer);
    _params.push_back(stride);
    _params.push_back(type);
    _params.push_back(size);
}
void Batch::do_glCullFace(int &paramOffset) {
    glCullFace(_params[paramOffset++]._uint);
}


void Batch::_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    _commands.push_back(COMMAND_glVertexPointer);
    _params.push_back(pointer);
    _params.push_back(stride);
    _params.push_back(normalized);
    _params.push_back(type);
    _params.push_back(size);
    _params.push_back(index);
}
void Batch::do_glCullFace(int &paramOffset) {
    glCullFace(_params[paramOffset++]._uint);
}

void Batch::_glEnableVertexArrayAttrib(GLint location) {
    _commands.push_back(COMMAND_glEnableVertexArrayAttrib);
    _params.push_back(location);
}
void Batch::do_glCullFace(int &paramOffset) {
    glCullFace(_params[paramOffset++]._uint);
}

void Batch::_glDisableVertexArrayAttrib(GLint location) {
    _commands.push_back(COMMAND_glDisableVertexArrayAttrib);
    _params.push_back(location);
}
void Batch::do_glCullFace(int &paramOffset) {
    glCullFace(_params[paramOffset++]._uint);
}

void Batch::_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    _commands.push_back(COMMAND_glColor4f);
    _params.push_back(alpha);
    _params.push_back(blue);
    _params.push_back(green);
    _params.push_back(red);
}
void Batch::do_glCullFace(int &paramOffset) {
    glCullFace(_params[paramOffset++]._uint);
}

void Batch::_glMaterialf(GLenum face, GLenum pname, GLfloat param) {
    _commands.push_back(COMMAND_glMaterialf);
    _params.push_back(param);
    _params.push_back(pname);
    _params.push_back(face);
}
void Batch::do_glCullFace(int &paramOffset) {
    glCullFace(_params[paramOffset++]._uint);
}

void Batch::_glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
    _commands.push_back(COMMAND_glMaterialfv);
    _params.push_back(params);
    _params.push_back(pname);
    _params.push_back(face);
}
void Batch::do_glCullFace(int &paramOffset) {
    glCullFace(_params[paramOffset++]._uint);
}


