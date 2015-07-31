//
//  GLBackendShader.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 2/28/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GPULogging.h"
#include "GLBackendShared.h"
#include "Format.h"

using namespace gpu;

GLBackend::GLShader::GLShader() :
    _shader(0),
    _program(0)
{}

GLBackend::GLShader::~GLShader() {
    if (_shader != 0) {
        glDeleteShader(_shader);
    }
    if (_program != 0) {
        glDeleteProgram(_program);
    }
}

void makeBindings(GLBackend::GLShader* shader) {
    if(!shader || !shader->_program) {
        return;
    }
    GLuint glprogram = shader->_program;
    GLint loc = -1;
     
    //Check for gpu specific attribute slotBindings
    loc = glGetAttribLocation(glprogram, "position");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::POSITION, "position");
    }

    loc = glGetAttribLocation(glprogram, "attribPosition");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::POSITION, "attribPosition");
    }

    //Check for gpu specific attribute slotBindings
    loc = glGetAttribLocation(glprogram, "gl_Vertex");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::POSITION, "gl_Vertex");
    }

    loc = glGetAttribLocation(glprogram, "normal");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::NORMAL, "normal");
    }
    loc = glGetAttribLocation(glprogram, "attribNormal");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::NORMAL, "attribNormal");
    }

    loc = glGetAttribLocation(glprogram, "color");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::COLOR, "color");
    }
    loc = glGetAttribLocation(glprogram, "attribColor");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::COLOR, "attribColor");
    }

    loc = glGetAttribLocation(glprogram, "texcoord");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::TEXCOORD, "texcoord");
    }
    loc = glGetAttribLocation(glprogram, "attribTexcoord");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::TEXCOORD, "attribTexcoord");
    }
    
    loc = glGetAttribLocation(glprogram, "tangent");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::TANGENT, "tangent");
    }

    loc = glGetAttribLocation(glprogram, "texcoord1");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::TEXCOORD1, "texcoord1");
    }
    loc = glGetAttribLocation(glprogram, "attribTexcoord1");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::TEXCOORD1, "texcoord1");
    }

    loc = glGetAttribLocation(glprogram, "clusterIndices");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::SKIN_CLUSTER_INDEX, "clusterIndices");
    }

    loc = glGetAttribLocation(glprogram, "clusterWeights");
    if (loc >= 0) {
        glBindAttribLocation(glprogram, gpu::Stream::SKIN_CLUSTER_WEIGHT, "clusterWeights");
    }

    // Link again to take into account the assigned attrib location
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);
    if (!linked) {
        qCDebug(gpulogging) << "GLShader::makeBindings - failed to link after assigning slotBindings?";
    }

    // now assign the ubo binding, then DON't relink!

    //Check for gpu specific uniform slotBindings
#if (GPU_TRANSFORM_PROFILE == GPU_CORE)
    loc = glGetUniformBlockIndex(glprogram, "transformObjectBuffer");
    if (loc >= 0) {
        glUniformBlockBinding(glprogram, loc, gpu::TRANSFORM_OBJECT_SLOT);
        shader->_transformObjectSlot = gpu::TRANSFORM_OBJECT_SLOT;
    }

    loc = glGetUniformBlockIndex(glprogram, "transformCameraBuffer");
    if (loc >= 0) {
        glUniformBlockBinding(glprogram, loc, gpu::TRANSFORM_CAMERA_SLOT);
        shader->_transformCameraSlot = gpu::TRANSFORM_CAMERA_SLOT;
    }
#else
    loc = glGetUniformLocation(glprogram, "transformObject_model");
    if (loc >= 0) {
        shader->_transformObject_model = loc;
    }

    loc = glGetUniformLocation(glprogram, "transformCamera_viewInverse");
    if (loc >= 0) {
        shader->_transformCamera_viewInverse = loc;
    }

    loc = glGetUniformLocation(glprogram, "transformCamera_viewport");
    if (loc >= 0) {
        shader->_transformCamera_viewport = loc;
    }
#endif
}

GLBackend::GLShader* compileShader(const Shader& shader) {
    // Any GLSLprogram ? normally yes...
    const std::string& shaderSource = shader.getSource().getCode();
    if (shaderSource.empty()) {
        qCDebug(gpulogging) << "GLShader::compileShader - no GLSL shader source code ? so failed to create";
        return nullptr;
    }

    // Shader domain
    const GLenum SHADER_DOMAINS[2] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    GLenum shaderDomain = SHADER_DOMAINS[shader.getType()];

    // Create the shader object
    GLuint glshader = glCreateShader(shaderDomain);
    if (!glshader) {
        qCDebug(gpulogging) << "GLShader::compileShader - failed to create the gl shader object";
        return nullptr;
    }

    // Assign the source
    const GLchar* srcstr = shaderSource.c_str();
    glShaderSource(glshader, 1, &srcstr, NULL);

    // Compile !
    glCompileShader(glshader);

    // check if shader compiled
    GLint compiled = 0;
    glGetShaderiv(glshader, GL_COMPILE_STATUS, &compiled);

    // if compilation fails
    if (!compiled) {
        // save the source code to a temp file so we can debug easily
       /* std::ofstream filestream;
        filestream.open("debugshader.glsl");
        if (filestream.is_open()) {
            filestream << shaderSource->source;
            filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetShaderiv(glshader, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength] ;
        glGetShaderInfoLog(glshader, infoLength, NULL, temp);

        qCDebug(gpulogging) << "GLShader::compileShader - failed to compile the gl shader object:";
        qCDebug(gpulogging) << temp;

        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
            filestream << std::string(temp);
            filestream.close();
        }
        */
        delete[] temp;

        glDeleteShader(glshader);
        return nullptr;
    }

    GLuint glprogram = 0;
#ifdef SEPARATE_PROGRAM
    // so far so good, program is almost done, need to link:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(gpulogging) << "GLShader::compileShader - failed to create the gl shader & gl program object";
        return nullptr;
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

        char* temp = new char[infoLength] ;
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);

        qCDebug(gpulogging) << "GLShader::compileShader -  failed to LINK the gl program object :";
        qCDebug(gpulogging) << temp;

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
        return nullptr;
    }
#endif

    // So far so good, the shader is created successfully
    GLBackend::GLShader* object = new GLBackend::GLShader();
    object->_shader = glshader;
    object->_program = glprogram;

    makeBindings(object);

    return object;
}

GLBackend::GLShader* compileProgram(const Shader& program) {
    if(!program.isProgram()) {
        return nullptr;
    }

    // Let's go through every shaders and make sure they are ready to go
    std::vector< GLuint > shaderObjects;
    for (auto subShader : program.getShaders()) {
        GLuint so = GLBackend::getShaderID(subShader);
        if (!so) {
            qCDebug(gpulogging) << "GLShader::compileProgram - One of the shaders of the program is not compiled?";
            return nullptr;
        }
        shaderObjects.push_back(so);
    }

    // so far so good, program is almost done, need to link:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(gpulogging) << "GLShader::compileProgram - failed to create the gl program object";
        return nullptr;
    }

    // glProgramParameteri(glprogram, GL_PROGRAM_, GL_TRUE);
    // Create the program from the sub shaders
    for (auto so : shaderObjects) {
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

        char* temp = new char[infoLength] ;
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);

        qCDebug(gpulogging) << "GLShader::compileProgram -  failed to LINK the gl program object :";
        qCDebug(gpulogging) << temp;

        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
            filestream << std::string(temp);
            filestream.close();
        }
        */
        delete[] temp;

        glDeleteProgram(glprogram);
        return nullptr;
    }

    // So far so good, the program is created successfully
    GLBackend::GLShader* object = new GLBackend::GLShader();
    object->_shader = 0;
    object->_program = glprogram;

    makeBindings(object);

    return object;
}

GLBackend::GLShader* GLBackend::syncGPUObject(const Shader& shader) {
    GLShader* object = Backend::getGPUObject<GLBackend::GLShader>(shader);

    // If GPU object already created then good
    if (object) {
        return object;
    }
    // need to have a gpu object?
    if (shader.isProgram()) {
         GLShader* tempObject = compileProgram(shader);
         if (tempObject) {
            object = tempObject;
            Backend::setGPUObject(shader, object);
        }
    } else if (shader.isDomain()) {
         GLShader* tempObject = compileShader(shader);
         if (tempObject) {
            object = tempObject;
            Backend::setGPUObject(shader, object);
        }
    }

    return object;
}


GLuint GLBackend::getShaderID(const ShaderPointer& shader) {
    if (!shader) {
        return 0;
    }
    GLShader* object = GLBackend::syncGPUObject(*shader);
    if (object) {
        if (shader->isProgram()) {
            return object->_program;
        } else {
            return object->_shader;
        }
    } else {
        return 0;
    }
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

/*    {GL_FLOAT_MAT2x3	mat2x3},
    {GL_FLOAT_MAT2x4	mat2x4},
    {GL_FLOAT_MAT3x2	mat3x2},
    {GL_FLOAT_MAT3x4	mat3x4},
    {GL_FLOAT_MAT4x2	mat4x2},
    {GL_FLOAT_MAT4x3	mat4x3},
    {GL_DOUBLE_MAT2	dmat2},
    {GL_DOUBLE_MAT3	dmat3},
    {GL_DOUBLE_MAT4	dmat4},
    {GL_DOUBLE_MAT2x3	dmat2x3},
    {GL_DOUBLE_MAT2x4	dmat2x4},
    {GL_DOUBLE_MAT3x2	dmat3x2},
    {GL_DOUBLE_MAT3x4	dmat3x4},
    {GL_DOUBLE_MAT4x2	dmat4x2},
    {GL_DOUBLE_MAT4x3	dmat4x3},
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
            
//    {GL_SAMPLER_1D_SHADOW	sampler1DShadow},
 //   {GL_SAMPLER_1D_ARRAY_SHADOW	sampler1DArrayShadow},

//    {GL_SAMPLER_BUFFER	samplerBuffer},
//    {GL_SAMPLER_2D_RECT	sampler2DRect},
 //   {GL_SAMPLER_2D_RECT_SHADOW	sampler2DRectShadow},

#if defined(Q_OS_WIN)
    case GL_INT_SAMPLER_1D: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_1D);
    case GL_INT_SAMPLER_2D: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_2D);
    case GL_INT_SAMPLER_2D_MULTISAMPLE: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D);
    case GL_INT_SAMPLER_3D: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_3D);
    case GL_INT_SAMPLER_CUBE: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_CUBE);
          
    case GL_INT_SAMPLER_1D_ARRAY: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_1D_ARRAY);
    case GL_INT_SAMPLER_2D_ARRAY: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_2D_ARRAY);
    case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D_ARRAY);

 //   {GL_INT_SAMPLER_BUFFER	isamplerBuffer},
 //   {GL_INT_SAMPLER_2D_RECT	isampler2DRect},

    case GL_UNSIGNED_INT_SAMPLER_1D: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_1D);
    case GL_UNSIGNED_INT_SAMPLER_2D: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_2D);
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D);
    case GL_UNSIGNED_INT_SAMPLER_3D: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_3D);
    case GL_UNSIGNED_INT_SAMPLER_CUBE: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_CUBE);
          
    case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_1D_ARRAY);
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_2D_ARRAY);
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D_ARRAY);
#endif
//    {GL_UNSIGNED_INT_SAMPLER_BUFFER	usamplerBuffer},
//    {GL_UNSIGNED_INT_SAMPLER_2D_RECT	usampler2DRect},
/*
    {GL_IMAGE_1D	image1D},
    {GL_IMAGE_2D	image2D},
    {GL_IMAGE_3D	image3D},
    {GL_IMAGE_2D_RECT	image2DRect},
    {GL_IMAGE_CUBE	imageCube},
    {GL_IMAGE_BUFFER	imageBuffer},
    {GL_IMAGE_1D_ARRAY	image1DArray},
    {GL_IMAGE_2D_ARRAY	image2DArray},
    {GL_IMAGE_2D_MULTISAMPLE	image2DMS},
    {GL_IMAGE_2D_MULTISAMPLE_ARRAY	image2DMSArray},
    {GL_INT_IMAGE_1D	iimage1D},
    {GL_INT_IMAGE_2D	iimage2D},
    {GL_INT_IMAGE_3D	iimage3D},
    {GL_INT_IMAGE_2D_RECT	iimage2DRect},
    {GL_INT_IMAGE_CUBE	iimageCube},
    {GL_INT_IMAGE_BUFFER	iimageBuffer},
    {GL_INT_IMAGE_1D_ARRAY	iimage1DArray},
    {GL_INT_IMAGE_2D_ARRAY	iimage2DArray},
    {GL_INT_IMAGE_2D_MULTISAMPLE	iimage2DMS},
    {GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY	iimage2DMSArray},
    {GL_UNSIGNED_INT_IMAGE_1D	uimage1D},
    {GL_UNSIGNED_INT_IMAGE_2D	uimage2D},
    {GL_UNSIGNED_INT_IMAGE_3D	uimage3D},
    {GL_UNSIGNED_INT_IMAGE_2D_RECT	uimage2DRect},
    {GL_UNSIGNED_INT_IMAGE_CUBE	uimageCube},+		[0]	{_name="fInnerRadius" _location=0 _element={_semantic=15 '\xf' _dimension=0 '\0' _type=0 '\0' } }	gpu::Shader::Slot

    {GL_UNSIGNED_INT_IMAGE_BUFFER	uimageBuffer},
    {GL_UNSIGNED_INT_IMAGE_1D_ARRAY	uimage1DArray},
    {GL_UNSIGNED_INT_IMAGE_2D_ARRAY	uimage2DArray},
    {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE	uimage2DMS},
    {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY	uimage2DMSArray},
    {GL_UNSIGNED_INT_ATOMIC_COUNTER	atomic_uint}
*/
    default:
        return ElementResource(Element(), Resource::BUFFER);
    }

};


int makeUniformSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& uniforms, Shader::SlotSet& textures, Shader::SlotSet& samplers) {
    GLint uniformsCount = 0;

#if (GPU_FEATURE_PROFILE == GPU_LEGACY)
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    glUseProgram(glprogram);
#endif

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
#if (GPU_FEATURE_PROFILE == GPU_LEGACY)
                        glUniform1i(location, binding);
#else
                        glProgramUniform1i(glprogram, location, binding);
#endif
                    }
                }

                textures.insert(Shader::Slot(name, binding, elementResource._element, elementResource._resource));
                samplers.insert(Shader::Slot(name, binding, elementResource._element, elementResource._resource));
            }
        }
    }

#if (GPU_FEATURE_PROFILE == GPU_LEGACY)
    glUseProgram(currentProgram);
#endif

    return uniformsCount;
}

const GLint UNUSED_SLOT = -1;
bool isUnusedSlot(GLint binding) {
    return (binding == UNUSED_SLOT);
}

int makeUniformBlockSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& buffers) {
    GLint buffersCount = 0;

#if (GPU_FEATURE_PROFILE == GPU_CORE)

    glGetProgramiv(glprogram, GL_ACTIVE_UNIFORM_BLOCKS, &buffersCount);

    // fast exit
    if (buffersCount == 0) {
        return 0;
    }

    GLint maxNumUniformBufferSlots = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxNumUniformBufferSlots);
    std::vector<GLint> uniformBufferSlotMap(maxNumUniformBufferSlots, -1);

    for (int i = 0; i < buffersCount; i++) {
        const GLint NAME_LENGTH = 256;
        GLchar name[NAME_LENGTH];
        GLint length = 0;
        GLint size = 0;
        GLenum type = 0;
        GLint binding = -1;

        glGetActiveUniformBlockiv(glprogram, i, GL_UNIFORM_BLOCK_NAME_LENGTH, &length);
        glGetActiveUniformBlockName(glprogram, i, NAME_LENGTH, &length, name);
        glGetActiveUniformBlockiv(glprogram, i, GL_UNIFORM_BLOCK_BINDING, &binding);
        glGetActiveUniformBlockiv(glprogram, i, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
        
        GLuint blockIndex = glGetUniformBlockIndex(glprogram, name);

        // CHeck if there is a requested binding for this block
        auto requestedBinding = slotBindings.find(std::string(name));
        if (requestedBinding != slotBindings.end()) {
        // If yes force it
            if (binding != (*requestedBinding)._location) {
                binding = (*requestedBinding)._location;
                glUniformBlockBinding(glprogram, blockIndex, binding);
            }
        } else if (binding == 0) {
        // If no binding was assigned then just do it finding a free slot
            auto slotIt = std::find_if(uniformBufferSlotMap.begin(), uniformBufferSlotMap.end(), isUnusedSlot); 
            if (slotIt != uniformBufferSlotMap.end()) {
                binding = slotIt - uniformBufferSlotMap.begin();
                glUniformBlockBinding(glprogram, blockIndex, binding);
            } else {
                // This should neve happen, an active ubo cannot find an available slot among the max available?!
                binding = -1;
            }
        }
        // If binding is valid record it
        if (binding >= 0) {
            uniformBufferSlotMap[binding] = blockIndex;
        }

        Element element(SCALAR, gpu::UINT32, gpu::UNIFORM_BUFFER);
        buffers.insert(Shader::Slot(name, binding, element, Resource::BUFFER));
    }
#endif
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

bool GLBackend::makeProgram(Shader& shader, const Shader::BindingSet& slotBindings) {

    // First make sure the Shader has been compiled
    GLShader* object = GLBackend::syncGPUObject(shader);
    if (!object) {
        return false;
    }

    if (object->_program) {
        Shader::SlotSet buffers;
        makeUniformBlockSlots(object->_program, slotBindings, buffers);

        Shader::SlotSet uniforms;
        Shader::SlotSet textures;
        Shader::SlotSet samplers;
        makeUniformSlots(object->_program, slotBindings, uniforms, textures, samplers);

        Shader::SlotSet inputs;
        makeInputSlots(object->_program, slotBindings, inputs);

        Shader::SlotSet outputs;
        makeOutputSlots(object->_program, slotBindings, outputs);

        shader.defineSlots(uniforms, buffers, textures, samplers, inputs, outputs);

    } else if (object->_shader) {

    }

    return true;
}

