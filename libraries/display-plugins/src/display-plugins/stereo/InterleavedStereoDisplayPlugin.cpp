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

InterleavedStereoDisplayPlugin::InterleavedStereoDisplayPlugin() {
}

void InterleavedStereoDisplayPlugin::customizeContext() {
    StereoDisplayPlugin::customizeContext();
    // Set up the stencil buffers?  Or use a custom shader?
    compileProgram(_program, INTERLEAVED_TEXTURED_VS, INTERLEAVED_TEXTURED_FS);
}

glm::uvec2 InterleavedStereoDisplayPlugin::getRecommendedRenderSize() const {
    uvec2 result = WindowOpenGLDisplayPlugin::getRecommendedRenderSize();
    result.x *= 2;
    result.y /= 2;
    return result;
}

void InterleavedStereoDisplayPlugin::internalPresent() {
    using namespace oglplus;
    _program->Bind();
    auto sceneSize = getRecommendedRenderSize();
    Uniform<ivec2>(*_program, "textureSize").SetValue(sceneSize);
    WindowOpenGLDisplayPlugin::internalPresent();
}
