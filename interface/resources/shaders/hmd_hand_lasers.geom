//
//  Created by Bradley Austin Davis on 2016/07/11
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#version 410 core
#extension GL_EXT_geometry_shader4 : enable

layout(location = 0) out vec3 outLineDistance;

layout(lines) in;
layout(triangle_strip, max_vertices = 24) out;

vec3[2] getOrthogonals(in vec3 n, float scale) {
    float yDot = abs(dot(n, vec3(0, 1, 0)));

    vec3 result[2];
    if (yDot < 0.9) {
        result[0] = normalize(cross(n, vec3(0, 1, 0)));
    } else {
        result[0] = normalize(cross(n, vec3(1, 0, 0)));
    }
    // The cross of result[0] and n is orthogonal to both, which are orthogonal to each other
    result[1] = cross(result[0], n);
    result[0] *= scale;
    result[1] *= scale;
    return result;
}


vec2 orthogonal(vec2 v) {
    vec2 result = v.yx;
    result.y *= -1.0;
    return result;
}

void main() {
    vec2 endpoints[2];
    for (int i = 0; i < 2; ++i) {
        endpoints[i] = gl_PositionIn[i].xy / gl_PositionIn[i].w;
    }
    vec2 lineNormal = normalize(endpoints[1] - endpoints[0]);
    vec2 lineOrthogonal = orthogonal(lineNormal);
    lineNormal *= 0.02;
    lineOrthogonal *= 0.02;

    gl_Position = gl_PositionIn[0];
    gl_Position.xy -= lineOrthogonal;
    outLineDistance = vec3(-1.02, -1, gl_Position.z);
    EmitVertex();

    gl_Position = gl_PositionIn[0];
    gl_Position.xy += lineOrthogonal;
    outLineDistance = vec3(-1.02, 1, gl_Position.z);
    EmitVertex();

    gl_Position = gl_PositionIn[1];
    gl_Position.xy -= lineOrthogonal;
    outLineDistance = vec3(1.02, -1, gl_Position.z);
    EmitVertex();

    gl_Position = gl_PositionIn[1];
    gl_Position.xy += lineOrthogonal;
    outLineDistance = vec3(1.02, 1, gl_Position.z);
    EmitVertex();

    EndPrimitive();
}
