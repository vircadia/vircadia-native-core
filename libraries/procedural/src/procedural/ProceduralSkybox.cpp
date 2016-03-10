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
    _procedural._vertexSource = ProceduralSkybox_vert;
    _procedural._fragmentSource = ProceduralSkybox_frag;
    // Adjust the pipeline state for background using the stencil test
    _procedural._state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
}

void ProceduralSkybox::render(gpu::Batch& batch, const ViewFrustum& frustum) const {
    ProceduralSkybox::render(batch, frustum, (*this));
}

void ProceduralSkybox::render(gpu::Batch& batch, const ViewFrustum& viewFrustum, const ProceduralSkybox& skybox) {
    if (!(skybox._procedural.ready())) {
        skybox.updateDataBuffer();
        Skybox::render(batch, viewFrustum, skybox);
    } else {
        gpu::TexturePointer skymap = skybox.getCubemap();
        // FIXME: skymap->isDefined may not be threadsafe
        assert(skymap && skymap->isDefined());

        glm::mat4 projMat;
        viewFrustum.evalProjectionMatrix(projMat);

        Transform viewTransform;
        viewFrustum.evalViewTransform(viewTransform);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewTransform);
        batch.setModelTransform(Transform()); // only for Mac
        batch.setResourceTexture(0, skybox.getCubemap());

        skybox._procedural.prepare(batch, glm::vec3(0), glm::vec3(1));
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    }
}
