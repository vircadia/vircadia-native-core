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
    return std::string("#version 410 core\n#define GPU_GL410 1");
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
                        }
                    } else {
                        binding += GL41Backend::RESOURCE_BUFFER_SLOT0_TEX_UNIT;
                    }
                    glProgramUniform1i(glprogram, location, binding);

                    ssboCount++;
                    resourceBuffers.insert(Shader::Slot(name, binding, elementResource._element, elementResource._resource));
                }
            }
        }
    }

    return ssboCount;
}

void GL41Backend::makeProgramBindings(gl::ShaderObject& shaderObject) {
    if (!shaderObject.glprogram) {
        return;
    }
    GLuint glprogram = shaderObject.glprogram;
    GLint loc = -1;

    GLBackend::makeProgramBindings(shaderObject);

    // now assign the ubo binding, then DON't relink!

    //Check for gpu specific uniform slotBindings
    loc = glGetUniformLocation(glprogram, "transformObjectBuffer");
    if (loc >= 0) {
        glProgramUniform1i(glprogram, loc, GL41Backend::TRANSFORM_OBJECT_SLOT);
        shaderObject.transformObjectSlot = GL41Backend::TRANSFORM_OBJECT_SLOT;
    }

    loc = glGetUniformBlockIndex(glprogram, "transformCameraBuffer");
    if (loc >= 0) {
        glUniformBlockBinding(glprogram, loc, gpu::TRANSFORM_CAMERA_SLOT);
        shaderObject.transformCameraSlot = gpu::TRANSFORM_CAMERA_SLOT;
    }

    (void)CHECK_GL_ERROR();
}

