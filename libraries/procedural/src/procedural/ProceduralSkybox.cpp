//
//  ProceduralSkybox.cpp
//  libraries/procedural/src/procedural
//
//  Created by Sam Gateau on 9/21/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ProceduralSkybox.h"


#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <ViewFrustum.h>

#include "ProceduralSkybox_vert.h"
#include "ProceduralSkybox_frag.h"

ProceduralSkybox::ProceduralSkybox() : model::Skybox() {
}

ProceduralSkybox::ProceduralSkybox(const ProceduralSkybox& skybox) :
    model::Skybox(skybox),
    _procedural(skybox._procedural) {

}

void ProceduralSkybox::setProcedural(const ProceduralPointer& procedural) {
    _procedural = procedural;
    if (_procedural) {
        _procedural->_vertexSource = ProceduralSkybox_vert;
        _procedural->_fragmentSource = ProceduralSkybox_frag;
        // Adjust the pipeline state for background using the stencil test
        _procedural->_state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
    }
}

void ProceduralSkybox::render(gpu::Batch& batch, const ViewFrustum& frustum) const {
    ProceduralSkybox::render(batch, frustum, (*this));
}

void ProceduralSkybox::render(gpu::Batch& batch, const ViewFrustum& viewFrustum, const ProceduralSkybox& skybox) {
    if (!(skybox._procedural)) {
        skybox.updateDataBuffer();
        Skybox::render(batch, viewFrustum, skybox);
    }

    static gpu::BufferPointer theBuffer;
    static gpu::Stream::FormatPointer theFormat;

    if (skybox._procedural && skybox._procedural->_enabled && skybox._procedural->ready()) {
        if (!theBuffer) {
            const float CLIP = 1.0f;
            const glm::vec2 vertices[4] = { { -CLIP, -CLIP }, { CLIP, -CLIP }, { -CLIP, CLIP }, { CLIP, CLIP } };
            theBuffer = std::make_shared<gpu::Buffer>(sizeof(vertices), (const gpu::Byte*) vertices);
            theFormat = std::make_shared<gpu::Stream::Format>();
            theFormat->setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ));
        }

        glm::mat4 projMat;
        viewFrustum.evalProjectionMatrix(projMat);

        Transform viewTransform;
        viewFrustum.evalViewTransform(viewTransform);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewTransform);
        batch.setModelTransform(Transform()); // only for Mac
        batch.setInputBuffer(gpu::Stream::POSITION, theBuffer, 0, 8);
        batch.setInputFormat(theFormat);

        if (skybox.getCubemap() && skybox.getCubemap()->isDefined()) {
            batch.setResourceTexture(0, skybox.getCubemap());
        }

        skybox._procedural->prepare(batch, glm::vec3(0), glm::vec3(1));
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    }
}

