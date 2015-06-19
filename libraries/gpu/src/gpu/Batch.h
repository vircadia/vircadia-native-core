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
#include "GPUConfig.h"

#include "Transform.h"

#include <vector>

#include "Stream.h"
#include "Texture.h"

#include "Pipeline.h"

#include "Framebuffer.h"

#if defined(NSIGHT_FOUND)
    #include "nvToolsExt.h"
    class ProfileRange {
    public:
        ProfileRange(const char *name) {
            nvtxRangePush(name);
        }
        ~ProfileRange() {
            nvtxRangePop();
        }
    };

    #define PROFILE_RANGE(name) ProfileRange profileRangeThis(name);
#else
#define PROFILE_RANGE(name)
#endif

namespace gpu {

enum Primitive {
    POINTS = 0,
    LINES,
    LINE_STRIP,
    TRIANGLES,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
    QUADS,
    QUAD_STRIP,

    NUM_PRIMITIVES,
};

enum ReservedSlot {
/*    TRANSFORM_OBJECT_SLOT = 6,
    TRANSFORM_CAMERA_SLOT = 7,
    */
    TRANSFORM_OBJECT_SLOT = 1,
    TRANSFORM_CAMERA_SLOT = 2,
};

class Batch {
public:
    typedef Stream::Slot Slot;

    Batch();
    explicit Batch(const Batch& batch);
    ~Batch();

    void clear();

    // Drawcalls
    void draw(Primitive primitiveType, uint32 numVertices, uint32 startVertex = 0);
    void drawIndexed(Primitive primitiveType, uint32 nbIndices, uint32 startIndex = 0);
    void drawInstanced(uint32 nbInstances, Primitive primitiveType, uint32 nbVertices, uint32 startVertex = 0, uint32 startInstance = 0);
    void drawIndexedInstanced(uint32 nbInstances, Primitive primitiveType, uint32 nbIndices, uint32 startIndex = 0, uint32 startInstance = 0);

    // Clear framebuffer layers
    void clearFramebuffer(Framebuffer::Masks targets, const Vec4& color, float depth, int stencil);
    
    // Input Stage
    // InputFormat
    // InputBuffers
    // IndexBuffer
    void setInputFormat(const Stream::FormatPointer& format);

    void setInputBuffer(Slot channel, const BufferPointer& buffer, Offset offset, Offset stride);
    void setInputBuffer(Slot channel, const BufferView& buffer); // not a command, just a shortcut from a BufferView
    void setInputStream(Slot startChannel, const BufferStream& stream); // not a command, just unroll into a loop of setInputBuffer

    void setIndexBuffer(Type type, const BufferPointer& buffer, Offset offset);

    // Transform Stage
    // Vertex position is transformed by ModelTransform from object space to world space
    // Then by the inverse of the ViewTransform from world space to eye space
    // finaly projected into the clip space by the projection transform
    // WARNING: ViewTransform transform from eye space to world space, its inverse is composed
    // with the ModelTransformu to create the equivalent of the glModelViewMatrix
    void setModelTransform(const Transform& model);
    void setViewTransform(const Transform& view);
    void setProjectionTransform(const Mat4& proj);

    // Pipeline Stage
    void setPipeline(const PipelinePointer& pipeline);

    void setStateBlendFactor(const Vec4& factor);

    void setUniformBuffer(uint32 slot, const BufferPointer& buffer, Offset offset, Offset size);
    void setUniformBuffer(uint32 slot, const BufferView& view); // not a command, just a shortcut from a BufferView

    void setUniformTexture(uint32 slot, const TexturePointer& view);
    void setUniformTexture(uint32 slot, const TextureView& view); // not a command, just a shortcut from a TextureView

    // Framebuffer Stage
    void setFramebuffer(const FramebufferPointer& framebuffer);

    // TODO: As long as we have gl calls explicitely issued from interface
    // code, we need to be able to record and batch these calls. THe long 
    // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
    // For now, instead of calling the raw glCall, use the equivalent call on the batch so the call is beeing recorded
    // THe implementation of these functions is in GLBackend.cpp

    void _glEnable(GLenum cap);
    void _glDisable(GLenum cap);

    void _glEnableClientState(GLenum array);
    void _glDisableClientState(GLenum array);

    void _glCullFace(GLenum mode);
    void _glAlphaFunc(GLenum func, GLclampf ref);

    void _glDepthFunc(GLenum func);
    void _glDepthMask(GLboolean flag);
    void _glDepthRange(GLfloat  zNear, GLfloat  zFar);

    void _glBindBuffer(GLenum target, GLuint buffer);

    void _glBindTexture(GLenum target, GLuint texture);
    void _glActiveTexture(GLenum texture);
    void _glTexParameteri(GLenum target, GLenum pname, GLint param);
    
    void _glDrawBuffers(GLsizei n, const GLenum* bufs);

    void _glUseProgram(GLuint program);
    void _glUniform1i(GLint location, GLint v0);
    void _glUniform1f(GLint location, GLfloat v0);
    void _glUniform2f(GLint location, GLfloat v0, GLfloat v1);
    void _glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    void _glUniform3fv(GLint location, GLsizei count, const GLfloat* value);
    void _glUniform4fv(GLint location, GLsizei count, const GLfloat* value);
    void _glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

    void _glEnableVertexAttribArray(GLint location);
    void _glDisableVertexAttribArray(GLint location);

    void _glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void _glLineWidth(GLfloat width);

    enum Command {
        COMMAND_draw = 0,
        COMMAND_drawIndexed,
        COMMAND_drawInstanced,
        COMMAND_drawIndexedInstanced,

        COMMAND_clearFramebuffer,

        COMMAND_setInputFormat,
        COMMAND_setInputBuffer,
        COMMAND_setIndexBuffer,

        COMMAND_setModelTransform,
        COMMAND_setViewTransform,
        COMMAND_setProjectionTransform,

        COMMAND_setPipeline,
        COMMAND_setStateBlendFactor,

        COMMAND_setUniformBuffer,
        COMMAND_setUniformTexture,

        COMMAND_setFramebuffer,

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
        COMMAND_glTexParameteri,

        COMMAND_glDrawBuffers,

        COMMAND_glUseProgram,
        COMMAND_glUniform1i,
        COMMAND_glUniform1f,
        COMMAND_glUniform2f,
        COMMAND_glUniform3f,
        COMMAND_glUniform3fv,
        COMMAND_glUniform4fv,
        COMMAND_glUniformMatrix4fv,

        COMMAND_glEnableVertexAttribArray,
        COMMAND_glDisableVertexAttribArray,

        COMMAND_glColor4f,
        COMMAND_glLineWidth,

        NUM_COMMANDS,
    };
    typedef std::vector<Command> Commands;
    typedef std::vector<uint32> CommandOffsets;

    const Commands& getCommands() const { return _commands; }
    const CommandOffsets& getCommandOffsets() const { return _commandOffsets; }

    class Param {
    public:
        union {
            int32 _int;
            uint32 _uint;
            float   _float;
            char _chars[4];
        };
        Param(int32 val) : _int(val) {}
        Param(uint32 val) : _uint(val) {}
        Param(float val) : _float(val) {}
    };
    typedef std::vector<Param> Params;

    const Params& getParams() const { return _params; }

    // The template cache mechanism for the gpu::Object passed to the gpu::Batch
    // this allow us to have one cache container for each different types and eventually
    // be smarter how we manage them
    template <typename T>
    class Cache {
    public:
        typedef T Data;
        Data _data;
        Cache<T>(const Data& data) : _data(data) {}

        class Vector {
        public:
            std::vector< Cache<T> > _items;

            uint32 cache(const Data& data) {
                uint32 offset = _items.size();
                _items.push_back(Cache<T>(data));
                return offset;
            }

            Data get(uint32 offset) {
                if (offset >= _items.size()) {
                    return Data();
                }
                return (_items.data() + offset)->_data;
            }

            void clear() {
                _items.clear();
            }
        };
    };

    typedef Cache<BufferPointer>::Vector BufferCaches;
    typedef Cache<TexturePointer>::Vector TextureCaches;
    typedef Cache<Stream::FormatPointer>::Vector StreamFormatCaches;
    typedef Cache<Transform>::Vector TransformCaches;
    typedef Cache<PipelinePointer>::Vector PipelineCaches;
    typedef Cache<FramebufferPointer>::Vector FramebufferCaches;

    // Cache Data in a byte array if too big to fit in Param
    // FOr example Mat4s are going there
    typedef unsigned char Byte;
    typedef std::vector<Byte> Bytes;
    uint32 cacheData(uint32 size, const void* data);
    Byte* editData(uint32 offset) {
        if (offset >= _data.size()) {
            return 0;
        }
        return (_data.data() + offset);
    }

    Commands _commands;
    CommandOffsets _commandOffsets;
    Params _params;
    Bytes _data;

    BufferCaches _buffers;
    TextureCaches _textures;
    StreamFormatCaches _streamFormats;
    TransformCaches _transforms;
    PipelineCaches _pipelines;
    FramebufferCaches _framebuffers;

protected:
};

};

#endif
