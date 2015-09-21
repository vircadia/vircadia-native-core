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

void ProceduralSkybox::setProcedural(const ProceduralPointer& procedural) {
    _procedural = procedural;
    if (_procedural) {
        _procedural->_vertexSource = ProceduralSkybox_vert;
        _procedural->_fragmentSource = ProceduralSkybox_frag;
        // No pipeline state customization
    }
}

void ProceduralSkybox::render(gpu::Batch& batch, const ViewFrustum& viewFrustum, const ProceduralSkybox& skybox) {
    static gpu::BufferPointer theBuffer;
    static gpu::Stream::FormatPointer theFormat;

    if (skybox._procedural || skybox.getCubemap()) {
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

        if (skybox._procedural && skybox._procedural->_enabled && skybox._procedural->ready()) {
            if (skybox.getCubemap() && skybox.getCubemap()->isDefined()) {
                batch.setResourceTexture(0, skybox.getCubemap());
            }

            skybox._procedural->prepare(batch, glm::vec3(1));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }
    } else {
        // skybox has no cubemap, just clear the color buffer
        auto color = skybox.getColor();
        batch.clearFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(color, 0.0f), 0.0f, 0, true);
    }
}

