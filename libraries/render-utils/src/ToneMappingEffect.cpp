//
//  ToneMappingEffect.cpp
//  libraries/render-utils/src
//
//  Created by Sam Gateau on 12/7/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ToneMappingEffect.h"

#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include <RenderArgs.h>

#include "FramebufferCache.h"


ToneMappingEffect::ToneMappingEffect() {

}

void ToneMappingEffect::init() {
    //auto VSFS = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
    //auto PSBlit = gpu::StandardShaderLib::getDrawTexturePS();
    const char BlitTextureGamma_frag[] = R"SCRIBE(#version 410 core
        //  Generated on Sat Oct 24 09:34:37 2015
        //
        //  Draw texture 0 fetched at texcoord.xy
        //
        //  Created by Sam Gateau on 6/22/2015
        //  Copyright 2015 High Fidelity, Inc.
        //
        //  Distributed under the Apache License, Version 2.0.
        //  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
        //
        
        
        uniform sampler2D colorMap;
        
        in vec2 varTexCoord0;
        out vec4 outFragColor;
        
        void main(void) {
            vec4 fragColor = texture(colorMap, varTexCoord0);
            // if (gl_FragCoord.x > 1000) {
            // Manually gamma correct from Ligthing BUffer to color buffer
       //    outFragColor.xyz = pow( fragColor.xyz , vec3(1.0 / 2.2) );

            fragColor *= 2.0;  // Hardcoded Exposure Adjustment
            vec3 x = max(vec3(0.0),fragColor.xyz-0.004);
            vec3 retColor = (x*(6.2*x+.5))/(x*(6.2*x+1.7)+0.06);

        //    fragColor *= 8;  // Hardcoded Exposure Adjustment
        //    fragColor = fragColor/(1.0+fragColor);
        //    vec3 retColor = pow(fragColor.xyz,vec3(1/2.2));

            outFragColor = vec4(retColor, 1.0);
            // }
        }
        
        )SCRIBE";
    auto blitPS = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(BlitTextureGamma_frag)));

    //auto blitProgram = gpu::StandardShaderLib::getProgram(gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS, gpu::StandardShaderLib::getDrawTexturePS);
    auto blitVS = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
    auto blitProgram = gpu::ShaderPointer(gpu::Shader::createProgram(blitVS, blitPS));

    //auto blitProgram = gpu::StandardShaderLib::getProgram(gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS, gpu::StandardShaderLib::getDrawTexturePS);
    gpu::Shader::makeProgram(*blitProgram);
    auto blitState = std::make_shared<gpu::State>();
    blitState->setColorWriteMask(true, true, true, true);
    _blitLightBuffer = gpu::PipelinePointer(gpu::Pipeline::create(blitProgram, blitState));
}


void ToneMappingEffect::render(RenderArgs* args) {
    if (!_blitLightBuffer) {
        init();
    }
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);
        QSize framebufferSize = framebufferCache->getFrameBufferSize();

        auto lightingBuffer = framebufferCache->getLightingTexture();
        auto destFbo = framebufferCache->getPrimaryFramebufferDepthColor();
        batch.setFramebuffer(destFbo);
        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());
        {
            float sMin = args->_viewport.x / (float)framebufferSize.width();
            float sWidth = args->_viewport.z / (float)framebufferSize.width();
            float tMin = args->_viewport.y / (float)framebufferSize.height();
            float tHeight = args->_viewport.w / (float)framebufferSize.height();
            Transform model;
            batch.setPipeline(_blitLightBuffer);
            model.setTranslation(glm::vec3(sMin, tMin, 0.0));
            model.setScale(glm::vec3(sWidth, tHeight, 1.0));
            batch.setModelTransform(model);
        }

        batch.setResourceTexture(0, lightingBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        args->_context->render(batch);
    });
}