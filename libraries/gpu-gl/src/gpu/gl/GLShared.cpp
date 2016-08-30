//
//  Created by Bradley Austin Davis on 2016/05/14
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLShared.h"

#include <mutex>

#include <GPUIdent.h>
#include <NumericalConstants.h>
#include <fstream>

Q_LOGGING_CATEGORY(gpugllogging, "hifi.gpu.gl")

namespace gpu { namespace gl {

bool checkGLError(const char* name) {
    GLenum error = glGetError();
    if (!error) {
        return false;
    } else {
        switch (error) {
        case GL_INVALID_ENUM:
            qCDebug(gpugllogging) << "GLBackend::" << name << ": An unacceptable value is specified for an enumerated argument.The offending command is ignored and has no other side effect than to set the error flag.";
            break;
        case GL_INVALID_VALUE:
            qCDebug(gpugllogging) << "GLBackend" << name << ": A numeric argument is out of range.The offending command is ignored and has no other side effect than to set the error flag";
            break;
        case GL_INVALID_OPERATION:
            qCDebug(gpugllogging) << "GLBackend" << name << ": The specified operation is not allowed in the current state.The offending command is ignored and has no other side effect than to set the error flag..";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            qCDebug(gpugllogging) << "GLBackend" << name << ": The framebuffer object is not complete.The offending command is ignored and has no other side effect than to set the error flag.";
            break;
        case GL_OUT_OF_MEMORY:
            qCDebug(gpugllogging) << "GLBackend" << name << ": There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
            break;
        case GL_STACK_UNDERFLOW:
            qCDebug(gpugllogging) << "GLBackend" << name << ": An attempt has been made to perform an operation that would cause an internal stack to underflow.";
            break;
        case GL_STACK_OVERFLOW:
            qCDebug(gpugllogging) << "GLBackend" << name << ": An attempt has been made to perform an operation that would cause an internal stack to overflow.";
            break;
        }
        return true;
    }
}

bool checkGLErrorDebug(const char* name) {
#ifdef DEBUG
    return checkGLError(name);
#else
    Q_UNUSED(name);
    return false;
#endif
}

gpu::Size getDedicatedMemory() {
    static Size dedicatedMemory { 0 };
    static std::once_flag once;
    std::call_once(once, [&] {
#ifdef Q_OS_WIN
        if (!dedicatedMemory && wglGetGPUIDsAMD && wglGetGPUInfoAMD) {
            UINT maxCount = wglGetGPUIDsAMD(0, 0);
            std::vector<UINT> ids;
            ids.resize(maxCount);
            wglGetGPUIDsAMD(maxCount, &ids[0]);
            GLuint memTotal;
            wglGetGPUInfoAMD(ids[0], WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(GLuint), &memTotal);
            dedicatedMemory = MB_TO_BYTES(memTotal);
        }
#endif

        if (!dedicatedMemory) {
            GLint atiGpuMemory[4];
            // not really total memory, but close enough if called early enough in the application lifecycle
            glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, atiGpuMemory);
            if (GL_NO_ERROR == glGetError()) {
                dedicatedMemory = KB_TO_BYTES(atiGpuMemory[0]);
            }
        }

        if (!dedicatedMemory) {
            GLint nvGpuMemory { 0 };
            glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &nvGpuMemory);
            if (GL_NO_ERROR == glGetError()) {
                dedicatedMemory = KB_TO_BYTES(nvGpuMemory);
            }
        }

        if (!dedicatedMemory) {
            auto gpuIdent = GPUIdent::getInstance();
            if (gpuIdent && gpuIdent->isValid()) {
                dedicatedMemory = MB_TO_BYTES(gpuIdent->getMemory());
            }
        }
    });

    return dedicatedMemory;
}




ComparisonFunction comparisonFuncFromGL(GLenum func) {
    if (func == GL_NEVER) {
        return NEVER;
    } else if (func == GL_LESS) {
        return LESS;
    } else if (func == GL_EQUAL) {
        return EQUAL;
    } else if (func == GL_LEQUAL) {
        return LESS_EQUAL;
    } else if (func == GL_GREATER) {
        return GREATER;
    } else if (func == GL_NOTEQUAL) {
        return NOT_EQUAL;
    } else if (func == GL_GEQUAL) {
        return GREATER_EQUAL;
    } else if (func == GL_ALWAYS) {
        return ALWAYS;
    }

    return ALWAYS;
}

State::StencilOp stencilOpFromGL(GLenum stencilOp) {
    if (stencilOp == GL_KEEP) {
        return State::STENCIL_OP_KEEP;
    } else if (stencilOp == GL_ZERO) {
        return State::STENCIL_OP_ZERO;
    } else if (stencilOp == GL_REPLACE) {
        return State::STENCIL_OP_REPLACE;
    } else if (stencilOp == GL_INCR_WRAP) {
        return State::STENCIL_OP_INCR_SAT;
    } else if (stencilOp == GL_DECR_WRAP) {
        return State::STENCIL_OP_DECR_SAT;
    } else if (stencilOp == GL_INVERT) {
        return State::STENCIL_OP_INVERT;
    } else if (stencilOp == GL_INCR) {
        return State::STENCIL_OP_INCR;
    } else if (stencilOp == GL_DECR) {
        return State::STENCIL_OP_DECR;
    }

    return State::STENCIL_OP_KEEP;
}

State::BlendOp blendOpFromGL(GLenum blendOp) {
    if (blendOp == GL_FUNC_ADD) {
        return State::BLEND_OP_ADD;
    } else if (blendOp == GL_FUNC_SUBTRACT) {
        return State::BLEND_OP_SUBTRACT;
    } else if (blendOp == GL_FUNC_REVERSE_SUBTRACT) {
        return State::BLEND_OP_REV_SUBTRACT;
    } else if (blendOp == GL_MIN) {
        return State::BLEND_OP_MIN;
    } else if (blendOp == GL_MAX) {
        return State::BLEND_OP_MAX;
    }

    return State::BLEND_OP_ADD;
}

State::BlendArg blendArgFromGL(GLenum blendArg) {
    if (blendArg == GL_ZERO) {
        return State::ZERO;
    } else if (blendArg == GL_ONE) {
        return State::ONE;
    } else if (blendArg == GL_SRC_COLOR) {
        return State::SRC_COLOR;
    } else if (blendArg == GL_ONE_MINUS_SRC_COLOR) {
        return State::INV_SRC_COLOR;
    } else if (blendArg == GL_DST_COLOR) {
        return State::DEST_COLOR;
    } else if (blendArg == GL_ONE_MINUS_DST_COLOR) {
        return State::INV_DEST_COLOR;
    } else if (blendArg == GL_SRC_ALPHA) {
        return State::SRC_ALPHA;
    } else if (blendArg == GL_ONE_MINUS_SRC_ALPHA) {
        return State::INV_SRC_ALPHA;
    } else if (blendArg == GL_DST_ALPHA) {
        return State::DEST_ALPHA;
    } else if (blendArg == GL_ONE_MINUS_DST_ALPHA) {
        return State::INV_DEST_ALPHA;
    } else if (blendArg == GL_CONSTANT_COLOR) {
        return State::FACTOR_COLOR;
    } else if (blendArg == GL_ONE_MINUS_CONSTANT_COLOR) {
        return State::INV_FACTOR_COLOR;
    } else if (blendArg == GL_CONSTANT_ALPHA) {
        return State::FACTOR_ALPHA;
    } else if (blendArg == GL_ONE_MINUS_CONSTANT_ALPHA) {
        return State::INV_FACTOR_ALPHA;
    }

    return State::ONE;
}

void getCurrentGLState(State::Data& state) {
    {
        GLint modes[2];
        glGetIntegerv(GL_POLYGON_MODE, modes);
        if (modes[0] == GL_FILL) {
            state.fillMode = State::FILL_FACE;
        } else {
            if (modes[0] == GL_LINE) {
                state.fillMode = State::FILL_LINE;
            } else {
                state.fillMode = State::FILL_POINT;
            }
        }
    }
    {
        if (glIsEnabled(GL_CULL_FACE)) {
            GLint mode;
            glGetIntegerv(GL_CULL_FACE_MODE, &mode);
            state.cullMode = (mode == GL_FRONT ? State::CULL_FRONT : State::CULL_BACK);
        } else {
            state.cullMode = State::CULL_NONE;
        }
    }
    {
        GLint winding;
        glGetIntegerv(GL_FRONT_FACE, &winding);
        state.frontFaceClockwise = (winding == GL_CW);
        state.depthClampEnable = glIsEnabled(GL_DEPTH_CLAMP);
        state.scissorEnable = glIsEnabled(GL_SCISSOR_TEST);
        state.multisampleEnable = glIsEnabled(GL_MULTISAMPLE);
        state.antialisedLineEnable = glIsEnabled(GL_LINE_SMOOTH);
    }
    {
        if (glIsEnabled(GL_POLYGON_OFFSET_FILL)) {
            glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &state.depthBiasSlopeScale);
            glGetFloatv(GL_POLYGON_OFFSET_UNITS, &state.depthBias);
        }
    }
    {
        GLboolean isEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean writeMask;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &writeMask);
        GLint func;
        glGetIntegerv(GL_DEPTH_FUNC, &func);

        state.depthTest = State::DepthTest(isEnabled, writeMask, comparisonFuncFromGL(func));
    }
    {
        GLboolean isEnabled = glIsEnabled(GL_STENCIL_TEST);

        GLint frontWriteMask;
        GLint frontReadMask;
        GLint frontRef;
        GLint frontFail;
        GLint frontDepthFail;
        GLint frontPass;
        GLint frontFunc;
        glGetIntegerv(GL_STENCIL_WRITEMASK, &frontWriteMask);
        glGetIntegerv(GL_STENCIL_VALUE_MASK, &frontReadMask);
        glGetIntegerv(GL_STENCIL_REF, &frontRef);
        glGetIntegerv(GL_STENCIL_FAIL, &frontFail);
        glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &frontDepthFail);
        glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &frontPass);
        glGetIntegerv(GL_STENCIL_FUNC, &frontFunc);

        GLint backWriteMask;
        GLint backReadMask;
        GLint backRef;
        GLint backFail;
        GLint backDepthFail;
        GLint backPass;
        GLint backFunc;
        glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &backWriteMask);
        glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &backReadMask);
        glGetIntegerv(GL_STENCIL_BACK_REF, &backRef);
        glGetIntegerv(GL_STENCIL_BACK_FAIL, &backFail);
        glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &backDepthFail);
        glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &backPass);
        glGetIntegerv(GL_STENCIL_BACK_FUNC, &backFunc);

        state.stencilActivation = State::StencilActivation(isEnabled, frontWriteMask, backWriteMask);
        state.stencilTestFront = State::StencilTest(frontRef, frontReadMask, comparisonFuncFromGL(frontFunc), stencilOpFromGL(frontFail), stencilOpFromGL(frontDepthFail), stencilOpFromGL(frontPass));
        state.stencilTestBack = State::StencilTest(backRef, backReadMask, comparisonFuncFromGL(backFunc), stencilOpFromGL(backFail), stencilOpFromGL(backDepthFail), stencilOpFromGL(backPass));
    }
    {
        GLint mask = 0xFFFFFFFF;
        if (glIsEnabled(GL_SAMPLE_MASK)) {
            glGetIntegerv(GL_SAMPLE_MASK, &mask);
            state.sampleMask = mask;
        }
        state.sampleMask = mask;
    }
    {
        state.alphaToCoverageEnable = glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    {
        GLboolean isEnabled = glIsEnabled(GL_BLEND);
        GLint srcRGB;
        GLint srcA;
        GLint dstRGB;
        GLint dstA;
        glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcA);
        glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &dstA);

        GLint opRGB;
        GLint opA;
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &opRGB);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &opA);

        state.blendFunction = State::BlendFunction(isEnabled,
            blendArgFromGL(srcRGB), blendOpFromGL(opRGB), blendArgFromGL(dstRGB),
            blendArgFromGL(srcA), blendOpFromGL(opA), blendArgFromGL(dstA));
    }
    {
        GLboolean mask[4];
        glGetBooleanv(GL_COLOR_WRITEMASK, mask);
        state.colorWriteMask = (mask[0] ? State::WRITE_RED : 0)
            | (mask[1] ? State::WRITE_GREEN : 0)
            | (mask[2] ? State::WRITE_BLUE : 0)
            | (mask[3] ? State::WRITE_ALPHA : 0);
    }

    (void)CHECK_GL_ERROR();
}


class ElementResource {
public:
    gpu::Element _element;
    uint16 _resource;

    ElementResource(Element&& elem, uint16 resource) : _element(elem), _resource(resource) {}
};

ElementResource getFormatFromGLUniform(GLenum gltype) {
    switch (gltype) {
    case GL_FLOAT: return ElementResource(Element(SCALAR, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_FLOAT_VEC2: return ElementResource(Element(VEC2, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_FLOAT_VEC3: return ElementResource(Element(VEC3, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_FLOAT_VEC4: return ElementResource(Element(VEC4, gpu::FLOAT, UNIFORM), Resource::BUFFER);
        /*
        case GL_DOUBLE: return ElementResource(Element(SCALAR, gpu::FLOAT, UNIFORM), Resource::BUFFER);
        case GL_DOUBLE_VEC2: return ElementResource(Element(VEC2, gpu::FLOAT, UNIFORM), Resource::BUFFER);
        case GL_DOUBLE_VEC3: return ElementResource(Element(VEC3, gpu::FLOAT, UNIFORM), Resource::BUFFER);
        case GL_DOUBLE_VEC4: return ElementResource(Element(VEC4, gpu::FLOAT, UNIFORM), Resource::BUFFER);
        */
    case GL_INT: return ElementResource(Element(SCALAR, gpu::INT32, UNIFORM), Resource::BUFFER);
    case GL_INT_VEC2: return ElementResource(Element(VEC2, gpu::INT32, UNIFORM), Resource::BUFFER);
    case GL_INT_VEC3: return ElementResource(Element(VEC3, gpu::INT32, UNIFORM), Resource::BUFFER);
    case GL_INT_VEC4: return ElementResource(Element(VEC4, gpu::INT32, UNIFORM), Resource::BUFFER);

    case GL_UNSIGNED_INT: return ElementResource(Element(SCALAR, gpu::UINT32, UNIFORM), Resource::BUFFER);
#if defined(Q_OS_WIN)
    case GL_UNSIGNED_INT_VEC2: return ElementResource(Element(VEC2, gpu::UINT32, UNIFORM), Resource::BUFFER);
    case GL_UNSIGNED_INT_VEC3: return ElementResource(Element(VEC3, gpu::UINT32, UNIFORM), Resource::BUFFER);
    case GL_UNSIGNED_INT_VEC4: return ElementResource(Element(VEC4, gpu::UINT32, UNIFORM), Resource::BUFFER);
#endif

    case GL_BOOL: return ElementResource(Element(SCALAR, gpu::BOOL, UNIFORM), Resource::BUFFER);
    case GL_BOOL_VEC2: return ElementResource(Element(VEC2, gpu::BOOL, UNIFORM), Resource::BUFFER);
    case GL_BOOL_VEC3: return ElementResource(Element(VEC3, gpu::BOOL, UNIFORM), Resource::BUFFER);
    case GL_BOOL_VEC4: return ElementResource(Element(VEC4, gpu::BOOL, UNIFORM), Resource::BUFFER);


    case GL_FLOAT_MAT2: return ElementResource(Element(gpu::MAT2, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_FLOAT_MAT3: return ElementResource(Element(MAT3, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_FLOAT_MAT4: return ElementResource(Element(MAT4, gpu::FLOAT, UNIFORM), Resource::BUFFER);

        /*    {GL_FLOAT_MAT2x3    mat2x3},
        {GL_FLOAT_MAT2x4    mat2x4},
        {GL_FLOAT_MAT3x2    mat3x2},
        {GL_FLOAT_MAT3x4    mat3x4},
        {GL_FLOAT_MAT4x2    mat4x2},
        {GL_FLOAT_MAT4x3    mat4x3},
        {GL_DOUBLE_MAT2    dmat2},
        {GL_DOUBLE_MAT3    dmat3},
        {GL_DOUBLE_MAT4    dmat4},
        {GL_DOUBLE_MAT2x3    dmat2x3},
        {GL_DOUBLE_MAT2x4    dmat2x4},
        {GL_DOUBLE_MAT3x2    dmat3x2},
        {GL_DOUBLE_MAT3x4    dmat3x4},
        {GL_DOUBLE_MAT4x2    dmat4x2},
        {GL_DOUBLE_MAT4x3    dmat4x3},
        */

    case GL_SAMPLER_1D: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_1D);
    case GL_SAMPLER_2D: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_2D);

    case GL_SAMPLER_3D: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_3D);
    case GL_SAMPLER_CUBE: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_CUBE);

#if defined(Q_OS_WIN)
    case GL_SAMPLER_2D_MULTISAMPLE: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D);
    case GL_SAMPLER_1D_ARRAY: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_1D_ARRAY);
    case GL_SAMPLER_2D_ARRAY: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_2D_ARRAY);
    case GL_SAMPLER_2D_MULTISAMPLE_ARRAY: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D_ARRAY);
#endif

    case GL_SAMPLER_2D_SHADOW: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER_SHADOW), Resource::TEXTURE_2D);
#if defined(Q_OS_WIN)
    case GL_SAMPLER_CUBE_SHADOW: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER_SHADOW), Resource::TEXTURE_CUBE);

    case GL_SAMPLER_2D_ARRAY_SHADOW: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER_SHADOW), Resource::TEXTURE_2D_ARRAY);
#endif

        //    {GL_SAMPLER_1D_SHADOW    sampler1DShadow},
        //   {GL_SAMPLER_1D_ARRAY_SHADOW    sampler1DArrayShadow},

        //    {GL_SAMPLER_BUFFER    samplerBuffer},
        //    {GL_SAMPLER_2D_RECT    sampler2DRect},
        //   {GL_SAMPLER_2D_RECT_SHADOW    sampler2DRectShadow},

#if defined(Q_OS_WIN)
    case GL_INT_SAMPLER_1D: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_1D);
    case GL_INT_SAMPLER_2D: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_2D);
    case GL_INT_SAMPLER_2D_MULTISAMPLE: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D);
    case GL_INT_SAMPLER_3D: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_3D);
    case GL_INT_SAMPLER_CUBE: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_CUBE);

    case GL_INT_SAMPLER_1D_ARRAY: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_1D_ARRAY);
    case GL_INT_SAMPLER_2D_ARRAY: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_2D_ARRAY);
    case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D_ARRAY);

        //   {GL_INT_SAMPLER_BUFFER    isamplerBuffer},
        //   {GL_INT_SAMPLER_2D_RECT    isampler2DRect},

    case GL_UNSIGNED_INT_SAMPLER_1D: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_1D);
    case GL_UNSIGNED_INT_SAMPLER_2D: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_2D);
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D);
    case GL_UNSIGNED_INT_SAMPLER_3D: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_3D);
    case GL_UNSIGNED_INT_SAMPLER_CUBE: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_CUBE);

    case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_1D_ARRAY);
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_2D_ARRAY);
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D_ARRAY);
#endif
        //    {GL_UNSIGNED_INT_SAMPLER_BUFFER    usamplerBuffer},
        //    {GL_UNSIGNED_INT_SAMPLER_2D_RECT    usampler2DRect},
        /*
        {GL_IMAGE_1D    image1D},
        {GL_IMAGE_2D    image2D},
        {GL_IMAGE_3D    image3D},
        {GL_IMAGE_2D_RECT    image2DRect},
        {GL_IMAGE_CUBE    imageCube},
        {GL_IMAGE_BUFFER    imageBuffer},
        {GL_IMAGE_1D_ARRAY    image1DArray},
        {GL_IMAGE_2D_ARRAY    image2DArray},
        {GL_IMAGE_2D_MULTISAMPLE    image2DMS},
        {GL_IMAGE_2D_MULTISAMPLE_ARRAY    image2DMSArray},
        {GL_INT_IMAGE_1D    iimage1D},
        {GL_INT_IMAGE_2D    iimage2D},
        {GL_INT_IMAGE_3D    iimage3D},
        {GL_INT_IMAGE_2D_RECT    iimage2DRect},
        {GL_INT_IMAGE_CUBE    iimageCube},
        {GL_INT_IMAGE_BUFFER    iimageBuffer},
        {GL_INT_IMAGE_1D_ARRAY    iimage1DArray},
        {GL_INT_IMAGE_2D_ARRAY    iimage2DArray},
        {GL_INT_IMAGE_2D_MULTISAMPLE    iimage2DMS},
        {GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY    iimage2DMSArray},
        {GL_UNSIGNED_INT_IMAGE_1D    uimage1D},
        {GL_UNSIGNED_INT_IMAGE_2D    uimage2D},
        {GL_UNSIGNED_INT_IMAGE_3D    uimage3D},
        {GL_UNSIGNED_INT_IMAGE_2D_RECT    uimage2DRect},
        {GL_UNSIGNED_INT_IMAGE_CUBE    uimageCube},+        [0]    {_name="fInnerRadius" _location=0 _element={_semantic=15 '\xf' _dimension=0 '\0' _type=0 '\0' } }    gpu::Shader::Slot

        {GL_UNSIGNED_INT_IMAGE_BUFFER    uimageBuffer},
        {GL_UNSIGNED_INT_IMAGE_1D_ARRAY    uimage1DArray},
        {GL_UNSIGNED_INT_IMAGE_2D_ARRAY    uimage2DArray},
        {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE    uimage2DMS},
        {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY    uimage2DMSArray},
        {GL_UNSIGNED_INT_ATOMIC_COUNTER    atomic_uint}
        */
    default:
        return ElementResource(Element(), Resource::BUFFER);
    }

};

int makeUniformSlots(GLuint glprogram, const Shader::BindingSet& slotBindings,
    Shader::SlotSet& uniforms, Shader::SlotSet& textures, Shader::SlotSet& samplers) {
    GLint uniformsCount = 0;

    glGetProgramiv(glprogram, GL_ACTIVE_UNIFORMS, &uniformsCount);

    for (int i = 0; i < uniformsCount; i++) {
        const GLint NAME_LENGTH = 256;
        GLchar name[NAME_LENGTH];
        GLint length = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveUniform(glprogram, i, NAME_LENGTH, &length, &size, &type, name);
        GLint location = glGetUniformLocation(glprogram, name);
        const GLint INVALID_UNIFORM_LOCATION = -1;

        // Try to make sense of the gltype
        auto elementResource = getFormatFromGLUniform(type);

        // The uniform as a standard var type
        if (location != INVALID_UNIFORM_LOCATION) {
            // Let's make sure the name doesn't contains an array element
            std::string sname(name);
            auto foundBracket = sname.find_first_of('[');
            if (foundBracket != std::string::npos) {
                //  std::string arrayname = sname.substr(0, foundBracket);

                if (sname[foundBracket + 1] == '0') {
                    sname = sname.substr(0, foundBracket);
                } else {
                    // skip this uniform since it's not the first element of an array
                    continue;
                }
            }

            if (elementResource._resource == Resource::BUFFER) {
                uniforms.insert(Shader::Slot(sname, location, elementResource._element, elementResource._resource));
            } else {
                // For texture/Sampler, the location is the actual binding value
                GLint binding = -1;
                glGetUniformiv(glprogram, location, &binding);

                auto requestedBinding = slotBindings.find(std::string(sname));
                if (requestedBinding != slotBindings.end()) {
                    if (binding != (*requestedBinding)._location) {
                        binding = (*requestedBinding)._location;
                        glProgramUniform1i(glprogram, location, binding);
                    }
                }

                textures.insert(Shader::Slot(name, binding, elementResource._element, elementResource._resource));
                samplers.insert(Shader::Slot(name, binding, elementResource._element, elementResource._resource));
            }
        }
    }

    return uniformsCount;
}

const GLint UNUSED_SLOT = -1;
bool isUnusedSlot(GLint binding) {
    return (binding == UNUSED_SLOT);
}

int makeUniformBlockSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& buffers) {
    GLint buffersCount = 0;

    glGetProgramiv(glprogram, GL_ACTIVE_UNIFORM_BLOCKS, &buffersCount);

    // fast exit
    if (buffersCount == 0) {
        return 0;
    }

    GLint maxNumUniformBufferSlots = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxNumUniformBufferSlots);
    std::vector<GLint> uniformBufferSlotMap(maxNumUniformBufferSlots, -1);

    struct UniformBlockInfo {
        using Vector = std::vector<UniformBlockInfo>;
        const GLuint index{ 0 };
        const std::string name;
        GLint binding{ -1 };
        GLint size{ 0 };

        static std::string getName(GLuint glprogram, GLuint i) {
            static const GLint NAME_LENGTH = 256;
            GLint length = 0;
            GLchar nameBuffer[NAME_LENGTH];
            glGetActiveUniformBlockiv(glprogram, i, GL_UNIFORM_BLOCK_NAME_LENGTH, &length);
            glGetActiveUniformBlockName(glprogram, i, NAME_LENGTH, &length, nameBuffer);
            return std::string(nameBuffer);
        }

        UniformBlockInfo(GLuint glprogram, GLuint i) : index(i), name(getName(glprogram, i)) {
            glGetActiveUniformBlockiv(glprogram, index, GL_UNIFORM_BLOCK_BINDING, &binding);
            glGetActiveUniformBlockiv(glprogram, index, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
        }
    };

    UniformBlockInfo::Vector uniformBlocks;
    uniformBlocks.reserve(buffersCount);
    for (int i = 0; i < buffersCount; i++) {
        uniformBlocks.push_back(UniformBlockInfo(glprogram, i));
    }

    for (auto& info : uniformBlocks) {
        auto requestedBinding = slotBindings.find(info.name);
        if (requestedBinding != slotBindings.end()) {
            info.binding = (*requestedBinding)._location;
            glUniformBlockBinding(glprogram, info.index, info.binding);
            uniformBufferSlotMap[info.binding] = info.index;
        }
    }

    for (auto& info : uniformBlocks) {
        if (slotBindings.count(info.name)) {
            continue;
        }

        // If the binding is 0, or the binding maps to an already used binding
        if (info.binding == 0 || uniformBufferSlotMap[info.binding] != UNUSED_SLOT) {
            // If no binding was assigned then just do it finding a free slot
            auto slotIt = std::find_if(uniformBufferSlotMap.begin(), uniformBufferSlotMap.end(), isUnusedSlot);
            if (slotIt != uniformBufferSlotMap.end()) {
                info.binding = slotIt - uniformBufferSlotMap.begin();
                glUniformBlockBinding(glprogram, info.index, info.binding);
            } else {
                // This should neve happen, an active ubo cannot find an available slot among the max available?!
                info.binding = -1;
            }
        }

        uniformBufferSlotMap[info.binding] = info.index;
    }

    for (auto& info : uniformBlocks) {
        static const Element element(SCALAR, gpu::UINT32, gpu::UNIFORM_BUFFER);
        buffers.insert(Shader::Slot(info.name, info.binding, element, Resource::BUFFER, info.size));
    }
    return buffersCount;
}

int makeInputSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& inputs) {
    GLint inputsCount = 0;

    glGetProgramiv(glprogram, GL_ACTIVE_ATTRIBUTES, &inputsCount);

    for (int i = 0; i < inputsCount; i++) {
        const GLint NAME_LENGTH = 256;
        GLchar name[NAME_LENGTH];
        GLint length = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveAttrib(glprogram, i, NAME_LENGTH, &length, &size, &type, name);

        GLint binding = glGetAttribLocation(glprogram, name);

        auto elementResource = getFormatFromGLUniform(type);
        inputs.insert(Shader::Slot(name, binding, elementResource._element, -1));
    }

    return inputsCount;
}

int makeOutputSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& outputs) {
    /*   GLint outputsCount = 0;

    glGetProgramiv(glprogram, GL_ACTIVE_, &outputsCount);

    for (int i = 0; i < inputsCount; i++) {
    const GLint NAME_LENGTH = 256;
    GLchar name[NAME_LENGTH];
    GLint length = 0;
    GLint size = 0;
    GLenum type = 0;
    glGetActiveAttrib(glprogram, i, NAME_LENGTH, &length, &size, &type, name);

    auto element = getFormatFromGLUniform(type);
    outputs.insert(Shader::Slot(name, i, element));
    }
    */
    return 0; //inputsCount;
}


bool compileShader(GLenum shaderDomain, const std::string& shaderSource, const std::string& defines, GLuint &shaderObject, GLuint &programObject) {
    if (shaderSource.empty()) {
        qCDebug(gpugllogging) << "GLShader::compileShader - no GLSL shader source code ? so failed to create";
        return false;
    }

    // Create the shader object
    GLuint glshader = glCreateShader(shaderDomain);
    if (!glshader) {
        qCDebug(gpugllogging) << "GLShader::compileShader - failed to create the gl shader object";
        return false;
    }

    // Assign the source
    const int NUM_SOURCE_STRINGS = 2;
    const GLchar* srcstr[] = { defines.c_str(), shaderSource.c_str() };
    glShaderSource(glshader, NUM_SOURCE_STRINGS, srcstr, NULL);

    // Compile !
    glCompileShader(glshader);

    // check if shader compiled
    GLint compiled = 0;
    glGetShaderiv(glshader, GL_COMPILE_STATUS, &compiled);

    // if compilation fails
    if (!compiled) {

        // save the source code to a temp file so we can debug easily
        /*
        std::ofstream filestream;
        filestream.open("debugshader.glsl");
        if (filestream.is_open()) {
        filestream << srcstr[0];
        filestream << srcstr[1];
        filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetShaderiv(glshader, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength];
        glGetShaderInfoLog(glshader, infoLength, NULL, temp);


        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
        filestream << std::string(temp);
        filestream.close();
        }
        */

        qCWarning(gpugllogging) << "GLShader::compileShader - failed to compile the gl shader object:";
        for (auto s : srcstr) {
            qCWarning(gpugllogging) << s;
        }
        qCWarning(gpugllogging) << "GLShader::compileShader - errors:";
        qCWarning(gpugllogging) << temp;
        delete[] temp;

        glDeleteShader(glshader);
        return false;
    }

    GLuint glprogram = 0;
#ifdef SEPARATE_PROGRAM
    // so far so good, program is almost done, need to link:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(gpugllogging) << "GLShader::compileShader - failed to create the gl shader & gl program object";
        return false;
    }

    glProgramParameteri(glprogram, GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(glprogram, glshader);
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);

    if (!linked) {
        /*
        // save the source code to a temp file so we can debug easily
        std::ofstream filestream;
        filestream.open("debugshader.glsl");
        if (filestream.is_open()) {
        filestream << shaderSource->source;
        filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength];
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);

        qCDebug(gpugllogging) << "GLShader::compileShader -  failed to LINK the gl program object :";
        qCDebug(gpugllogging) << temp;

        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
        filestream << String(temp);
        filestream.close();
        }
        */
        delete[] temp;

        glDeleteShader(glshader);
        glDeleteProgram(glprogram);
        return false;
    }
#endif

    shaderObject = glshader;
    programObject = glprogram;

    return true;
}

GLuint compileProgram(const std::vector<GLuint>& glshaders) {
    // A brand new program:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(gpugllogging) << "GLShader::compileProgram - failed to create the gl program object";
        return 0;
    }

    // glProgramParameteri(glprogram, GL_PROGRAM_, GL_TRUE);
    // Create the program from the sub shaders
    for (auto so : glshaders) {
        glAttachShader(glprogram, so);
    }

    // Link!
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);

    if (!linked) {
        /*
        // save the source code to a temp file so we can debug easily
        std::ofstream filestream;
        filestream.open("debugshader.glsl");
        if (filestream.is_open()) {
        filestream << shaderSource->source;
        filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength];
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);

        qCDebug(gpugllogging) << "GLShader::compileProgram -  failed to LINK the gl program object :";
        qCDebug(gpugllogging) << temp;

        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
        filestream << std::string(temp);
        filestream.close();
        }
        */
        delete[] temp;

        glDeleteProgram(glprogram);
        return 0;
    }

    return glprogram;
}


void makeProgramBindings(ShaderObject& shaderObject) {
    if (!shaderObject.glprogram) {
        return;
    }
    GLuint glprogram = shaderObject.glprogram;
    GLint loc = -1;

    //Check for gpu specific attribute slotBindings
    loc = glGetAttribLocation(glprogram, "inPosition");
    if (loc >= 0 && loc != gpu::Stream::POSITION) {
        glBindAttribLocation(glprogram, gpu::Stream::POSITION, "inPosition");
    }

    loc = glGetAttribLocation(glprogram, "inNormal");
    if (loc >= 0 && loc != gpu::Stream::NORMAL) {
        glBindAttribLocation(glprogram, gpu::Stream::NORMAL, "inNormal");
    }

    loc = glGetAttribLocation(glprogram, "inColor");
    if (loc >= 0 && loc != gpu::Stream::COLOR) {
        glBindAttribLocation(glprogram, gpu::Stream::COLOR, "inColor");
    }

    loc = glGetAttribLocation(glprogram, "inTexCoord0");
    if (loc >= 0 && loc != gpu::Stream::TEXCOORD) {
        glBindAttribLocation(glprogram, gpu::Stream::TEXCOORD, "inTexCoord0");
    }

    loc = glGetAttribLocation(glprogram, "inTangent");
    if (loc >= 0 && loc != gpu::Stream::TANGENT) {
        glBindAttribLocation(glprogram, gpu::Stream::TANGENT, "inTangent");
    }

    loc = glGetAttribLocation(glprogram, "inTexCoord1");
    if (loc >= 0 && loc != gpu::Stream::TEXCOORD1) {
        glBindAttribLocation(glprogram, gpu::Stream::TEXCOORD1, "inTexCoord1");
    }

    loc = glGetAttribLocation(glprogram, "inSkinClusterIndex");
    if (loc >= 0 && loc != gpu::Stream::SKIN_CLUSTER_INDEX) {
        glBindAttribLocation(glprogram, gpu::Stream::SKIN_CLUSTER_INDEX, "inSkinClusterIndex");
    }

    loc = glGetAttribLocation(glprogram, "inSkinClusterWeight");
    if (loc >= 0 && loc != gpu::Stream::SKIN_CLUSTER_WEIGHT) {
        glBindAttribLocation(glprogram, gpu::Stream::SKIN_CLUSTER_WEIGHT, "inSkinClusterWeight");
    }

    loc = glGetAttribLocation(glprogram, "_drawCallInfo");
    if (loc >= 0 && loc != gpu::Stream::DRAW_CALL_INFO) {
        glBindAttribLocation(glprogram, gpu::Stream::DRAW_CALL_INFO, "_drawCallInfo");
    }

    // Link again to take into account the assigned attrib location
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);
    if (!linked) {
        qCDebug(gpugllogging) << "GLShader::makeBindings - failed to link after assigning slotBindings?";
    }

    // now assign the ubo binding, then DON't relink!

    //Check for gpu specific uniform slotBindings
#ifdef GPU_SSBO_DRAW_CALL_INFO
    loc = glGetProgramResourceIndex(glprogram, GL_SHADER_STORAGE_BLOCK, "transformObjectBuffer");
    if (loc >= 0) {
        glShaderStorageBlockBinding(glprogram, loc, gpu::TRANSFORM_OBJECT_SLOT);
        shaderObject.transformObjectSlot = gpu::TRANSFORM_OBJECT_SLOT;
    }
#else
    loc = glGetUniformLocation(glprogram, "transformObjectBuffer");
    if (loc >= 0) {
        glProgramUniform1i(glprogram, loc, gpu::TRANSFORM_OBJECT_SLOT);
        shaderObject.transformObjectSlot = gpu::TRANSFORM_OBJECT_SLOT;
    }
#endif

    loc = glGetUniformBlockIndex(glprogram, "transformCameraBuffer");
    if (loc >= 0) {
        glUniformBlockBinding(glprogram, loc, gpu::TRANSFORM_CAMERA_SLOT);
        shaderObject.transformCameraSlot = gpu::TRANSFORM_CAMERA_SLOT;
    }

    (void)CHECK_GL_ERROR();
}


} }

using namespace gpu;


