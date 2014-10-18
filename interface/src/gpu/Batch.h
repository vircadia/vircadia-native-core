//
//  Batch.h
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/14/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Batch_h
#define hifi_gpu_Batch_h

#include <assert.h>
#include "InterfaceConfig.h"

#include <vector>

namespace gpu {

class Buffer;
class Resource;
typedef int  Stamp;
typedef unsigned int uint32;
typedef int int32;

// TODO: move the backend namespace into dedicated files, for now we keep it close to the gpu objects definition for convenience
namespace backend {

};

enum Primitive {
    PRIMITIVE_POINTS = 0,
    PRIMITIVE_LINES,
    PRIMITIVE_LINE_STRIP,
    PRIMITIVE_TRIANGLES,
    PRIMITIVE_TRIANGLE_STRIP,
    PRIMITIVE_QUADS,
};

class Batch {
public:

    Batch();
    Batch(const Batch& batch);
    ~Batch();

    void clear();

    void draw( Primitive primitiveType, int nbVertices, int startVertex = 0);
    void drawIndexed( Primitive primitiveType, int nbIndices, int startIndex = 0 );
    void drawInstanced( uint32 nbInstances, Primitive primitiveType, int nbVertices, int startVertex = 0, int startInstance = 0);
    void drawIndexedInstanced( uint32 nbInstances, Primitive primitiveType, int nbIndices, int startIndex = 0, int startInstance = 0);

    // TODO: As long as we have gl calls explicitely issued from interface
    // code, we need to be able to record and batch these calls. THe long 
    // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
    void _glEnable(GLenum cap);
    void _glDisable(GLenum cap);

    void _glEnableClientState(GLenum array);
    void _glDisableClientState(GLenum array);

    void _glCullFace(GLenum mode);
    void _glAlphaFunc(GLenum func, GLclampf ref);

    void _glDepthFunc(GLenum func);
    void _glDepthMask(GLboolean flag);
    void _glDepthRange(GLclampd zNear, GLclampd zFar);

    void _glBindBuffer(GLenum target, GLuint buffer);

    void _glBindTexture(GLenum target, GLuint texture);
    void _glActiveTexture(GLenum texture);

    void _glDrawBuffers(GLsizei n, const GLenum* bufs);

    void _glUseProgram(GLuint program);
    void _glUniform1f(GLint location, GLfloat v0);
    void _glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

    void _glMatrixMode(GLenum mode); 
    void _glPushMatrix();
    void _glPopMatrix();
    void _glMultMatrixf(const GLfloat *m);
    void _glLoadMatrixf(const GLfloat *m);
    void _glLoadIdentity(void);
    void _glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
    void _glScalef(GLfloat x, GLfloat y, GLfloat z);
    void _glTranslatef(GLfloat x, GLfloat y, GLfloat z);

    void _glDrawArrays(GLenum mode, GLint first, GLsizei count);
    void _glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);

    void _glColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer);
    void _glNormalPointer(GLenum type, GLsizei stride, const void *pointer);
    void _glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer);
    void _glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer);

    void _glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
    void _glEnableVertexArrayAttrib(GLint location);
    void _glDisableVertexArrayAttrib(GLint location);

    void _glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);

    void _glMaterialf(GLenum face, GLenum pname, GLfloat param);
    void _glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);

protected:

    enum Command {
        COMMAND_DRAW = 0,
        COMMAND_DRAW_INDEXED,
        COMMAND_DRAW_INSTANCED,
        COMMAND_DRAW_INDEXED_INSTANCED,
        
        COMMAND_SET_PIPE_STATE,
        COMMAND_SET_VIEWPORT,
        COMMAND_SET_FRAMEBUFFER,
        COMMAND_SET_RESOURCE,
        COMMAND_SET_VERTEX_STREAM,
        COMMAND_SET_INDEX_STREAM,

        // TODO: As long as we have gl calls explicitely issued from interface
        // code, we need to be able to record and batch these calls. THe long 
        // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
        COMMAND_glEnable,
        COMMAND_glDisable,

        COMMAND_glEnableClientState,
        COMMAND_glDisableClientState,

        COMMAND_glCullFace,
        COMMAND_glAlphaFunc,

        COMMAND_glDepthFunc,
        COMMAND_glDepthMask,
        COMMAND_glDepthRange,

        COMMAND_glBindBuffer,

        COMMAND_glBindTexture,
        COMMAND_glActiveTexture,

        COMMAND_glDrawBuffers,

        COMMAND_glUseProgram,
        COMMAND_glUniform1f,
        COMMAND_glUniformMatrix4fv,

        COMMAND_glMatrixMode,
        COMMAND_glPushMatrix,
        COMMAND_glPopMatrix,
        COMMAND_glMultMatrixf,
        COMMAND_glLoadMatrixf,
        COMMAND_glLoadIdentity,
        COMMAND_glRotatef,
        COMMAND_glScalef,
        COMMAND_glTranslatef,

        COMMAND_glDrawArrays,
        COMMAND_glDrawRangeElements,

        COMMAND_glColorPointer,
        COMMAND_glNormalPointer,
        COMMAND_glTexCoordPointer,
        COMMAND_glVertexPointer,

        COMMAND_glVertexAttribPointer,
        COMMAND_glEnableVertexArrayAttrib,
        COMMAND_glDisableVertexArrayAttrib,

        COMMAND_glColor4f,

        COMMAND_glMaterialf,
        COMMAND_glMaterialfv,
    };
    typedef std::vector<Command> Commands;

    class Param {
    public:
        union {
            int32 _int;
            uint32 _uint;
            float   _float;
            char _chars[4];
            const void* _constPointer;
            double _double;
        };
        Param(int32 val) : _int(val) {}
        Param(uint32 val) : _uint(val) {}
        Param(float val) : _float(val) {}
        Param(const void* val) : _constPointer(val) {}
        Param(double val) : _double(val) {}
    };
    typedef std::vector<Param> Params;

    class ResourceCache {
    public:
        Resource* _resource;
    };
    typedef std::vector<ResourceCache> Resources;

    Commands _commands;
    Params _params;
    Resources _resources;

    // TODO: As long as we have gl calls explicitely issued from interface
    // code, we need to be able to record and batch these calls. THe long 
    // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
    void do_glEnable(int &paramOffset);
    void do_glDisable(int &paramOffset);

    void do_glEnableClientState(int &paramOffset);
    void do_glDisableClientState(int &paramOffset);

    void do_glCullFace(int &paramOffset);
    void do_glAlphaFunc(int &paramOffset);

    void do_glDepthFunc(int &paramOffset);
    void do_glDepthMask(int &paramOffset);
    void do_glDepthRange(int &paramOffset);

    void do_glBindBuffer(int &paramOffset);

    void do_glBindTexture(int &paramOffset);
    void do_glActiveTexture(int &paramOffset);

    void do_glDrawBuffers(int &paramOffset);

    void do_glUseProgram(int &paramOffset);
    void do_glUniform1f(int &paramOffset);
    void do_glUniformMatrix4fv(int &paramOffset);

    void do_glMatrixMode(int &paramOffset);
    void do_glPushMatrix(int &paramOffset);
    void do_glPopMatrix(int &paramOffset);
    void do_glMultMatrixf(int &paramOffset);
    void do_glLoadMatrixf(int &paramOffset);
    void do_glLoadIdentity(int &paramOffset);
    void do_glRotatef(int &paramOffset);
    void do_glScalef(int &paramOffset);
    void do_glTranslatef(int &paramOffset);

    void do_glDrawArrays(int &paramOffset);
    void do_glDrawRangeElements(int &paramOffset);

    void do_glColorPointer(int &paramOffset);
    void do_glNormalPointer(int &paramOffset);
    void do_glTexCoordPointer(int &paramOffset);
    void do_glVertexPointer(int &paramOffset);

    void do_glVertexAttribPointer(int &paramOffset);
    void do_glEnableVertexArrayAttrib(int &paramOffset);
    void do_glDisableVertexArrayAttrib(int &paramOffset);

    void do_glColor4f(int &paramOffset);

    void do_glMaterialf(int &paramOffset);
    void do_glMaterialfv(int &paramOffset);
};

};

#endif
