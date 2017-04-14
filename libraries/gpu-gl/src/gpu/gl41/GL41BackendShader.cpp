//
//  Created by Sam Gateau on 2017/04/13
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL41Backend.h"
#include "../gl/GLShader.h"
//#include <gl/GLShaders.h>

using namespace gpu;
using namespace gpu::gl41;

// GLSL version
std::string GL41Backend::getBackendShaderHeader() const {
    return std::string("#version 410 core");
}

int GL41Backend::makeResourceBufferSlots(GLuint glprogram, const Shader::BindingSet& slotBindings,Shader::SlotSet& resourceBuffers) {
    GLint ssboCount = 0;
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
        
            if (elementResource._resource == Resource::BUFFER) {
                 if (elementResource._element.getSemantic() == gpu::RESOURCE_BUFFER) {
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

                    // For texture/Sampler, the location is the actual binding value
                    GLint binding = -1;
                    glGetUniformiv(glprogram, location, &binding);

                    if (binding == GL41Backend::TRANSFORM_OBJECT_SLOT) {
                        continue;
                    }

                    auto requestedBinding = slotBindings.find(std::string(sname));
                    if (requestedBinding != slotBindings.end()) {
                        GLint requestedLoc = (*requestedBinding)._location + GL41Backend::RESOURCE_BUFFER_SLOT0_TEX_UNIT;
                        if (binding != requestedLoc) {
                            binding = requestedLoc;
                            glProgramUniform1i(glprogram, location, binding);
                        }
                    } else {
                        binding += GL41Backend::RESOURCE_BUFFER_SLOT0_TEX_UNIT;
                    }
                    ssboCount++;
                    resourceBuffers.insert(Shader::Slot(name, binding, elementResource._element, elementResource._resource));
                }
            }
        }
    }

    return ssboCount;

    /*glGetProgramInterfaceiv(glprogram, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &ssboCount);
    if (ssboCount > 0) {
        GLint maxNameLength = 0;
        glGetProgramInterfaceiv(glprogram, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, &maxNameLength);
        std::vector<GLchar> nameBytes(maxNameLength);

        for (GLint b = 0; b < ssboCount; b++) {
            GLint  length;
            glGetProgramResourceName(glprogram, GL_SHADER_STORAGE_BLOCK, b, maxNameLength, &length, nameBytes.data());

            std::string bufferName(nameBytes.data());
            qCWarning(gpugllogging) << "GLShader::makeBindings - " << bufferName.c_str();

            static const Element element(SCALAR, gpu::UINT32, gpu::UNIFORM_BUFFER);
            resourceBuffers.insert(Shader::Slot(bufferName, b, element, -1));
        }
    }*/
    return ssboCount;
}

void GL41Backend::makeProgramBindings(gl::ShaderObject& shaderObject) {
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
        qCWarning(gpugllogging) << "GLShader::makeBindings - failed to link after assigning slotBindings?";
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

