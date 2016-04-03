//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterleavedStereoDisplayPlugin.h"

static const char * INTERLEAVED_TEXTURED_VS = R"VS(#version 410 core
#pragma line __LINE__

in vec3 Position;
in vec2 TexCoord;

out vec2 vTexCoord;

void main() {
  gl_Position = vec4(Position, 1);
  vTexCoord = TexCoord;
}

)VS";

static const char * INTERLEAVED_TEXTURED_FS = R"FS(#version 410 core
#pragma line __LINE__

uniform sampler2D sampler;
uniform ivec2 textureSize;

in vec2 vTexCoord;
out vec4 FragColor;

void main() {
    ivec2 texCoord = ivec2(floor(vTexCoord * textureSize));
    texCoord.x /= 2;
    int row = int(floor(gl_FragCoord.y));
    if (row % 2 > 0) {
        texCoord.x += (textureSize.x / 2);
    }
    FragColor = texelFetch(sampler, texCoord, 0); //texture(sampler, texCoord);
}

)FS";

const QString InterleavedStereoDisplayPlugin::NAME("3D TV - Interleaved");

void InterleavedStereoDisplayPlugin::customizeContext() {
    StereoDisplayPlugin::customizeContext();
    // Set up the stencil buffers?  Or use a custom shader?
    compileProgram(_interleavedProgram, INTERLEAVED_TEXTURED_VS, INTERLEAVED_TEXTURED_FS);
}

void InterleavedStereoDisplayPlugin::uncustomizeContext() {
    _interleavedProgram.reset();
    StereoDisplayPlugin::uncustomizeContext();
}

glm::uvec2 InterleavedStereoDisplayPlugin::getRecommendedRenderSize() const {
    uvec2 result = Parent::getRecommendedRenderSize();
    result.x *= 2;
    result.y /= 2;
    return result;
}

void InterleavedStereoDisplayPlugin::internalPresent() {
    using namespace oglplus;
    auto sceneSize = getRecommendedRenderSize();
    _interleavedProgram->Bind();
    Uniform<ivec2>(*_interleavedProgram, "textureSize").SetValue(sceneSize);
    auto surfaceSize = getSurfacePixels();
    Context::Viewport(0, 0, surfaceSize.x, surfaceSize.y);
    glBindTexture(GL_TEXTURE_2D, GetName(_compositeFramebuffer->color));
    _plane->Use();
    _plane->Draw();
    swapBuffers();
}

