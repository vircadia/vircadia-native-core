//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_GLShared_h
#define hifi_gpu_GLShared_h

#include <gl/Config.h>
#include <gpu/Forward.h>
#include <gpu/Format.h>
#include <gpu/Context.h>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(gpugllogging)
Q_DECLARE_LOGGING_CATEGORY(trace_render_gpu_gl)
Q_DECLARE_LOGGING_CATEGORY(trace_render_gpu_gl_detail)

namespace gpu { namespace gl { 

// Create a fence and inject a GPU wait on the fence
void serverWait();

// Create a fence and synchronously wait on the fence
void clientWait();

gpu::Size getDedicatedMemory();
gpu::Size getFreeDedicatedMemory();
ComparisonFunction comparisonFuncFromGL(GLenum func);
State::StencilOp stencilOpFromGL(GLenum stencilOp);
State::BlendOp blendOpFromGL(GLenum blendOp);
State::BlendArg blendArgFromGL(GLenum blendArg);
void getCurrentGLState(State::Data& state);

struct ShaderObject {
    GLuint glshader { 0 };
    GLuint glprogram { 0 };
    GLint transformCameraSlot { -1 };
    GLint transformObjectSlot { -1 };
};

int makeUniformSlots(GLuint glprogram, const Shader::BindingSet& slotBindings,
    Shader::SlotSet& uniforms, Shader::SlotSet& textures, Shader::SlotSet& samplers);
int makeUniformBlockSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& buffers);
int makeInputSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& inputs);
int makeOutputSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& outputs);
void makeProgramBindings(ShaderObject& shaderObject);

enum GLSyncState {
    // The object is currently undergoing no processing, although it's content
    // may be out of date, or it's storage may be invalid relative to the 
    // owning GPU object
    Idle,
    // The object has been queued for transfer to the GPU
    Pending,
    // The object has been transferred to the GPU, but is awaiting
    // any post transfer operations that may need to occur on the 
    // primary rendering thread
    Transferred,
};

static const GLenum BLEND_OPS_TO_GL[State::NUM_BLEND_OPS] = {
    GL_FUNC_ADD,
    GL_FUNC_SUBTRACT,
    GL_FUNC_REVERSE_SUBTRACT,
    GL_MIN,
    GL_MAX 
};

static const GLenum BLEND_ARGS_TO_GL[State::NUM_BLEND_ARGS] = {
    GL_ZERO,
    GL_ONE,
    GL_SRC_COLOR,
    GL_ONE_MINUS_SRC_COLOR,
    GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_DST_COLOR,
    GL_ONE_MINUS_DST_COLOR,
    GL_SRC_ALPHA_SATURATE,
    GL_CONSTANT_COLOR,
    GL_ONE_MINUS_CONSTANT_COLOR,
    GL_CONSTANT_ALPHA,
    GL_ONE_MINUS_CONSTANT_ALPHA,
};

static const GLenum COMPARISON_TO_GL[gpu::NUM_COMPARISON_FUNCS] = {
    GL_NEVER,
    GL_LESS,
    GL_EQUAL,
    GL_LEQUAL,
    GL_GREATER,
    GL_NOTEQUAL,
    GL_GEQUAL,
    GL_ALWAYS 
};

static const GLenum PRIMITIVE_TO_GL[gpu::NUM_PRIMITIVES] = {
    GL_POINTS,
    GL_LINES,
    GL_LINE_STRIP,
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
};

static const GLenum ELEMENT_TYPE_TO_GL[gpu::NUM_TYPES] = {
    GL_FLOAT,
    GL_INT,
    GL_UNSIGNED_INT,
    GL_HALF_FLOAT,
    GL_SHORT,
    GL_UNSIGNED_SHORT,
    GL_BYTE,
    GL_UNSIGNED_BYTE,
    // Normalized values
    GL_INT,
    GL_UNSIGNED_INT,
    GL_SHORT,
    GL_UNSIGNED_SHORT,
    GL_BYTE,
    GL_UNSIGNED_BYTE
};

bool checkGLError(const char* name = nullptr);
bool checkGLErrorDebug(const char* name = nullptr);

class GLBackend;

template <typename GPUType>
struct GLObject : public GPUObject {
public:
    GLObject(const std::weak_ptr<GLBackend>& backend, const GPUType& gpuObject, GLuint id) : _gpuObject(gpuObject), _id(id), _backend(backend) {}

    virtual ~GLObject() { }

    const GPUType& _gpuObject;
    const GLuint _id;
protected:
    const std::weak_ptr<GLBackend> _backend;
};

class GlBuffer;
class GLFramebuffer;
class GLPipeline;
class GLQuery;
class GLState;
class GLShader;
class GLTexture;

} } // namespace gpu::gl 

#define CHECK_GL_ERROR() gpu::gl::checkGLErrorDebug(__FUNCTION__)

#endif



