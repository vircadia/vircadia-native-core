//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL41Backend.h"

#include <mutex>
#include <queue>
#include <list>
#include <functional>
#include <glm/gtc/type_ptr.hpp>

Q_LOGGING_CATEGORY(gpugl41logging, "hifi.gpu.gl41")

using namespace gpu;
using namespace gpu::gl41;

void GL41Backend::do_draw(const Batch& batch, size_t paramOffset) {
    Primitive primitiveType = (Primitive)batch._params[paramOffset + 2]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[primitiveType];
    uint32 numVertices = batch._params[paramOffset + 1]._uint;
    uint32 startVertex = batch._params[paramOffset + 0]._uint;

    if (isStereo()) {
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
        glDrawArraysInstanced(mode, startVertex, numVertices, 2);
#else
        setupStereoSide(0);
        glDrawArrays(mode, startVertex, numVertices);
        setupStereoSide(1);
        glDrawArrays(mode, startVertex, numVertices);
#endif
        _stats._DSNumTriangles += 2 * numVertices / 3;
        _stats._DSNumDrawcalls += 2;

    } else {
        glDrawArrays(mode, startVertex, numVertices);
        _stats._DSNumTriangles += numVertices / 3;
        _stats._DSNumDrawcalls++;
    }
    _stats._DSNumAPIDrawcalls++;

    (void) CHECK_GL_ERROR();
}

void GL41Backend::do_drawIndexed(const Batch& batch, size_t paramOffset) {
    Primitive primitiveType = (Primitive)batch._params[paramOffset + 2]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[primitiveType];
    uint32 numIndices = batch._params[paramOffset + 1]._uint;
    uint32 startIndex = batch._params[paramOffset + 0]._uint;

    GLenum glType = gl::ELEMENT_TYPE_TO_GL[_input._indexBufferType];
    
    auto typeByteSize = TYPE_SIZE[_input._indexBufferType];
    GLvoid* indexBufferByteOffset = reinterpret_cast<GLvoid*>(startIndex * typeByteSize + _input._indexBufferOffset);

    if (isStereo()) {
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
        glDrawElementsInstanced(mode, numIndices, glType, indexBufferByteOffset, 2);
#else
        setupStereoSide(0);
        glDrawElements(mode, numIndices, glType, indexBufferByteOffset);
        setupStereoSide(1);
        glDrawElements(mode, numIndices, glType, indexBufferByteOffset);
#endif
        _stats._DSNumTriangles += 2 * numIndices / 3;
        _stats._DSNumDrawcalls += 2;
    } else {
        glDrawElements(mode, numIndices, glType, indexBufferByteOffset);
        _stats._DSNumTriangles += numIndices / 3;
        _stats._DSNumDrawcalls++;
    }
    _stats._DSNumAPIDrawcalls++;

    (void) CHECK_GL_ERROR();
}

void GL41Backend::do_drawInstanced(const Batch& batch, size_t paramOffset) {
    GLint numInstances = batch._params[paramOffset + 4]._uint;
    Primitive primitiveType = (Primitive)batch._params[paramOffset + 3]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[primitiveType];
    uint32 numVertices = batch._params[paramOffset + 2]._uint;
    uint32 startVertex = batch._params[paramOffset + 1]._uint;


    if (isStereo()) {
        GLint trueNumInstances = 2 * numInstances;

#ifdef GPU_STEREO_DRAWCALL_INSTANCED
        glDrawArraysInstanced(mode, startVertex, numVertices, trueNumInstances);
#else
        setupStereoSide(0);
        glDrawArraysInstanced(mode, startVertex, numVertices, numInstances);
        setupStereoSide(1);
        glDrawArraysInstanced(mode, startVertex, numVertices, numInstances);
#endif
        _stats._DSNumTriangles += (trueNumInstances * numVertices) / 3;
        _stats._DSNumDrawcalls += trueNumInstances;
    } else {
        glDrawArraysInstancedARB(mode, startVertex, numVertices, numInstances);
        _stats._DSNumTriangles += (numInstances * numVertices) / 3;
        _stats._DSNumDrawcalls += numInstances;
    }
    _stats._DSNumAPIDrawcalls++;

    (void) CHECK_GL_ERROR();
}

void glbackend_glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount, GLint basevertex, GLuint baseinstance) {
#if (GPU_INPUT_PROFILE == GPU_CORE_43)
    glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, primcount, basevertex, baseinstance);
#else
    glDrawElementsInstanced(mode, count, type, indices, primcount);
#endif
}

void GL41Backend::do_drawIndexedInstanced(const Batch& batch, size_t paramOffset) {
    GLint numInstances = batch._params[paramOffset + 4]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[(Primitive)batch._params[paramOffset + 3]._uint];
    uint32 numIndices = batch._params[paramOffset + 2]._uint;
    uint32 startIndex = batch._params[paramOffset + 1]._uint;
    // FIXME glDrawElementsInstancedBaseVertexBaseInstance is only available in GL 4.3 
    // and higher, so currently we ignore this field
    uint32 startInstance = batch._params[paramOffset + 0]._uint;
    GLenum glType = gl::ELEMENT_TYPE_TO_GL[_input._indexBufferType];

    auto typeByteSize = TYPE_SIZE[_input._indexBufferType];
    GLvoid* indexBufferByteOffset = reinterpret_cast<GLvoid*>(startIndex * typeByteSize + _input._indexBufferOffset);
 
    if (isStereo()) {
        GLint trueNumInstances = 2 * numInstances;

#ifdef GPU_STEREO_DRAWCALL_INSTANCED
        glbackend_glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, trueNumInstances, 0, startInstance);
#else
        setupStereoSide(0);
        glbackend_glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, numInstances, 0, startInstance);
        setupStereoSide(1);
        glbackend_glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, numInstances, 0, startInstance);
#endif

        _stats._DSNumTriangles += (trueNumInstances * numIndices) / 3;
        _stats._DSNumDrawcalls += trueNumInstances;
    } else {
        glbackend_glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, numInstances, 0, startInstance);
        _stats._DSNumTriangles += (numInstances * numIndices) / 3;
        _stats._DSNumDrawcalls += numInstances;
    }

    _stats._DSNumAPIDrawcalls++;

    (void)CHECK_GL_ERROR();
}


void GL41Backend::do_multiDrawIndirect(const Batch& batch, size_t paramOffset) {
#if (GPU_INPUT_PROFILE == GPU_CORE_43)
    uint commandCount = batch._params[paramOffset + 0]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[(Primitive)batch._params[paramOffset + 1]._uint];

    glMultiDrawArraysIndirect(mode, reinterpret_cast<GLvoid*>(_input._indirectBufferOffset), commandCount, (GLsizei)_input._indirectBufferStride);
    _stats._DSNumDrawcalls += commandCount;
    _stats._DSNumAPIDrawcalls++;

#else
    // FIXME implement the slow path
#endif
    (void)CHECK_GL_ERROR();

}

void GL41Backend::do_multiDrawIndexedIndirect(const Batch& batch, size_t paramOffset) {
#if (GPU_INPUT_PROFILE == GPU_CORE_43)
    uint commandCount = batch._params[paramOffset + 0]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[(Primitive)batch._params[paramOffset + 1]._uint];
    GLenum indexType = gl::ELEMENT_TYPE_TO_GL[_input._indexBufferType];
  
    glMultiDrawElementsIndirect(mode, indexType, reinterpret_cast<GLvoid*>(_input._indirectBufferOffset), commandCount, (GLsizei)_input._indirectBufferStride);
    _stats._DSNumDrawcalls += commandCount;
    _stats._DSNumAPIDrawcalls++;
#else
    // FIXME implement the slow path
#endif
    (void)CHECK_GL_ERROR();
}
