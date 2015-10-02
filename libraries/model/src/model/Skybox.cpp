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
    Data data;
    _dataBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Data), (const gpu::Byte*) &data));

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
    _dataBuffer.edit<Data>()._color = color;
}

void Skybox::setCubemap(const gpu::TexturePointer& cubemap) {
    _cubemap = cubemap;
}


void Skybox::updateDataBuffer() const {
    auto blend = 0.0f;
    if (getCubemap() && getCubemap()->isDefined()) {
        blend = 1.0f;
        // If pitch black neutralize the color
        if (glm::all(glm::equal(getColor(), glm::vec3(0.0f)))) {
            blend = 2.0f;
        }
    }

    if (blend != _dataBuffer.get<Data>()._blend) {
        _dataBuffer.edit<Data>()._blend = blend;
    }
}



void Skybox::render(gpu::Batch& batch, const ViewFrustum& frustum) const {
    updateDataBuffer();
    Skybox::render(batch, frustum, (*this));
}


void Skybox::render(gpu::Batch& batch, const ViewFrustum& viewFrustum, const Skybox& skybox) {
    // Create the static shared elements used to render the skybox
    static gpu::BufferPointer theBuffer;
    static gpu::Stream::FormatPointer theFormat;
    static gpu::BufferPointer theConstants;
    static gpu::PipelinePointer thePipeline;
    const int SKYBOX_SKYMAP_SLOT = 0;
    const int SKYBOX_CONSTANTS_SLOT = 0;
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
            bindings.insert(gpu::Shader::Binding(std::string("cubeMap"), SKYBOX_SKYMAP_SLOT));
            bindings.insert(gpu::Shader::Binding(std::string("skyboxBuffer"), SKYBOX_CONSTANTS_SLOT));
            if (!gpu::Shader::makeProgram(*skyShader, bindings)) {

            }

            auto skyState = std::make_shared<gpu::State>();
            skyState->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

            thePipeline = gpu::PipelinePointer(gpu::Pipeline::create(skyShader, skyState));
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
    if (skybox.getCubemap() && skybox.getCubemap()->isDefined()) {
        skymap = skybox.getCubemap();
    }

    batch.setPipeline(thePipeline);
    batch.setUniformBuffer(SKYBOX_CONSTANTS_SLOT, skybox._dataBuffer);
    batch.setResourceTexture(SKYBOX_SKYMAP_SLOT, skymap);

    batch.draw(gpu::TRIANGLE_STRIP, 4);

    batch.setResourceTexture(SKYBOX_SKYMAP_SLOT, nullptr);

}

