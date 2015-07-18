//
//  Skybox.cpp
//  libraries/model/src/model
//
//  Created by Sam Gateau on 5/4/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Skybox.h"

#include "gpu/Batch.h"
#include "gpu/Context.h"

#include "ViewFrustum.h"
#include "Skybox_vert.h"
#include "Skybox_frag.h"

using namespace model;

Skybox::Skybox() {

/* // PLease create a default engineer skybox
    _cubemap.reset( gpu::Texture::createCube(gpu::Element::COLOR_RGBA_32, 1));
    unsigned char texels[] = {
        255, 0, 0, 255,
        0, 255, 255, 255,
        0, 0, 255, 255,
        255, 255, 0, 255,
        0, 255, 0, 255,
        255, 0, 255, 255,
    };
    _cubemap->assignStoredMip(0, gpu::Element::COLOR_RGBA_32, sizeof(texels), texels);*/
}

void Skybox::setColor(const Color& color) {
    _color = color;
}

void Skybox::setCubemap(const gpu::TexturePointer& cubemap) {
    _cubemap = cubemap;
}

void Skybox::render(gpu::Batch& batch, const ViewFrustum& viewFrustum, const Skybox& skybox) {

    if (skybox.getCubemap()) {
        if (skybox.getCubemap()->isDefined()) {

            static gpu::PipelinePointer thePipeline;
            static gpu::BufferPointer theBuffer;
            static gpu::Stream::FormatPointer theFormat;
            static gpu::BufferPointer theConstants;
            static int SKYBOX_CONSTANTS_SLOT = 0; // need to be defined by the compilation of the shader
            if (!thePipeline) {
                auto skyVS = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(Skybox_vert)));
                auto skyFS = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(Skybox_frag)));
                auto skyShader = gpu::ShaderPointer(gpu::Shader::createProgram(skyVS, skyFS));

                gpu::Shader::BindingSet bindings;
                bindings.insert(gpu::Shader::Binding(std::string("cubeMap"), 0));
                if (!gpu::Shader::makeProgram(*skyShader, bindings)) {

                }

                SKYBOX_CONSTANTS_SLOT = skyShader->getBuffers().findLocation("skyboxBuffer");
                if (SKYBOX_CONSTANTS_SLOT == gpu::Shader::INVALID_LOCATION) {
                    SKYBOX_CONSTANTS_SLOT = skyShader->getUniforms().findLocation("skyboxBuffer");
                }
            
                auto skyState = std::make_shared<gpu::State>();

                thePipeline = gpu::PipelinePointer(gpu::Pipeline::create(skyShader, skyState));
        
                const float CLIP = 1.0f;
                const glm::vec2 vertices[4] = { {-CLIP, -CLIP}, {CLIP, -CLIP}, {-CLIP, CLIP}, {CLIP, CLIP}};
                theBuffer = std::make_shared<gpu::Buffer>(sizeof(vertices), (const gpu::Byte*) vertices);
        
                theFormat = std::make_shared<gpu::Stream::Format>();
                theFormat->setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ));
        
                auto color = glm::vec4(1.0f);
                theConstants = std::make_shared<gpu::Buffer>(sizeof(color), (const gpu::Byte*) &color);
            }

            glm::mat4 projMat;
            viewFrustum.evalProjectionMatrix(projMat);

            Transform viewTransform;
            viewFrustum.evalViewTransform(viewTransform);

            if (glm::all(glm::equal(skybox.getColor(), glm::vec3(0.0f)))) { 
                auto color = glm::vec4(1.0f);
                theConstants->setSubData(0, sizeof(color), (const gpu::Byte*) &color);
            } else {
                theConstants->setSubData(0, sizeof(Color), (const gpu::Byte*) &skybox.getColor());
            }

            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewTransform);
            batch.setModelTransform(Transform()); // only for Mac
            batch.setPipeline(thePipeline);
            batch.setInputBuffer(gpu::Stream::POSITION, theBuffer, 0, 8);
            batch.setUniformBuffer(SKYBOX_CONSTANTS_SLOT, theConstants, 0, theConstants->getSize());
            batch.setInputFormat(theFormat);
            batch.setResourceTexture(0, skybox.getCubemap());
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }

    } else {
        // skybox has no cubemap, just clear the color buffer
        auto color = skybox.getColor();
        batch.clearFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(color, 0.0f), 0.0f, 0);
    }
}

