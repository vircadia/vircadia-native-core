//
//  Created by Sam Gateau on 2017/04/13
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"
#include "../gl/GLShader.h"
//#include <gl/GLShaders.h>

using namespace gpu;
using namespace gpu::gl45;

// GLSL version
std::string GL45Backend::getBackendShaderHeader() const {
    const char header[] = 
R"GLSL(#version 450 core
#define GPU_GL450
)GLSL"
#ifdef GPU_SSBO_TRANSFORM_OBJECT
        R"GLSL(#define GPU_SSBO_TRANSFORM_OBJECT 1)GLSL"
#endif
    ;
    return std::string(header);
}

int GL45Backend::makeResourceBufferSlots(GLuint glprogram, const Shader::BindingSet& slotBindings,Shader::SlotSet& resourceBuffers) {
    GLint buffersCount = 0;
    glGetProgramInterfaceiv(glprogram, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &buffersCount);

    // fast exit
    if (buffersCount == 0) {
        return 0;
    }

    GLint maxNumResourceBufferSlots = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxNumResourceBufferSlots);
    std::vector<GLint> resourceBufferSlotMap(maxNumResourceBufferSlots, -1);

    struct ResourceBlockInfo {
        using Vector = std::vector<ResourceBlockInfo>;
        const GLuint index{ 0 };
        const std::string name;
        GLint binding{ -1 };
        GLint size{ 0 };

        static std::string getName(GLuint glprogram, GLuint i) {
            static const GLint NAME_LENGTH = 256;
            GLint length = 0;
            GLchar nameBuffer[NAME_LENGTH];
            glGetProgramResourceName(glprogram, GL_SHADER_STORAGE_BLOCK, i, NAME_LENGTH, &length, nameBuffer);
            return std::string(nameBuffer);
        }

        ResourceBlockInfo(GLuint glprogram, GLuint i) : index(i), name(getName(glprogram, i)) {
            GLenum props[2] = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE};
            glGetProgramResourceiv(glprogram, GL_SHADER_STORAGE_BLOCK, i, 2, props, 2, nullptr, &binding); 
        }
    };

    ResourceBlockInfo::Vector resourceBlocks;
    resourceBlocks.reserve(buffersCount);
    for (int i = 0; i < buffersCount; i++) {
        resourceBlocks.push_back(ResourceBlockInfo(glprogram, i));
    }

    for (auto& info : resourceBlocks) {
        auto requestedBinding = slotBindings.find(info.name);
        if (requestedBinding != slotBindings.end()) {
            info.binding = (*requestedBinding)._location;
            glUniformBlockBinding(glprogram, info.index, info.binding);
            resourceBufferSlotMap[info.binding] = info.index;
        }
    }

    for (auto& info : resourceBlocks) {
        if (slotBindings.count(info.name)) {
            continue;
        }

        // If the binding is -1, or the binding maps to an already used binding
        if (info.binding == -1 || !isUnusedSlot(resourceBufferSlotMap[info.binding])) {
            // If no binding was assigned then just do it finding a free slot
            auto slotIt = std::find_if(resourceBufferSlotMap.begin(), resourceBufferSlotMap.end(), GLBackend::isUnusedSlot);
            if (slotIt != resourceBufferSlotMap.end()) {
                info.binding = slotIt - resourceBufferSlotMap.begin();
                glUniformBlockBinding(glprogram, info.index, info.binding);
            } else {
                // This should never happen, an active ssbo cannot find an available slot among the max available?!
                info.binding = -1;
            }
        }

        resourceBufferSlotMap[info.binding] = info.index;
    }

    for (auto& info : resourceBlocks) {
        static const Element element(SCALAR, gpu::UINT32, gpu::RESOURCE_BUFFER);
        resourceBuffers.insert(Shader::Slot(info.name, info.binding, element, Resource::BUFFER, info.size));
    }
    return buffersCount;
/*
    GLint ssboCount = 0;
    glGetProgramInterfaceiv(glprogram, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &ssboCount);
    if (ssboCount > 0) {
        GLint maxNameLength = 0;
        glGetProgramInterfaceiv(glprogram, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, &maxNameLength);
        std::vector<GLchar> nameBytes(maxNameLength);

        for (GLint b = 0; b < ssboCount; b++) {
            GLint  length;
            glGetProgramResourceName(glprogram, GL_SHADER_STORAGE_BLOCK, b, maxNameLength, &length, nameBytes.data());
            std::string bufferName(nameBytes.data());

            GLenum props = GL_BUFFER_BINDING;
            GLint binding = -1;
            glGetProgramResourceiv(glprogram, GL_SHADER_STORAGE_BLOCK, b, 1, &props, 1, nullptr, &binding); 

            auto requestedBinding = slotBindings.find(std::string(bufferName));
            if (requestedBinding != slotBindings.end()) {
                if (binding != (*requestedBinding)._location) {
                    binding = (*requestedBinding)._location;
                    glShaderStorageBlockBinding(glprogram, b, binding);
                }
            }

            static const Element element(SCALAR, gpu::UINT32, gpu::RESOURCE_BUFFER);
            resourceBuffers.insert(Shader::Slot(bufferName, binding, element, -1));
        }
    }
    return ssboCount;*/
}

void GL45Backend::makeProgramBindings(gl::ShaderObject& shaderObject) {
    if (!shaderObject.glprogram) {
        return;
    }
    GLuint glprogram = shaderObject.glprogram;
    GLint loc = -1;

    GLBackend::makeProgramBindings(shaderObject);

    // now assign the ubo binding, then DON't relink!

    //Check for gpu specific uniform slotBindings
#ifdef GPU_SSBO_TRANSFORM_OBJECT
    loc = glGetProgramResourceIndex(glprogram, GL_SHADER_STORAGE_BLOCK, "transformObjectBuffer");
    if (loc >= 0) {
        glShaderStorageBlockBinding(glprogram, loc, GL45Backend::TRANSFORM_OBJECT_SLOT);
        shaderObject.transformObjectSlot = GL45Backend::TRANSFORM_OBJECT_SLOT;
    }
#else
    loc = glGetUniformLocation(glprogram, "transformObjectBuffer");
    if (loc >= 0) {
        glProgramUniform1i(glprogram, loc, GL45Backend::TRANSFORM_OBJECT_SLOT);
        shaderObject.transformObjectSlot = GL45Backend::TRANSFORM_OBJECT_SLOT;
    }
#endif

    loc = glGetUniformBlockIndex(glprogram, "transformCameraBuffer");
    if (loc >= 0) {
        glUniformBlockBinding(glprogram, loc, gpu::TRANSFORM_CAMERA_SLOT);
        shaderObject.transformCameraSlot = gpu::TRANSFORM_CAMERA_SLOT;
    }

    (void)CHECK_GL_ERROR();
}

