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

#include "skybox_vert.h"
#include "skybox_frag.h"

using namespace model;

Skybox::Skybox() {
    Schema schema;
    _schemaBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Schema), (const gpu::Byte*) &schema));
}

void Skybox::setColor(const Color& color) {
    _empty = false;
    _schemaBuffer.edit<Schema>().color = color;
}

void Skybox::setCubemap(const gpu::TexturePointer& cubemap) {
    _empty = false;
    _cubemap = cubemap;
}

void Skybox::updateSchemaBuffer() const {
    auto blend = 0.0f;
    if (getCubemap() && getCubemap()->isDefined()) {
        blend = 0.5f;

        // If pitch black neutralize the color
        if (glm::all(glm::equal(getColor(), glm::vec3(0.0f)))) {
            blend = 1.0f;
        }
    }

    if (blend != _schemaBuffer.get<Schema>().blend) {
        _schemaBuffer.edit<Schema>().blend = blend;
    }
}

void Skybox::clear() {
    _empty = true;
    _schemaBuffer.edit<Schema>().color = vec3(0);
    setCubemap(nullptr);
}

void Skybox::prepare(gpu::Batch& batch, int textureSlot, int bufferSlot) const {
    if (bufferSlot > -1) {
        batch.setUniformBuffer(bufferSlot, _schemaBuffer);
    }

    if (textureSlot > -1) {
        gpu::TexturePointer skymap = getCubemap();
        // FIXME: skymap->isDefined may not be threadsafe
        if (skymap && skymap->isDefined()) {
            batch.setResourceTexture(textureSlot, skymap);
        }
    }
}

void Skybox::render(gpu::Batch& batch, const ViewFrustum& frustum) const {
    updateSchemaBuffer();
    Skybox::render(batch, frustum, (*this));
}

void Skybox::render(gpu::Batch& batch, const ViewFrustum& viewFrustum, const Skybox& skybox) {
    // Create the static shared elements used to render the skybox
    static gpu::BufferPointer theConstants;
    static gpu::PipelinePointer thePipeline;
    static std::once_flag once;
    std::call_once(once, [&] {
        {
            auto skyVS = gpu::Shader::createVertex(std::string(skybox_vert));
            auto skyFS = gpu::Shader::createPixel(std::string(skybox_frag));
            auto skyShader = gpu::Shader::createProgram(skyVS, skyFS);

            gpu::Shader::BindingSet bindings;
            bindings.insert(gpu::Shader::Binding(std::string("cubeMap"), SKYBOX_SKYMAP_SLOT));
            bindings.insert(gpu::Shader::Binding(std::string("skyboxBuffer"), SKYBOX_CONSTANTS_SLOT));
            if (!gpu::Shader::makeProgram(*skyShader, bindings)) {

            }

            auto skyState = std::make_shared<gpu::State>();
            skyState->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

            thePipeline = gpu::Pipeline::create(skyShader, skyState);
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

    batch.setPipeline(thePipeline);
    skybox.prepare(batch);
    batch.draw(gpu::TRIANGLE_STRIP, 4);

    batch.setResourceTexture(SKYBOX_SKYMAP_SLOT, nullptr);
}
