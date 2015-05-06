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
#include "skybox_vert.h"
#include "skybox_frag.h"

using namespace model;

Skybox::Skybox() {

    _cubemap.reset( gpu::Texture::createCube(gpu::Element::COLOR_RGBA_32, 1));
    unsigned char texels[] = {
        255, 0, 0, 255,
        0, 255, 255, 255,
        0, 0, 255, 255,
        255, 255, 0, 255,
        0, 255, 0, 255,
        255, 0, 255, 255,
    };
    _cubemap->assignStoredMip(0, gpu::Element::COLOR_RGBA_32, sizeof(texels), texels); 
}

void Skybox::setColor(const Color& color) {
    _color = color;
}

void Skybox::setCubemap(const gpu::TexturePointer& cubemap) {
    _cubemap = cubemap;
}

void Skybox::render(gpu::Batch& batch, const ViewFrustum& viewFrustum, const Skybox& skybox) {

    if (skybox.getCubemap()) {

        static gpu::PipelinePointer thePipeline;
        if (!thePipeline) {
            auto skyVS = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(Skybox_vert)));
            auto skyFS = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(Skybox_frag)));
            auto skyShader = gpu::ShaderPointer(gpu::Shader::createProgram(skyVS, skyFS));

            gpu::Shader::BindingSet bindings;
            bindings.insert(gpu::Shader::Binding(std::string("cubeMap"), 0));

            if (!gpu::Shader::makeProgram(*skyShader, bindings)) {

            }

            auto skyState = gpu::StatePointer(new gpu::State());

            thePipeline = gpu::PipelinePointer(gpu::Pipeline::create(skyShader, skyState));
        }

        glm::mat4 projMat;
        viewFrustum.evalProjectionMatrix(projMat);

        Transform viewTransform;
        viewFrustum.evalViewTransform(viewTransform);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewTransform);
        batch.setPipeline(thePipeline);
        batch.setUniformTexture(0, skybox.getCubemap());
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    } else {
        // skybox has no cubemap, just clear the color buffer
        auto color = skybox.getColor();
        batch.clearFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(skybox.getColor(),1.0f), 0.f, 0); 
    }
}
