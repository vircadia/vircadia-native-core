//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OglplusHelpers.h"


static const char * SIMPLE_TEXTURED_VS = R"VS(#version 410 core
#pragma line __LINE__

uniform mat4 Projection = mat4(1);
uniform mat4 ModelView = mat4(1);

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord;

out vec2 vTexCoord;

void main() {
  gl_Position = Projection * ModelView * vec4(Position, 1);
  vTexCoord = TexCoord;
}

)VS";

static const char * SIMPLE_TEXTURED_FS = R"FS(#version 410 core
#pragma line __LINE__

uniform sampler2D sampler;
uniform float Alpha = 1.0;

in vec2 vTexCoord;
out vec4 vFragColor;

void main() {
    vec4 c = texture(sampler, vTexCoord);
    c.a = min(Alpha, c.a);
    vFragColor = c; 
}

)FS";


ProgramPtr loadDefaultShader() {
    ProgramPtr result;
    compileProgram(result, SIMPLE_TEXTURED_VS, SIMPLE_TEXTURED_FS);
    return result;
}

void compileProgram(ProgramPtr & result, const std::string& vs, const std::string& fs) {
    using namespace oglplus;
    try {
        result = ProgramPtr(new Program());
        // attach the shaders to the program
        result->AttachShader(
            VertexShader()
            .Source(GLSLSource(vs))
            .Compile()
            );
        result->AttachShader(
            FragmentShader()
            .Source(GLSLSource(fs))
            .Compile()
            );
        result->Link();
    } catch (ProgramBuildError & err) {
        Q_UNUSED(err);
        Q_ASSERT_X(false, "compileProgram", "Failed to build shader program");
        qFatal((const char*)err.Message);
        result.reset();
    }
}


ShapeWrapperPtr loadPlane(ProgramPtr program, float aspect) {
    using namespace oglplus;
    Vec3f a(1, 0, 0);
    Vec3f b(0, 1, 0);
    if (aspect > 1) {
        b[1] /= aspect;
    } else {
        a[0] *= aspect;
    }
    return ShapeWrapperPtr(
        new shapes::ShapeWrapper({ "Position", "TexCoord" }, shapes::Plane(a, b), *program)
    );
}


static const char * QUAD_VS = R"VS(#version 330

uniform mat4 Projection = mat4(1);
uniform mat4 ModelView = mat4(1);
uniform vec2 UvMultiplier = vec2(1);

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord;

out vec2 vTexCoord;

void main() {
  gl_Position = Projection * ModelView * vec4(Position, 1);
  vTexCoord = TexCoord * UvMultiplier;
}
)VS";

static const char * QUAD_FS = R"FS(#version 330
uniform sampler2D sampler;
uniform float Alpha = 1.0;

in vec2 vTexCoord;
out vec4 vFragColor;

void main() {
    vec4 c = texture(sampler, vTexCoord);
    c.a = min(Alpha, c.a);
    vFragColor = c; 
}
)FS";
