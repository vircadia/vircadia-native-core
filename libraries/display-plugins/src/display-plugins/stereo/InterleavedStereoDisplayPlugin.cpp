//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterleavedStereoDisplayPlugin.h"

#include <gpu/StandardShaderLib.h>
#include <gpu/Pipeline.h>
#include <gpu/Batch.h>
#include <gpu/Context.h>

static const char* INTERLEAVED_SRGB_TO_LINEAR_FRAG = R"SCRIBE(

struct TextureData {
    ivec2 textureSize;
};

layout(std140) uniform textureDataBuffer {
    TextureData textureData;
};

uniform sampler2D colorMap;

in vec2 varTexCoord0;

out vec4 outFragColor;

void main(void) {
    ivec2 texCoord = ivec2(floor(varTexCoord0 * textureData.textureSize));
    texCoord.x /= 2;
    int row = int(floor(gl_FragCoord.y));
    if (row % 2 > 0) {
        texCoord.x += (textureData.textureSize.x / 2);
    }
    outFragColor = vec4(pow(texelFetch(colorMap, texCoord, 0).rgb, vec3(2.2)), 1.0);
}

)SCRIBE";

const QString InterleavedStereoDisplayPlugin::NAME("3D TV - Interleaved");

void InterleavedStereoDisplayPlugin::customizeContext() {
    StereoDisplayPlugin::customizeContext();
    if (!_interleavedPresentPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(INTERLEAVED_SRGB_TO_LINEAR_FRAG));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
        gpu::Shader::makeProgram(*program);
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(gpu::State::DepthTest(false));
        _interleavedPresentPipeline = gpu::Pipeline::create(program, state);
    }
}

void InterleavedStereoDisplayPlugin::uncustomizeContext() {
    _interleavedPresentPipeline.reset();
    StereoDisplayPlugin::uncustomizeContext();
}

glm::uvec2 InterleavedStereoDisplayPlugin::getRecommendedRenderSize() const {
    uvec2 result = Parent::getRecommendedRenderSize();
    result.x *= 2;
    result.y /= 2;
    return result;
}

void InterleavedStereoDisplayPlugin::internalPresent() {
    gpu::Batch presentBatch;
    presentBatch.enableStereo(false);
    presentBatch.resetViewTransform();
    presentBatch.setFramebuffer(gpu::FramebufferPointer());
    presentBatch.setViewportTransform(ivec4(uvec2(0), getSurfacePixels()));
    presentBatch.setResourceTexture(0, _currentFrame->framebuffer->getRenderBuffer(0));
    presentBatch.setPipeline(_interleavedPresentPipeline);
    presentBatch.draw(gpu::TRIANGLE_STRIP, 4);
    _backend->render(presentBatch);
    swapBuffers();
}
