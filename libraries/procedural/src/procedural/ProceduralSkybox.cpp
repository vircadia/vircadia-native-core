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

#include <model/skybox_vert.h>
#include <model/skybox_frag.h>

ProceduralSkybox::ProceduralSkybox() : model::Skybox() {
    _procedural._vertexSource = skybox_vert;
    _procedural._fragmentSource = skybox_frag;
    // Adjust the pipeline state for background using the stencil test
    _procedural._state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
}

void ProceduralSkybox::clear() {
    // Parse and prepare a procedural with no shaders to release textures
    parse(QString());
    _procedural.ready();

    Skybox::clear();
}

void ProceduralSkybox::render(gpu::Batch& batch, const ViewFrustum& frustum) const {
    if (_procedural.ready()) {
        ProceduralSkybox::render(batch, frustum, (*this));
    } else {
        Skybox::render(batch, frustum);
    }
}

void ProceduralSkybox::render(gpu::Batch& batch, const ViewFrustum& viewFrustum, const ProceduralSkybox& skybox) {
    glm::mat4 projMat;
    viewFrustum.evalProjectionMatrix(projMat);

    Transform viewTransform;
    viewFrustum.evalViewTransform(viewTransform);
    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewTransform);
    batch.setModelTransform(Transform()); // only for Mac

    auto& procedural = skybox._procedural;
    procedural.prepare(batch, glm::vec3(0), glm::vec3(1), glm::quat());
    auto textureSlot = procedural.getShader()->getTextures().findLocation("cubeMap");
    auto bufferSlot = procedural.getShader()->getBuffers().findLocation("skyboxBuffer");
    skybox.prepare(batch, textureSlot, bufferSlot);
    batch.draw(gpu::TRIANGLE_STRIP, 4);
}

