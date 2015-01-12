#version 120

//
//  metavoxel_heightfield_base.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 8/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the height texture
uniform sampler2D heightMap;

// the distance between height points in texture space
uniform vec4 heightScale;

// the scale between height and color textures
uniform vec2 colorScale;

// the interpolated normal
varying vec4 normal;

void main(void) {
    // transform and store the normal for interpolation
    vec2 heightCoord = gl_MultiTexCoord0.st;
    vec4 neighborHeights = vec4(texture2D(heightMap, heightCoord - vec2(heightScale.s, 0.0)).r,
        texture2D(heightMap, heightCoord + vec2(heightScale.s, 0.0)).r,
        texture2D(heightMap, heightCoord - vec2(0.0, heightScale.t)).r,
        texture2D(heightMap, heightCoord + vec2(0.0, heightScale.t)).r);
    vec4 neighborsZero = step(1.0 / 65535.0, neighborHeights);
    normal = vec4(normalize(gl_NormalMatrix * vec3(
        heightScale.p * (neighborHeights.y - neighborHeights.x) * neighborsZero.x * neighborsZero.y, 1.0,
        heightScale.q * (neighborHeights.w - neighborHeights.z) * neighborsZero.z * neighborsZero.w)), 0.0);
    
    // add the height to the position
    float height = texture2D(heightMap, heightCoord).r;
    gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex + vec4(0.0, height, 0.0, 0.0));
    
    // the zero height should be invisible
    gl_FrontColor = vec4(1.0, 1.0, 1.0, step(height, 0.0));
    
    // pass along the scaled/offset texture coordinates
    gl_TexCoord[0] = vec4((heightCoord - heightScale.st) * colorScale, 0.0, 1.0);
}
