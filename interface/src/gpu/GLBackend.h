//
//  GLBackend.h
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_GLBackend_h
#define hifi_gpu_GLBackend_h

#include <assert.h>
#include "InterfaceConfig.h"

#include "gpu/Context.h"
#include "gpu/Batch.h"

namespace gpu {

class GLBackend : public Backend {
public:

    GLBackend();
    ~GLBackend();

    static void renderBatch(Batch& batch);

    static void checkGLError();


    class GLBuffer {
    public:
        Stamp  _stamp;
        GLuint _buffer;
        GLuint _size;

        GLBuffer();
        ~GLBuffer();
    };
    static void syncGPUObject(const Buffer& buffer);

    static GLuint getBufferID(const Buffer& buffer);

protected:

    // TODO: As long as we have gl calls explicitely issued from interface
    // code, we need to be able to record and batch these calls. THe long 
    // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
    void do_glEnable(Batch& batch, uint32 paramOffset);
    void do_glDisable(Batch& batch, uint32 paramOffset);

    void do_glEnableClientState(Batch& batch, uint32 paramOffset);
    void do_glDisableClientState(Batch& batch, uint32 paramOffset);

    void do_glCullFace(Batch& batch, uint32 paramOffset);
    void do_glAlphaFunc(Batch& batch, uint32 paramOffset);

    void do_glDepthFunc(Batch& batch, uint32 paramOffset);
    void do_glDepthMask(Batch& batch, uint32 paramOffset);
    void do_glDepthRange(Batch& batch, uint32 paramOffset);

    void do_glBindBuffer(Batch& batch, uint32 paramOffset);

    void do_glBindTexture(Batch& batch, uint32 paramOffset);
    void do_glActiveTexture(Batch& batch, uint32 paramOffset);

    void do_glDrawBuffers(Batch& batch, uint32 paramOffset);

    void do_glUseProgram(Batch& batch, uint32 paramOffset);
    void do_glUniform1f(Batch& batch, uint32 paramOffset);
    void do_glUniformMatrix4fv(Batch& batch, uint32 paramOffset);

    void do_glMatrixMode(Batch& batch, uint32 paramOffset);
    void do_glPushMatrix(Batch& batch, uint32 paramOffset);
    void do_glPopMatrix(Batch& batch, uint32 paramOffset);
    void do_glMultMatrixf(Batch& batch, uint32 paramOffset);
    void do_glLoadMatrixf(Batch& batch, uint32 paramOffset);
    void do_glLoadIdentity(Batch& batch, uint32 paramOffset);
    void do_glRotatef(Batch& batch, uint32 paramOffset);
    void do_glScalef(Batch& batch, uint32 paramOffset);
    void do_glTranslatef(Batch& batch, uint32 paramOffset);

    void do_glDrawArrays(Batch& batch, uint32 paramOffset);
    void do_glDrawRangeElements(Batch& batch, uint32 paramOffset);

    void do_glColorPointer(Batch& batch, uint32 paramOffset);
    void do_glNormalPointer(Batch& batch, uint32 paramOffset);
    void do_glTexCoordPointer(Batch& batch, uint32 paramOffset);
    void do_glVertexPointer(Batch& batch, uint32 paramOffset);

    void do_glVertexAttribPointer(Batch& batch, uint32 paramOffset);
    void do_glEnableVertexAttribArray(Batch& batch, uint32 paramOffset);
    void do_glDisableVertexAttribArray(Batch& batch, uint32 paramOffset);

    void do_glColor4f(Batch& batch, uint32 paramOffset);

    void do_glMaterialf(Batch& batch, uint32 paramOffset);
    void do_glMaterialfv(Batch& batch, uint32 paramOffset);

    typedef void (GLBackend::*CommandCall)(Batch&, uint32);
    static CommandCall _commandCalls[Batch::NUM_COMMANDS];

};


};

#endif
