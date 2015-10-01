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


#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <ViewFrustum.h>

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
    const int VERTICES_SLOT = 0;
    const int COLOR_SLOT = 1;

    // Create the static shared elements used to render the skybox
    static gpu::BufferPointer theBuffer;
    static gpu::Stream::FormatPointer theFormat;
    static gpu::BufferPointer theConstants;
    static gpu::PipelinePointer thePipeline;
    static int SKYBOX_CONSTANTS_SLOT = 0; // need to be defined by the compilation of the shader
    static std::once_flag once;
    std::call_once(once, [&] {
        {
            const float CLIP = 1.0f;
            const glm::vec2 vertices[4] = { { -CLIP, -CLIP }, { CLIP, -CLIP }, { -CLIP, CLIP }, { CLIP, CLIP } };
            theBuffer = std::make_shared<gpu::Buffer>(sizeof(vertices), (const gpu::Byte*) vertices);
            theFormat = std::make_shared<gpu::Stream::Format>();
            theFormat->setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ));
        }

        {
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
            skyState->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

            thePipeline = gpu::PipelinePointer(gpu::Pipeline::create(skyShader, skyState));

            auto color = glm::vec4(1.0f);
            theConstants = std::make_shared<gpu::Buffer>(sizeof(color), (const gpu::Byte*) &color);
        }
    });

    // Render
    glm::mat4 projMat;
    viewFrustum.evalProjectionMatrix(projMat);

    Transform viewTransform;
    viewFrustum.evalViewTransform(viewTransform);
    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewTransform);
    batch.setModelTransform(Transform()); // only for Mac
    batch.setInputBuffer(gpu::Stream::POSITION, theBuffer, 0, 8);
    batch.setInputFormat(theFormat);

    gpu::TexturePointer skymap;
    auto color = glm::vec4(skybox.getColor(), 0.0f);
    if (skybox.getCubemap() && skybox.getCubemap()->isDefined()) {
        skymap = skybox.getCubemap();

        if (glm::all(glm::equal(skybox.getColor(), glm::vec3(0.0f)))) { 
            color = glm::vec4(1.0f);
        } else {
            color.w = 1.0f;
        }
    }
    // Update the constant color.
    theConstants->setSubData(0, sizeof(glm::vec4), (const gpu::Byte*) &color);

    batch.setPipeline(thePipeline);
    batch.setUniformBuffer(SKYBOX_CONSTANTS_SLOT, theConstants, 0, theConstants->getSize());
    batch.setResourceTexture(0, skymap);

    batch.draw(gpu::TRIANGLE_STRIP, 4);

    batch.setResourceTexture(0, nullptr);

}

